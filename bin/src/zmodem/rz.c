#define VERSION "2.03 05-17-88"

/*
 * rz.c By Chuck Forsberg
 *
 *  Unix is a trademark of Western Electric Company
 *
 * A program for Unix to receive files and commands from computers running
 *  Professional-YAM, PowerCom, YAM, IMP, or programs supporting XMODEM.
 *  rz uses Unix buffered input to reduce wasted CPU time.
 *
 * Iff the program is invoked by rzCOMMAND, output is piped to
 * "COMMAND filename"  (Unix only)
 *
 *  NFGVMIN Updated 2-18-87 CAF for Xenix systems where c_cc[VMIN]
 *  doesn't work properly (even though it compiles without error!),
 *
 *  SEGMENTS=n added 2-21-88 as a model for CP/M programs
 *    for CP/M-80 systems that cannot overlap modem and disk I/O.
 *
 *  -DMD may be added to compiler command line to compile in
 *    Directory-creating routines from Public Domain TAR by John Gilmore
 *
 *  USG UNIX (3.0) ioctl conventions courtesy  Jeff Martin
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "zmodem.h"

#define LOGFILE "/tmp/rzlog"
#define PUBDIR "/usr/spool/uucppublic"

int Nozmodem;   /* If invoked as "rb" */

FILE *fout;

int Lastrx;
int Crcflg;
int Firstsec;
int Eofseen;        /* indicates cpm eof (^Z) has been received */
int Restricted;   /* restricted; no /.. or ../ in filenames */

#define DEFBYTL 2000000000L /* default rx file size */
long Bytesleft;     /* number of bytes of incoming file left */
static long modtime; /* Unix style mod time for incoming file */
int Filemode;       /* Unix style mode for incoming file */
char Pathname[PATHLEN];
char *Progname;     /* the name by which we were called */

int Batch;
int Topipe;
int MakeLCPathname = TRUE;    /* make received pathname lower case */
int Nflag;      /* Don't really transfer files */
int Rxclob = FALSE;   /* Clobber existing file */
int Rxbinary = FALSE; /* receive all files in bin mode */
int Rxascii = FALSE;  /* receive files in ascii (translate) mode */
int Thisbinary;     /* current file is to be received in bin mode */
int Blklen;     /* record length of received packets */

#ifdef SEGMENTS
int chinseg;    /* Number of characters received in this data seg */
char secbuf[1 + (SEGMENTS + 1) * 1024];
#else
char secbuf[1025];
#endif

char Lzmanag;       /* Local file management request */
char zconv;     /* ZMODEM file conversion request */
char zmanag;        /* ZMODEM file management request */
char ztrans;        /* ZMODEM file transport request */
int Zctlesc;        /* Encode control characters */
int Zrwindow = 1400;    /* RX window size (controls garbage count) */

/** Header type to send corresponding to Last rx close */
int tryzhdrtype = ZRINIT;

/*
 * Routine to calculate the free bytes on the current file system
 *  ~0 means many free bytes (unknown)
 */
static long getfree(void)
{
    return ~0L;    /* many free bytes ... */
}

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
static void zmputs(char *s)
{
    int c;

    while (*s) {
        switch (c = *s++) {
        case '\336':
            sleep(1);
            continue;
        case '\335':
            sendbrk();
            continue;
        default:
            sendline(c);
        }
    }
}

/* called by signal interrupt or terminate to clean things up */
void bibi(int n)
{
    if (Zmodem)
        zmputs(Attn);
    canit();
    mode(0);
    fprintf(stderr, "rz: caught signal %d; exiting\n", n);
    cucheck();
    exit(128 + n);
}

/*
 * Totalitarian Communist pathname processing
 */
static void checkpath(char *name)
{
    if (Restricted) {
        struct stat buffer;

        if (stat(name, &buffer) != -1) {
            canit();
            fprintf(stderr, "\r\nrz: %s exists\n", name);
            bibi(-1);
        }
        /* restrict pathnames to current tree or uucppublic */
        if (substr(name, "../")
         || (name[0] == '/' && strncmp(name, PUBDIR, strlen(PUBDIR)))) {
            canit();
            fprintf(stderr, "\r\nrz:\tSecurity Violation\r\n");
            bibi(-1);
        }
    }
}

/*
 * Purge the modem input queue of all characters
 */
static void purgeline(void)
{
    Lleft = 0;
#ifdef USG
    ioctl(iofd, TCFLSH, 0);
#else
    lseek(iofd, 0L, 2);
#endif
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
static void ackbibi(void)
{
    int n;

    vfile("ackbibi:");
    Readnum = 1;
    stohdr(0L);
    for (n = 3; --n >= 0;) {
        purgeline();
        zshhdr(ZFIN, Txhdr);
        switch (readline(100)) {
        case 'O':
            readline(1);    /* Discard 2nd 'O' */
            vfile("ackbibi complete");
            return;
        case RCDO:
            return;
        case TIMEOUT:
        default:
            break;
        }
    }
}

/*
 * Strip leading ! if present, do shell escape.
 */
static int sys2(char *s)
{
    if (*s == '!')
        ++s;
    return system(s);
}

/*
 * Strip leading ! if present, do exec.
 */
static void exec2(char *s)
{
    if (*s == '!')
        ++s;
    mode(0);
    execl("/bin/sh", "sh", "-c", s, NULL);
}

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
static int tryz(void)
{
    int c, n, cmdzack1flg;

    if (Nozmodem)       /* Check for "rb" program name */
        return 0;


    for (n = Zmodem ? 15 : 5; --n >= 0;) {
        /* Set buffer length (0) and capability flags */
#ifdef SEGMENTS
        stohdr(SEGMENTS*1024L);
#else
        stohdr(0L);
#endif
#ifdef CANBREAK
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO | CANBRK;
#else
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
#endif
        if (Zctlesc)
            Txhdr[ZF0] |= TESCCTL;
        zshhdr(tryzhdrtype, Txhdr);
        if (tryzhdrtype == ZSKIP)   /* Don't skip too far */
            tryzhdrtype = ZRINIT;   /* CAF 8-21-87 */
again:
        switch (zgethdr(Rxhdr, 0)) {
        case ZRQINIT:
            continue;
        case ZEOF:
            continue;
        case TIMEOUT:
            continue;
        case ZFILE:
            zconv = Rxhdr[ZF0];
            zmanag = Rxhdr[ZF1];
            ztrans = Rxhdr[ZF2];
            tryzhdrtype = ZRINIT;
            c = zrdata(secbuf, 1024);
            mode(3);
            if (c == GOTCRCW)
                return ZFILE;
            zshhdr(ZNAK, Txhdr);
            goto again;
        case ZSINIT:
            Zctlesc = TESCCTL & Rxhdr[ZF0];
            if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
                stohdr(1L);
                zshhdr(ZACK, Txhdr);
                goto again;
            }
            zshhdr(ZNAK, Txhdr);
            goto again;
        case ZFREECNT:
            stohdr(getfree());
            zshhdr(ZACK, Txhdr);
            goto again;
        case ZCOMMAND:
            cmdzack1flg = Rxhdr[ZF0];
            if (zrdata(secbuf, 1024) == GOTCRCW) {
                if (cmdzack1flg & ZCACK1)
                    stohdr(0L);
                else
                    stohdr((long)sys2(secbuf));
                purgeline();    /* dump impatient questions */
                do {
                    zshhdr(ZCOMPL, Txhdr);
                } while (++errors < 20 && zgethdr(Rxhdr, 1) != ZFIN);
                ackbibi();
                if (cmdzack1flg & ZCACK1)
                    exec2(secbuf);
                return ZCOMPL;
            }
            zshhdr(ZNAK, Txhdr);
            goto again;
        case ZCOMPL:
            goto again;
        default:
            continue;
        case ZFIN:
            ackbibi();
            return ZCOMPL;
        case ZCAN:
            return ERROR;
        }
    }
    return 0;
}

/*
 * Close the receive dataset, return OK or ERROR
 */
static int closeit(void)
{
    time_t q;

    if (Topipe) {
        if (pclose(fout)) {
            return ERROR;
        }
        return OK;
    }
    if (fclose(fout) == ERROR) {
        fprintf(stderr, "file close ERROR\n");
        return ERROR;
    }
    if (modtime != 0) {
        const struct timeval times[2] = {
            { .tv_sec = time(&q), .tv_usec = 0 },
            { .tv_sec = modtime, .tv_usec = 0 }
        };

        utimes(Pathname, times);
    }
    if ((Filemode & S_IFMT) == S_IFREG)
        chmod(Pathname, (07777 & Filemode));
    return OK;
}

/*
 * Process incoming file information header
 */
static int procheader(char *name)
{
    char *openmode, *p;

    /* set default parameters and overrides */
    openmode = "w";
    Thisbinary = (!Rxascii) || Rxbinary;
    if (Lzmanag)
        zmanag = Lzmanag;

    /*
     *  Process ZMODEM remote file management requests
     */
    if (!Rxbinary && zconv == ZCNL) /* Remote ASCII override */
        Thisbinary = 0;
    if (zconv == ZCBIN) /* Remote Binary override */
        Thisbinary = TRUE;
    else if (zmanag == ZMAPND)
        openmode = "a";

    /* Check for existing file */
    struct stat statbuf;
    if (!Rxclob && (zmanag&ZMMASK) != ZMCLOB && stat(name, &statbuf) == -1) {
        return ERROR;
    }

    Bytesleft = DEFBYTL;
    Filemode = 0;
    modtime = 0;

    p = name + 1 + strlen(name);
    if (*p) {   /* file coming from Unix or DOS system */
        sscanf(p, "%ld%lo%o", &Bytesleft, &modtime, &Filemode);
        if (Filemode & UNIXFILE)
            ++Thisbinary;
        if (Verbose) {
            fprintf(stderr,  "\nIncoming: %s %ld %lo %o\n",
              name, Bytesleft, modtime, Filemode);
        }
    } else {      /* File coming from CP/M system */
        for (p = name; *p; ++p) {     /* change / to _ */
            if (*p == '/')
                *p = '_';
        }

        if (*--p == '.')       /* zap trailing period */
            *p = 0;
    }

    if (!Zmodem && MakeLCPathname && !IsAnyLower(name)
      && !(Filemode&UNIXFILE))
        uncaps(name);
    if (Topipe > 0) {
        sprintf(Pathname, "%s %s", Progname + 2, name);
        if (Verbose) {
            fprintf(stderr,  "Topipe: %s %s\n",
              Pathname, Thisbinary ? "BIN" : "ASCII");
        }
        if ((fout = popen(Pathname, "w")) == NULL)
            return ERROR;
    } else {
        strcpy(Pathname, name);
        if (Verbose) {
            fprintf(stderr,  "Receiving %s %s %s\n",
              name, Thisbinary ? "BIN" : "ASCII", openmode);
        }
        checkpath(name);
        if (Nflag)
            name = "/dev/null";
#ifdef OMEN
        if (name[0] == '!' || name[0] == '|') {
            if (!(fout = popen(name + 1, "w"))) {
                return ERROR;
            }
            Topipe = -1;  return(OK);
        }
#endif
#ifdef MD
        fout = fopen(name, openmode);
        if (!fout)
            if (make_dirs(name))
                fout = fopen(name, openmode);
#else
        fout = fopen(name, openmode);
#endif
        if (!fout)
            return ERROR;
    }
    return OK;
}

/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
static int putsec(char *buf, int n)
{
    char *p;

    if (n == 0)
        return OK;
    if (Thisbinary) {
        for (p = buf; --n >= 0;) {
            putc(*p++, fout);
        }
    } else {
        if (Eofseen)
            return OK;
        for (p = buf; --n >= 0; ++p) {
            if (*p == '\r')
                continue;
            if (*p == CPMEOF) {
                Eofseen = TRUE;
                return OK;
            }
            putc(*p, fout);
        }
    }
    return OK;
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
static int rzfile(void)
{
    int c, n;
    long rxbytes;

    Eofseen = FALSE;
    if (procheader(secbuf) == ERROR) {
        tryzhdrtype = ZSKIP;
        return ZSKIP;
    }

    n = 20; rxbytes = 0l;

    for (;;) {
#ifdef SEGMENTS
        chinseg = 0;
#endif
        stohdr(rxbytes);
        zshhdr(ZRPOS, Txhdr);
nxthdr:
        switch (c = zgethdr(Rxhdr, 0)) {
        default:
            vfile("rzfile: zgethdr returned %d", c);
            return ERROR;
        case ZNAK:
        case TIMEOUT:
#ifdef SEGMENTS
            putsec(secbuf, chinseg);
            chinseg = 0;
#endif
            if (--n < 0) {
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            }
        case ZFILE:
            zrdata(secbuf, 1024);
            continue;
        case ZEOF:
#ifdef SEGMENTS
            putsec(secbuf, chinseg);
            chinseg = 0;
#endif
            if (rclhdr(Rxhdr) != rxbytes) {
                /*
                 * Ignore eof if it's at wrong place - force
                 *  a timeout because the eof might have gone
                 *  out before we sent our zrpos.
                 */
                errors = 0;  goto nxthdr;
            }
            if (closeit()) {
                tryzhdrtype = ZFERR;
                vfile("rzfile: closeit returned <> 0");
                return ERROR;
            }
            vfile("rzfile: normal EOF");
            return c;
        case ERROR: /* Too much garbage in header search error */
#ifdef SEGMENTS
            putsec(secbuf, chinseg);
            chinseg = 0;
#endif
            if (--n < 0) {
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            }
            zmputs(Attn);
            continue;
        case ZSKIP:
#ifdef SEGMENTS
            putsec(secbuf, chinseg);
            chinseg = 0;
#endif
            closeit();
            vfile("rzfile: Sender SKIPPED file");
            return c;
        case ZDATA:
            if (rclhdr(Rxhdr) != rxbytes) {
                if (--n < 0) {
                    return ERROR;
                }
#ifdef SEGMENTS
                putsec(secbuf, chinseg);
                chinseg = 0;
#endif
                zmputs(Attn);  continue;
            }
moredata:
            if (Verbose > 1) {
                fprintf(stderr, "\r%7ld ZMODEM%s    ",
                        rxbytes, Crc32 ? " CRC-32" : "");
            }
#ifdef SEGMENTS
            if (chinseg >= (1024 * SEGMENTS)) {
                putsec(secbuf, chinseg);
                chinseg = 0;
            }
            switch (c = zrdata(secbuf+chinseg, 1024))
#else
            switch (c = zrdata(secbuf, 1024))
#endif
            {
            case ZCAN:
#ifdef SEGMENTS
                putsec(secbuf, chinseg);
                chinseg = 0;
#endif
                vfile("rzfile: zgethdr returned %d", c);
                return ERROR;
            case ERROR: /* CRC error */
#ifdef SEGMENTS
                putsec(secbuf, chinseg);
                chinseg = 0;
#endif
                if (--n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return ERROR;
                }
                zmputs(Attn);
                continue;
            case TIMEOUT:
#ifdef SEGMENTS
                putsec(secbuf, chinseg);
                chinseg = 0;
#endif
                if (--n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return ERROR;
                }
                continue;
            case GOTCRCW:
                n = 20;
#ifdef SEGMENTS
                chinseg += Rxcount;
                putsec(secbuf, chinseg);
                chinseg = 0;
#else
                putsec(secbuf, Rxcount);
#endif
                rxbytes += Rxcount;
                stohdr(rxbytes);
                zshhdr(ZACK, Txhdr);
                sendline(XON);
                goto nxthdr;
            case GOTCRCQ:
                n = 20;
#ifdef SEGMENTS
                chinseg += Rxcount;
#else
                putsec(secbuf, Rxcount);
#endif
                rxbytes += Rxcount;
                stohdr(rxbytes);
                zshhdr(ZACK, Txhdr);
                goto moredata;
            case GOTCRCG:
                n = 20;
#ifdef SEGMENTS
                chinseg += Rxcount;
#else
                putsec(secbuf, Rxcount);
#endif
                rxbytes += Rxcount;
                goto moredata;
            case GOTCRCE:
                n = 20;
#ifdef SEGMENTS
                chinseg += Rxcount;
#else
                putsec(secbuf, Rxcount);
#endif
                rxbytes += Rxcount;
                goto nxthdr;
            }
        }
    }
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */
static int rzfiles(void)
{
    int c;

    for (;;) {
        switch (c = rzfile()) {
        case ZEOF:
        case ZSKIP:
            switch (tryz()) {
            case ZCOMPL:
                return OK;
            default:
                return ERROR;
            case ZFILE:
                break;
            }
            continue;
        default:
            return c;
        case ERROR:
            return ERROR;
        }
    }
}

static int usage(void)
{
    cucheck();
    fprintf(stderr, "Usage:  rz [-abeuvy]        (ZMODEM)\n");
    fprintf(stderr, "or  rb [-abuvy]     (YMODEM)\n");
    fprintf(stderr, "or  rx [-abcv] file (XMODEM or XMODEM-1k)\n");
    fprintf(stderr, "      -a ASCII transfer (strip CR)\n");
    fprintf(stderr, "      -b Binary transfer for all files\n");
    fprintf(stderr, "      -c Use 16 bit CRC (XMODEM)\n");
    fprintf(stderr, "      -e Escape control characters  (ZMODEM)\n");
    fprintf(stderr, "      -v Verbose more v's give more info\n");
    fprintf(stderr, "      -y Yes, clobber existing file if any\n");
    fprintf(stderr, "%s %s by Chuck Forsberg, Omen Technology INC\n",
            Progname, VERSION);
    fprintf(stderr, "\t\t\042The High Reliability Software\042\n");
    exit(0);
}

/*
 * If called as [-][dir/../]vrzCOMMAND set Verbose to 1
 * If called as [-][dir/../]rzCOMMAND set the pipe flag
 * If called as rb use YMODEM protocol
 */
static void chkinvok(char *s)
{
    char *p = s;

    while (*p == '-') {
        s = ++p;
    }
    while (*p) {
        if (*p++ == '/')
            s = p;
    }
    if (*s == 'v') {
        Verbose = 1;
        ++s;
    }
    Progname = s;
    if (s[0] == 'r' && s[1] == 'z')
        Batch = TRUE;
    if (s[0] == 'r' && s[1] == 'b')
        Batch = Nozmodem = TRUE;
    if (s[2] && s[0] == 'r' && s[1] == 'b')
        Topipe = 1;
    if (s[2] && s[0] == 'r' && s[1] == 'z')
        Topipe = 1;
}

/*
 * Let's receive something already.
 */

char *rbmsg =
"%s ready. To begin transfer, type \"%s file ...\" to your modem program\r\n\n";

/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or ERROR if valid sector not received,
 * or CAN CAN received
 * or WCEOT if eot sector
 * time is timeout for first char, set to 4 seconds thereafter
 ***************** NO ACK IS SENT IF SECTOR IS RECEIVED OK **************
 *    (Caller must do that when he is good and ready to get next sector)
 */

static int wcgetsec(char *rxbuf, int maxtime)
{
    int checksum, wcj, firstch;
    unsigned short oldcrc;
    char *p;
    int sectcurr;

    for (Lastrx = errors = 0; errors < RETRYMAX; errors++) {

        if ((firstch = readline(maxtime)) == STX) {
            Blklen = 1024;
            goto get2;
        }
        if (firstch == SOH) {
            Blklen = 128;
get2:
            sectcurr = readline(1);
            if ((sectcurr + (oldcrc = readline(1))) == 0377) {
                oldcrc = checksum = 0;
                for (p = rxbuf, wcj = Blklen; --wcj >= 0;) {
                    if ((firstch = readline(1)) < 0)
                        goto bilge;
                    oldcrc = updcrc(firstch, oldcrc);
                    checksum += (*p++ = firstch);
                }
                if ((firstch = readline(1)) < 0)
                    goto bilge;
                if (Crcflg) {
                    oldcrc = updcrc(firstch, oldcrc);
                    if ((firstch = readline(1)) < 0)
                        goto bilge;
                    oldcrc = updcrc(firstch, oldcrc);
                    if (oldcrc & 0xFFFF) {
                        zperr("CRC");
                    } else {
                        Firstsec = FALSE;
                        return sectcurr;
                    }
                } else if (((checksum - firstch) & 0377) == 0) {
                    Firstsec = FALSE;
                    return sectcurr;
                } else {
                    zperr("Checksum");
                }
            } else {
                zperr("Sector number garbled");
            }
        }
        /* make sure eot really is eot and not just mixmash */
#ifdef NFGVMIN
        else if (firstch == EOT && readline(1) == TIMEOUT)
            return WCEOT;
#else
        else if (firstch == EOT && Lleft == 0)
            return WCEOT;
#endif
        else if (firstch == CAN) {
            if (Lastrx == CAN) {
                zperr("Sender CANcelled");
                return ERROR;
            } else {
                Lastrx = CAN;
                continue;
            }
        } else if (firstch == TIMEOUT) {
            if (Firstsec)
                goto humbug;
bilge:
            zperr("TIMEOUT");
        } else {
            zperr("Got 0%o sector header", firstch);
        }

humbug:
        Lastrx = 0;
        while (1) {
            if (readline(1) == TIMEOUT)
                break;
        }
        if (Firstsec) {
            sendline(Crcflg ? WANTCRC : NAK);
            Lleft = 0;    /* Do read next time ... */
        } else {
            maxtime = 40; sendline(NAK);
            Lleft = 0;    /* Do read next time ... */
        }
    }
    /* try to stop the bubble machine. */
    canit();
    return ERROR;
}

static void report(int sct)
{
    if (Verbose > 1)
        fprintf(stderr, "%03d%c", sct, sct % 10 ? ' ' : '\r');
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */

int wcrx(void)
{
    int sectnum, sectcurr;
    char sendchar;
    int cblklen;            /* bytes to dump this block */

    Firstsec = TRUE;
    sectnum = 0;
    Eofseen = FALSE;
    sendchar = Crcflg ? WANTCRC : NAK;

    for (;;) {
        sendline(sendchar); /* send it now, we're ready! */
        Lleft = 0;    /* Do read next time ... */
        sectcurr = wcgetsec(secbuf, (sectnum & 0177) ? 50 : 130);
        report(sectcurr);
        if (sectcurr == ((sectnum + 1) & 0377)) {
            sectnum++;
            cblklen = Bytesleft > Blklen ? Blklen : Bytesleft;
            if (putsec(secbuf, cblklen) == ERROR)
                return ERROR;
            if ((Bytesleft -= cblklen) < 0)
                Bytesleft = 0;
            sendchar = ACK;
        } else if (sectcurr == (sectnum & 0377)) {
            zperr("Received dup Sector");
            sendchar = ACK;
        } else if (sectcurr == WCEOT) {
            if (closeit())
                return ERROR;
            sendline(ACK);
            Lleft = 0;    /* Do read next time ... */
            return OK;
        } else if (sectcurr == ERROR) {
            return ERROR;
        } else {
            zperr("Sync Error");
            return ERROR;
        }
    }
}

/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 * @param rpn receive a pathname
 */
static int wcrxpn(char *rpn)
{
    int c;

#ifdef NFGVMIN
    readline(1);
#else
    purgeline();
#endif

et_tu:
    Firstsec = TRUE;
    Eofseen = FALSE;
    sendline(Crcflg ? WANTCRC : NAK);
    Lleft = 0;    /* Do read next time ... */
    while ((c = wcgetsec(rpn, 100)) != 0) {
        if (c == WCEOT) {
            zperr("Pathname fetch returned %d", c);
            sendline(ACK);
            Lleft = 0;    /* Do read next time ... */
            readline(1);
            goto et_tu;
        }
        return ERROR;
    }
    sendline(ACK);
    return OK;
}

static int wcreceive(int argc, char **argp)
{
    int c;

    if (Batch || argc == 0) {
        Crcflg = 1;
        if (Verbose != 0)
            fprintf(stderr, rbmsg, Progname, Nozmodem ? "sb" : "sz");
        c = tryz();
        if (c) {
            if (c == ZCOMPL)
                return OK;
            if (c == ERROR)
                goto fubar;
            c = rzfiles();
            if (c)
                goto fubar;
        } else {
            for (;;) {
                if (wcrxpn(secbuf) == ERROR)
                    goto fubar;
                if (secbuf[0] == 0)
                    return OK;
                if (procheader(secbuf) == ERROR)
                    goto fubar;
                if (wcrx() == ERROR)
                    goto fubar;
            }
        }
    } else {
        Bytesleft = DEFBYTL;
        Filemode = 0;
        modtime = 0;

        procheader("");
        strcpy(Pathname, *argp);
        checkpath(Pathname);

        fprintf(stderr, "\nrz: ready to receive %s\r\n", Pathname);
        if ((fout = fopen(Pathname, "w")) == NULL)
            return ERROR;
        if (wcrx() == ERROR)
            goto fubar;
    }
    return OK;
fubar:
    canit();
    if (Topipe && fout) {
        pclose(fout);
        return ERROR;
    }
    if (fout)
        fclose(fout);
    if (Restricted) {
        unlink(Pathname);
        fprintf(stderr, "\r\nrz: %s removed.\r\n", Pathname);
    }
    return ERROR;
}

int main(int argc, char *argv[])
{
    char *cp;
    int npats;
    char *virgin, **patts;
    int exitcode = 0;

    Rxtimeout = 100;

    setbuf(stderr, (char *)NULL);

    from_cu();
    chkinvok(virgin = argv[0]);   /* if called as [-]rzCOMMAND set flag */
    npats = 0;
    while (--argc) {
        cp = *++argv;
        if (*cp == '-') {
            while (*++cp) {
                switch (*cp) {
                case '\\':
                     cp[1] = toupper(cp[1]);
                     continue;
                case '+':
                    Lzmanag = ZMAPND;
                    break;
                case 'a':
                    Rxascii = TRUE;
                    break;
                case 'b':
                    Rxbinary = TRUE;
                    break;
                case 'c':
                    Crcflg = TRUE;
                    break;
                case 'D':
                    Nflag = TRUE;
                    break;
                case 'e':
                    Zctlesc = 1;
                    break;
                case 'p':
                    Lzmanag = ZMPROT;
                    break;
                case 'q':
                    Verbose = 0;
                    break;
                case 't':
                    if (--argc < 1) {
                        usage();
                    }
                    Rxtimeout = atoi(*++argv);
                    if (Rxtimeout < 10 || Rxtimeout > 1000)
                        usage();
                    break;
                case 'w':
                    if (--argc < 1) {
                        usage();
                    }
                    Zrwindow = atoi(*++argv);
                    break;
                case 'u':
                    MakeLCPathname = FALSE;
                    break;
                case 'v':
                    ++Verbose;
                    break;
                case 'y':
                    Rxclob = TRUE;
                    break;
                default:
                    usage();
                }
            }
        } else if (!npats && argc > 0) {
            if (argv[0][0]) {
                npats = argc;
                patts = argv;
            }
        }
    }
    if (npats > 1)
        usage();
    if (Batch && npats)
        usage();
    /* TODO only if asked to do so */
    if (Verbose) {
        if (freopen(LOGFILE, "a", stderr) == NULL) {
            printf("Can't open log file %s\n", LOGFILE);
            exit(0200);
        }
        setbuf(stderr, (char *)NULL);
        fprintf(stderr, "argv[0]=%s Progname=%s\n", virgin, Progname);
    }
    vfile("%s %s\n", Progname, VERSION);
    mode(1);
    if (signal(SIGINT, bibi) == SIG_IGN) {
        signal(SIGINT, SIG_IGN);
        signal(SIGKILL, SIG_IGN);
    } else {
        signal(SIGINT, bibi);
        signal(SIGKILL, bibi);
    }
    signal(SIGTERM, bibi);
    if (wcreceive(npats, patts) == ERROR) {
        exitcode = 0200;
        canit();
    }
    mode(0);
    vfile("exitcode = %d\n", exitcode);
    if (exitcode && !Zmodem)    /* bellow again with all thy might. */
        canit();
    if (exitcode)
        cucheck();
    if (Verbose)
        putc('\n', stderr);
    exit(exitcode ? exitcode : 0);
}

#ifdef MD
/*
 *  Directory-creating routines from Public Domain TAR by John Gilmore
 */

/*
 * After a file/link/symlink/dir creation has failed, see if
 * it's because some required directory was not present, and if
 * so, create all required dirs.
 */
static int make_dirs(char *pathname)
{
    char *p;       /* Points into path */
    int madeone = 0;        /* Did we do anything yet? */
    int save_errno = errno;     /* Remember caller's errno */
    char *strchr();

    if (errno != ENOENT)
        return 0;       /* Not our problem */

    for (p = strchr(pathname, '/'); p != NULL; p = strchr(p+1, '/')) {
        /* Avoid mkdir of empty string, if leading or double '/' */
        if (p == pathname || p[-1] == '/')
            continue;
        /* Avoid mkdir where last part of path is '.' */
        if (p[-1] == '.' && (p == pathname+1 || p[-2] == '/'))
            continue;
        *p = 0;             /* Truncate the path there */
        if (!makedir(pathname, 0777)) {    /* Try to create it as a dir */
            vfile("Made directory %s\n", pathname);
            madeone++;      /* Remember if we made one */
            *p = '/';
            continue;
        }
        *p = '/';
        if (errno == EEXIST)        /* Directory already exists */
            continue;
        /*
         * Some other error in the makedir.  We return to the caller.
         */
        break;
    }
    errno = save_errno;     /* Restore caller's errno */
    return madeone;         /* Tell them to retry if we made one */
}

#if (MD != 2)
/*
 * Make a directory.  Compatible with the mkdir() system call on 4.2BSD.
 */
static int makedir(char *dpath, int dmode)
{
    int cpid, status;
    struct stat statbuf;

    if (stat(dpath, &statbuf) == 0) {
        errno = EEXIST;     /* Stat worked, so it already exists */
        return -1;
    }

    /* If stat fails for a reason other than non-existence, return error */
    if (errno != ENOENT)
        return -1;

    switch (cpid = fork()) {

    case -1:            /* Error in fork() */
        return -1;      /* Errno is set already */

    case 0:             /* Child process */
        /*
         * Cheap hack to set mode of new directory.  Since this
         * child process is going away anyway, we zap its umask.
         * FIXME, this won't suffice to set SUID, SGID, etc. on this
         * directory.  Does anybody care?
         */
        status = umask(0);  /* Get current umask */
        status = umask(status | (0777 & ~dmode)); /* Set for mkdir */
        execl("/bin/mkdir", "mkdir", dpath, (char *)0);
        _exit(-1);      /* Can't exec /bin/mkdir */

    default:            /* Parent process */
        while (cpid != wait(&status)) {
            /* Wait for kid to finish */
        }
    }

    if (WIFSIGNALED(status) || WEXITSTATUS(status) != 0) {
        errno = EIO;        /* We don't know why, but */
        return -1;      /* /bin/mkdir failed */
    }

    return 0;
}
#endif /* MD != 2 */
#endif /* MD */
