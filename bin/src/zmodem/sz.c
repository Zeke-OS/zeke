#define VERSION "sz 2.12 05-29-88"
#define PUBDIR "/usr/spool/uucppublic"

/*
 ****************************************************************************
 *
 * sz.c By Chuck Forsberg,  Omen Technology INC
 *
 ****************************************************************************
 * A program for Unix to send files and commands to computers running
 *  Professional-YAM, PowerCom, YAM, IMP, or programs supporting Y/XMODEM.
 *
 *  Sz uses buffered I/O to greatly reduce CPU time compared to UMODEM.
 *
 *  USG UNIX (3.0) ioctl conventions courtesy Jeff Martin
 *
 *  2.1x hacks to avoid VMS fseek() bogosity, allow input from pipe
 *     -DBADSEEK -DTXBSIZE=32768
 *  2.x has mods for VMS flavor
 *
 * 1.34 implements tx backchannel garbage count and ZCRCW after ZRPOS
 * in accordance with the 7-31-87 ZMODEM Protocol Description
 */

#define LOGFILE "/tmp/szlog"

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "zmodem.h"

unsigned Txwindow;  /* Control the size of the transmitted window */
unsigned Txwspac;   /* Spacing between zcrcq requests */
unsigned Txwcnt;    /* Counter used to space ack requests */
long Lrxpos;        /* Receiver's last reported offset */
int errors;

int Filesleft;
long Totalleft;

/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
#ifdef READCHECK
char Myattn[] = { 0 };
#else
#ifdef USG
char Myattn[] = { 03, 0336, 0 };
#else
char Myattn[] = { 0 };
#endif
#endif

FILE *in;

int Canseek = 1;    /* 1: Can seek 0: only rewind -1: neither (pipe) */

char txbuf[1024];
long vpos;          /* Number of bytes read from file */

char Lastrx;
char Crcflg;
int Modem2;         /* XMODEM Protocol - don't send pathnames */
int Restricted;     /* restricted; no /.. or ../ in filenames */
int Ascii;          /* Add CR's for brain damaged programs */
int Fullname;       /* transmit full pathname */
int Unlinkafter;    /* Unlink file after it is sent */
int Dottoslash;     /* Change foo.bar.baz to foo/bar/baz */
int firstsec;
int errcnt;         /* number of files unreadable */
int blklen = 128;   /* length of transmitted records */
int Optiong;        /* Let it rip no wait for sector ACK's */
int Eofseen;        /* EOF seen on input set by zfilbuf */
int BEofseen;       /* EOF seen on input set by fooseek */
int Totsecs;        /* total number of sectors this file */
int Filcnt;         /* count of number of files opened */
int Lfseen;
unsigned Rxbuflen = 16384;  /* Receiver's max buffer length */
int Tframlen;       /* Override for tx frame length */
int blkopt;         /* Override value for zmodem blklen */
int Rxflags;
long bytcnt;
int Wantfcs32 = TRUE;   /* want to send 32 bit FCS */
char Lzconv;    /* Local ZMODEM file conversion request */
char Lzmanag;   /* Local ZMODEM file management request */
int Lskipnocor;
char Lztrans;
char zconv;     /* ZMODEM file conversion request */
char zmanag;        /* ZMODEM file management request */
char ztrans;        /* ZMODEM file transport request */
int Command;        /* Send a command, then exit. */
char *Cmdstr;       /* Pointer to the command string */
int Cmdtries = 11;
int Cmdack1;        /* Rx ACKs command, then do it */
int Exitcode;
int Test;       /* 1= Force receiver to send Attn, etc with qbf. */
                /* 2= Character transparency test */
const char qbf[] = "The quick brown fox jumped over the lazy dog's back 1234567890\r\n";
long Lastsync;      /* Last offset to which we got a ZRPOS */
int Beenhereb4;     /* How many times we've been ZRPOS'd same place */

jmp_buf tohere;     /* For the interrupt on RX timeout */
jmp_buf intrjmp;    /* For the interrupt on RX CAN */

int Zctlesc;    /* Encode control characters */
int Nozmodem;   /* If invoked as "sb" */
char *Progname = "sz";
int Zrwindow = 1400;    /* RX window size (controls garbage count) */

/* called by signal interrupt or terminate to clean things up */
void bibi(int n)
{
    canit();
    fflush(stdout);
    mode(0);
    fprintf(stderr, "sz: caught signal %d; exiting\n", n);
    if (n == SIGQUIT)
        abort();
    if (n == 99)
        fprintf(stderr, "mode(2) in rbsb.c not implemented!!\n");
    cucheck();
    exit(128 + n);
}
/* Called when ZMODEM gets an interrupt (^X) */
void onintr(int sig)
{
    signal(SIGINT, SIG_IGN);
    longjmp(intrjmp, -1);
}

/* Send send-init information */
int sendzsinit(void)
{
    int c;

    if (Myattn[0] == '\0' && (!Zctlesc || (Rxflags & TESCCTL)))
        return OK;
    errors = 0;
    for (;;) {
        stohdr(0L);
        if (Zctlesc) {
            Txhdr[ZF0] |= TESCCTL;
            zshhdr(ZSINIT, Txhdr);
        } else {
            zsbhdr(ZSINIT, Txhdr);
        }
        zsdata(Myattn, 1 + strlen(Myattn), ZCRCW);
        c = zgethdr(Rxhdr, 1);
        switch (c) {
        case ZCAN:
            return ERROR;
        case ZACK:
            return OK;
        default:
            if (++errors > 19)
                return ERROR;
            continue;
        }
    }
}

/*
 * Get the receiver's init parameters
 */
int getzrxinit(void)
{
    int n;
    struct stat f;

    for (n = 10; --n >= 0;) {

        switch (zgethdr(Rxhdr, 1)) {
        case ZCHALLENGE:    /* Echo receiver's challenge numbr */
            stohdr(Rxpos);
            zshhdr(ZACK, Txhdr);
            continue;
        case ZCOMMAND:      /* They didn't see out ZRQINIT */
            stohdr(0L);
            zshhdr(ZRQINIT, Txhdr);
            continue;
        case ZRINIT:
            Rxflags = 0377 & Rxhdr[ZF0];
            Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));
            Zctlesc |= Rxflags & TESCCTL;
            Rxbuflen = (0377 & Rxhdr[ZP0]) + ((0377 & Rxhdr[ZP1]) << 8);
            if (!(Rxflags & CANFDX))
                Txwindow = 0;
            vfile("Rxbuflen=%d Tframlen=%d", Rxbuflen, Tframlen);
            if (!Fromcu)
                signal(SIGINT, SIG_IGN);
#ifdef MODE2OK
            mode(2);    /* Set cbreak, XON/XOFF, etc. */
#endif
#ifndef READCHECK
#ifndef USG
            /* Use 1024 byte frames if no sample/interrupt */
            if (Rxbuflen < 32 || Rxbuflen > 1024) {
                Rxbuflen = 1024;
                vfile("Rxbuflen=%d", Rxbuflen);
            }
#endif
#endif
            /* Override to force shorter frame length */
            if (Rxbuflen && (Rxbuflen > Tframlen) && (Tframlen >= 32))
                Rxbuflen = Tframlen;
            if (!Rxbuflen && (Tframlen >= 32) && (Tframlen <= 1024))
                Rxbuflen = Tframlen;
            vfile("Rxbuflen=%d", Rxbuflen);

            /* If using a pipe for testing set lower buf len */
            fstat(iofd, &f);
            if ((f.st_mode & S_IFMT) != S_IFCHR) {
                Rxbuflen = 1024;
            }
            /*
             * If input is not a regular file, force ACK's to
             *  prevent running beyond the buffer limits
             */
            if (!Command) {
                fstat(fileno(in), &f);
                if ((f.st_mode & S_IFMT) != S_IFREG) {
                    Canseek = -1;
                    return ERROR;
                }
            }
            /* Set initial subpacket length */
            if (blklen < 1024) {    /* Command line override? */
                if (Baudrate > 300)
                    blklen = 256;
                if (Baudrate > 1200)
                    blklen = 512;
                if (Baudrate > 2400)
                    blklen = 1024;
            }
            if (Rxbuflen && blklen > Rxbuflen)
                blklen = Rxbuflen;
            if (blkopt && blklen > blkopt)
                blklen = blkopt;
            vfile("Rxbuflen=%d blklen=%d", Rxbuflen, blklen);
            vfile("Txwindow = %u Txwspac = %d", Txwindow, Txwspac);

            return sendzsinit();
        case ZCAN:
        case TIMEOUT:
            return ERROR;
        case ZRQINIT:
            if (Rxhdr[ZF0] == ZCOMMAND)
                continue;
        default:
            zshhdr(ZNAK, Txhdr);
            continue;
        }
    }
    return ERROR;
}

int getnak(void)
{
    int firstch;

    Lastrx = 0;
    for (;;) {
        switch (firstch = readline(800)) {
        case ZPAD:
            if (getzrxinit())
                return ERROR;
            Ascii = 0;  /* Receiver does the conversion */
            return FALSE;
        case TIMEOUT:
            zperr("Timeout on pathname");
            return TRUE;
        case WANTG:
#ifdef MODE2OK
            mode(2);    /* Set cbreak, XON/XOFF, etc. */
#endif
            Optiong = TRUE;
            blklen = 1024;
        case WANTCRC:
            Crcflg = TRUE;
        case NAK:
            return FALSE;
        case CAN:
            if ((firstch = readline(20)) == CAN && Lastrx == CAN)
                return TRUE;
        default:
            break;
        }
        Lastrx = firstch;
    }
}

/**
 * @param cseclen data length of this sector to send
 */
int wcputsec(char *buf, int sectnum, int cseclen)
{
    int checksum, wcj, firstch, attempts;
    unsigned oldcrc;
    char *cp;

    firstch = 0;  /* part of logic to detect CAN CAN */

    if (Verbose > 2)
        fprintf(stderr, "Sector %3d %2dk\n", Totsecs, Totsecs / 8);
    else if (Verbose > 1)
        fprintf(stderr, "\rSector %3d %2dk ", Totsecs, Totsecs / 8);

    for (attempts = 0; attempts <= RETRYMAX; attempts++) {
        Lastrx = firstch;
        sendline(cseclen == 1024 ? STX : SOH);
        sendline(sectnum);
        sendline(-sectnum - 1);
        oldcrc = checksum = 0;
        for (wcj = cseclen, cp = buf; --wcj >= 0;) {
            sendline(*cp);
            oldcrc = updcrc((0377 & *cp), oldcrc);
            checksum += *cp++;
        }
        if (Crcflg) {
            oldcrc = updcrc(0, updcrc(0, oldcrc));
            sendline((int)oldcrc >> 8);
            sendline((int)oldcrc);
        } else {
            sendline(checksum);
        }

        if (Optiong) {
            firstsec = FALSE;
            return OK;
        }
        firstch = readline(Rxtimeout);
gotnak:
        switch (firstch) {
        case CAN:
            if (Lastrx == CAN) {
cancan:
                zperr("Cancelled");
                return ERROR;
            }
            break;
        case TIMEOUT:
            zperr("Timeout on sector ACK"); continue;
        case WANTCRC:
            if (firstsec)
                Crcflg = TRUE;
        case NAK:
            zperr("NAK on sector"); continue;
        case ACK:
            firstsec = FALSE;
            Totsecs += (cseclen >> 7);
            return OK;
        case ERROR:
            zperr("Got burst for sector ACK"); break;
        default:
            zperr("Got %02x for sector ACK", firstch); break;
        }
        for (;;) {
            Lastrx = firstch;
            if ((firstch = readline(Rxtimeout)) == TIMEOUT)
                break;
            if (firstch == NAK || firstch == WANTCRC)
                goto gotnak;
            if (firstch == CAN && Lastrx == CAN)
                goto cancan;
        }
    }
    zperr("Retry Count Exceeded");
    return ERROR;
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int getinsync(int flag)
{
    int c;

    for (;;) {
        if (Test) {
            printf("\r\n\n\n***** Signal Caught *****\r\n");
            Rxpos = 0;
            c = ZRPOS;
        } else
            c = zgethdr(Rxhdr, 0);
        switch (c) {
        case ZCAN:
        case ZABORT:
        case ZFIN:
        case TIMEOUT:
            return ERROR;
        case ZRPOS:
            /* ************************************* */
            /*  If sending to a buffered modem, you  */
            /*   might send a break at this point to */
            /*   dump the modem's buffer.        */
            clearerr(in);   /* In case file EOF seen */
            if (fseek(in, Rxpos, 0))
                return ERROR;
            Eofseen = 0;
            bytcnt = Lrxpos = Txpos = Rxpos;
            if (Lastsync == Rxpos) {
                if (++Beenhereb4 > 4)
                    if (blklen > 32)
                        blklen /= 2;
            }
            Lastsync = Rxpos;
            return c;
        case ZACK:
            Lrxpos = Rxpos;
            if (flag || Txpos == Rxpos)
                return ZACK;
            continue;
        case ZRINIT:
        case ZSKIP:
            fclose(in);
            return c;
        case ERROR:
        default:
            zsbhdr(ZNAK, Txhdr);
            continue;
        }
    }
}

/* Fill buffer with blklen chars */
int zfilbuf(void)
{
    int n;

    n = fread(txbuf, 1, blklen, in);
    if (n < blklen)
        Eofseen = 1;
    return n;
}

/* Send the data in the file */
int zsendfdata(void)
{
    int c, e, n;
    int newcnt;
    long tcount = 0;
    int junkcount;      /* Counts garbage chars received by TX */
    static int tleft = 6;   /* Counter for test mode */

    Lrxpos = 0;
    junkcount = 0;
    Beenhereb4 = FALSE;
somemore:
    if (setjmp(intrjmp)) {
waitack:
        junkcount = 0;
        c = getinsync(0);
gotack:
        switch (c) {
        default:
        case ZCAN:
            fclose(in);
            return ERROR;
        case ZSKIP:
            fclose(in);
            return c;
        case ZACK:
        case ZRPOS:
            break;
        case ZRINIT:
            return OK;
        }
#ifdef READCHECK
        /*
         * If the reverse channel can be tested for data,
         *  this logic may be used to detect error packets
         *  sent by the receiver, in place of setjmp/longjmp
         *  rdchk(fdes) returns non 0 if a character is available
         */
        while (rdchk(iofd)) {
            switch (readline(1)) {
            case CAN:
            case ZPAD:
                c = getinsync(1);
                goto gotack;
            case XOFF:      /* Wait a while for an XON */
            case XOFF | 0200:
                readline(100);
            }
        }
#endif
    }

    if (!Fromcu)
        signal(SIGINT, onintr);
    newcnt = Rxbuflen;
    Txwcnt = 0;
    stohdr(Txpos);
    zsbhdr(ZDATA, Txhdr);

    /*
     * Special testing mode.  This should force receiver to Attn,ZRPOS
     *  many times.  Each time the signal should be caught, causing the
     *  file to be started over from the beginning.
     */
    if (Test) {
        if (--tleft)
            while (tcount < 20000) {
                printf(qbf);
                fflush(stdout);
                tcount += strlen(qbf);
#ifdef READCHECK
                while (rdchk(iofd)) {
                    switch (readline(1)) {
                    case CAN:
                    case ZPAD:
                        tcflush(iofd, TCOFLUSH);
                        goto waitack;
                    case XOFF:  /* Wait for XON */
                    case XOFF | 0200:
                        readline(100);
                    }
                }
#endif
            }
        signal(SIGINT, SIG_IGN);
        canit();
        sleep(3);
        purgeline();
        mode(0);
        printf("\nsz: Tcount = %ld\n", tcount);
        if (tleft) {
            printf("ERROR: Interrupts Not Caught\n");
            exit(1);
        }
        exit(0);
    }

    do {
        n = zfilbuf();
        if (Eofseen)
            e = ZCRCE;
        else if (junkcount > 3)
            e = ZCRCW;
        else if (bytcnt == Lastsync)
            e = ZCRCW;
        else if (Rxbuflen && (newcnt -= n) <= 0)
            e = ZCRCW;
        else if (Txwindow && (Txwcnt += n) >= Txwspac) {
            Txwcnt = 0;
            e = ZCRCQ;
        } else {
            e = ZCRCG;
        }
        if (Verbose > 1) {
            fprintf(stderr, "\r%7ld ZMODEM%s    ",
                    Txpos, Crc32t ? " CRC-32" : "");
        }
        zsdata(txbuf, n, e);
        bytcnt = Txpos += n;
        if (e == ZCRCW)
            goto waitack;
#ifdef READCHECK
        /*
         * If the reverse channel can be tested for data,
         *  this logic may be used to detect error packets
         *  sent by the receiver, in place of setjmp/longjmp
         *  rdchk(fdes) returns non 0 if a character is available
         */
        fflush(stdout);
        while (rdchk(iofd)) {
            switch (readline(1)) {
            case CAN:
            case ZPAD:
                c = getinsync(1);
                if (c == ZACK)
                    break;

                tcflush(iofd, TCOFLUSH);
                /* zcrce - dinna wanna starta ping-pong game */
                zsdata(txbuf, 0, ZCRCE);
                goto gotack;
            case XOFF:      /* Wait a while for an XON */
            case XOFF|0200:
                readline(100);
            default:
                ++junkcount;
            }
        }
#endif  /* READCHECK */
        if (Txwindow) {
            while ((tcount = Txpos - Lrxpos) >= Txwindow) {
                vfile("%ld window >= %u", tcount, Txwindow);
                if (e != ZCRCQ)
                    zsdata(txbuf, 0, e = ZCRCQ);
                c = getinsync(1);
                if (c != ZACK) {
                    tcflush(iofd, TCOFLUSH);
                    zsdata(txbuf, 0, ZCRCE);
                    goto gotack;
                }
            }
            vfile("window = %ld", tcount);
        }
    } while (!Eofseen);
    if (!Fromcu)
        signal(SIGINT, SIG_IGN);

    for (;;) {
        stohdr(Txpos);
        zsbhdr(ZEOF, Txhdr);
        switch (getinsync(0)) {
        case ZACK:
            continue;
        case ZRPOS:
            goto somemore;
        case ZRINIT:
            return OK;
        case ZSKIP:
            fclose(in);
            return c;
        default:
            fclose(in);
            return ERROR;
        }
    }
}

/* Send file name and related info */
int zsendfile(char *buf, int blen)
{
    int c;
    long crc;

    for (;;) {
        Txhdr[ZF0] = Lzconv;    /* file conversion request */
        Txhdr[ZF1] = Lzmanag;   /* file management request */
        if (Lskipnocor)
            Txhdr[ZF1] |= ZMSKNOLOC;
        Txhdr[ZF2] = Lztrans;   /* file transport request */
        Txhdr[ZF3] = 0;
        zsbhdr(ZFILE, Txhdr);
        zsdata(buf, blen, ZCRCW);
again:
        c = zgethdr(Rxhdr, 1);
        switch (c) {
        case ZRINIT:
            while ((c = readline(50)) > 0)
                if (c == ZPAD) {
                    goto again;
                }
            /* **** FALL THRU TO **** */
        default:
            continue;
        case ZCAN:
        case TIMEOUT:
        case ZABORT:
        case ZFIN:
            return ERROR;
        case ZCRC:
            crc = 0xFFFFFFFFL;
            if (Canseek >= 0) {
                while (((c = getc(in)) != EOF) && --Rxpos) {
                    crc = UPDC32(c, crc);
                }
                crc = ~crc;
                clearerr(in);   /* Clear EOF */
                fseek(in, 0L, 0);
            }
            stohdr(crc);
            zsbhdr(ZCRC, Txhdr);
            goto again;
        case ZSKIP:
            fclose(in);
            return c;
        case ZRPOS:
            /*
             * Suppress zcrcw request otherwise triggered by
             * lastyunc==bytcnt
             */
            if (Rxpos && fseek(in, Rxpos, 0))
                return ERROR;
            Lastsync = (bytcnt = Txpos = Rxpos) - 1;
            return zsendfdata();
        }
    }
}

/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
int wctxpn(char *name)
{
    char *p, *q;
    char name2[PATHLEN];
    struct stat f;

    if (Modem2) {
        if ((in != stdin) && *name && fstat(fileno(in), &f) != -1) {
            fprintf(stderr, "Sending %s, %ld blocks: ",
              name, f.st_size >> 7);
        }
        fprintf(stderr, "Give your local XMODEM receive command now.\r\n");
        return OK;
    }
    zperr("Awaiting pathname nak for %s", *name ? name : "<END>");
    if (!Zmodem)
        if (getnak())
            return ERROR;

    q = NULL;
    if (Dottoslash) {       /* change . to . */
        for (p = name; *p; ++p) {
            if (*p == '/')
                q = p;
            else if (*p == '.')
                *(q = p) = '/';
        }
        if (q && strlen(++q) > 8) { /* If name>8 chars */
            q += 8;         /*   make it .ext */
            strcpy(name2, q);   /* save excess of name */
            *q = '.';
            strcpy(++q, name2); /* add it back */
        }
    }

    for (p = name, q = txbuf ; *p;) {
        if ((*q++ = *p++) == '/' && !Fullname)
            q = txbuf;
    }
    *q++ = 0;
    p = q;
    while (q < (txbuf + 1024)) {
        *q++ = 0;
    }
    if (!Ascii && (in != stdin) && *name && fstat(fileno(in), &f) != -1)
        sprintf(p, "%lu %lo %o 0 %d %ld", f.st_size, f.st_mtime,
          f.st_mode, Filesleft, Totalleft);
    Totalleft -= f.st_size;
    if (--Filesleft <= 0)
        Totalleft = 0;
    if (Totalleft < 0)
        Totalleft = 0;

    /* force 1k blocks if name won't fit in 128 byte block */
    if (txbuf[125])
        blklen = 1024;
    else {      /* A little goodie for IMP/KMD */
        txbuf[127] = (f.st_size + 127) >> 7;
        txbuf[126] = (f.st_size + 127) >> 15;
    }
    if (Zmodem)
        return zsendfile(txbuf, 1 + strlen(p) + (p - txbuf));
    if (wcputsec(txbuf, 0, 128) == ERROR)
        return ERROR;
    return OK;
}

/* fill buf with count chars padding with ^Z for CPM */
int filbuf(char *buf, int count)
{
    int c, m;

    if (!Ascii) {
        m = read(fileno(in), buf, count);
        if (m <= 0)
            return 0;
        while (m < count) {
            buf[m++] = 032;
        }
        return count;
    }
    m = count;
    if (Lfseen) {
        *buf++ = 012;
        --m;
        Lfseen = 0;
    }
    while ((c = getc(in)) != EOF) {
        if (c == 012) {
            *buf++ = 015;
            if (--m == 0) {
                Lfseen = TRUE;
                break;
            }
        }
        *buf++ = c;
        if (--m == 0)
            break;
    }
    if (m == count)
        return 0;
    else {
        while (--m >= 0) {
            *buf++ = CPMEOF;
        }
    }
    return count;
}

int wctx(int flen)
{
    int thisblklen;
    int sectnum, attempts, firstch;
    long charssent;

    charssent = 0;
    firstsec = TRUE;
    thisblklen = blklen;
    vfile("wctx:file length=%ld", flen);

    while ((firstch = readline(Rxtimeout)) != NAK && firstch != WANTCRC
      && firstch != WANTG && firstch != TIMEOUT && firstch != CAN)
        ;
    if (firstch == CAN) {
        zperr("Receiver CANcelled");
        return ERROR;
    }
    if (firstch == WANTCRC)
        Crcflg = TRUE;
    if (firstch == WANTG)
        Crcflg = TRUE;
    sectnum = 0;
    for (;;) {
        if (flen <= (charssent + 896L))
            thisblklen = 128;
        if (!filbuf(txbuf, thisblklen))
            break;
        if (wcputsec(txbuf, ++sectnum, thisblklen) == ERROR)
            return ERROR;
        charssent += thisblklen;
    }
    fclose(in);
    attempts = 0;
    do {
        purgeline();
        sendline(EOT);
        fflush(stdout);
        ++attempts;
    }
        while ((firstch = (readline(Rxtimeout)) != ACK) && attempts < RETRYMAX);
    if (attempts == RETRYMAX) {
        zperr("No ACK on EOT");
        return ERROR;
    } else {
        return OK;
    }
}

int wcs(char *oname)
{
    int c;
    char *p;
    struct stat f;
    char name[PATHLEN];

    strcpy(name, oname);

    /* restrict pathnames to current tree or uucppublic */
    if (Restricted && (substr(name, "../") ||
        (name[0] == '/' && strncmp(name, PUBDIR, strlen(PUBDIR))))) {
        canit();
        fprintf(stderr, "\r\nsz:\tSecurity Violation\r\n");
        return ERROR;
    }

    if (!strcmp(oname, "-")) {
        if ((p = getenv("ONAME")) && *p) {
            strcpy(name, p);
        } else {
            sprintf(name, "s%d.sz", getpid());
        }
        in = stdin;
    } else if ((in = fopen(oname, "r")) == NULL) {
        ++errcnt;
        return OK;  /* pass over it, there may be others */
    }
    BEofseen = Eofseen = 0;
    vpos = 0;
    /* Check for directory or block special files */
    fstat(fileno(in), &f);
    c = f.st_mode & S_IFMT;
    if (c == S_IFDIR || c == S_IFBLK) {
        fclose(in);
        return OK;
    }

    ++Filcnt;
    switch (wctxpn(name)) {
    case ERROR:
        return ERROR;
    case ZSKIP:
        return OK;
    }
    if (!Zmodem && wctx(f.st_size) == ERROR)
        return ERROR;
    if (Unlinkafter)
        unlink(oname);
    return 0;
}

void saybibi(void)
{
    for (;;) {
        stohdr(0L);     /* CAF Was zsbhdr - minor change */
        zshhdr(ZFIN, Txhdr);    /*  to make debugging easier */
        switch (zgethdr(Rxhdr, 0)) {
        case ZFIN:
            sendline('O');
            sendline('O');
            flushmo();
        case ZCAN:
        case TIMEOUT:
            return;
        }
    }
}

/* Say "bibi" to the receiver, try to do it cleanly */
/* Send command and related info */
int zsendcmd(char *buf, int blen)
{
    int c;
    long cmdnum;

    cmdnum = getpid();
    errors = 0;
    for (;;) {
        stohdr(cmdnum);
        Txhdr[ZF0] = Cmdack1;
        zsbhdr(ZCOMMAND, Txhdr);
        zsdata(buf, blen, ZCRCW);
listen:
        Rxtimeout = 100;        /* Ten second wait for resp. */
        c = zgethdr(Rxhdr, 1);

        switch (c) {
        case ZRINIT:
            goto listen;    /* CAF 8-21-87 */
        case ERROR:
        case TIMEOUT:
            if (++errors > Cmdtries)
                return ERROR;
            continue;
        case ZCAN:
        case ZABORT:
        case ZFIN:
        case ZSKIP:
        case ZRPOS:
            return ERROR;
        default:
            if (++errors > 20)
                return ERROR;
            continue;
        case ZCOMPL:
            Exitcode = Rxpos;
            saybibi();
            return OK;
        case ZRQINIT:
            vfile("******** RZ *******");
            system("rz");
            vfile("******** SZ *******");
            goto listen;
        }
    }
}

int wcsend(int argc, char *argp[])
{
    int n;

    Crcflg = FALSE;
    firstsec = TRUE;
    bytcnt = -1;
    for (n = 0; n < argc; ++n) {
        Totsecs = 0;
        if (wcs(argp[n]) == ERROR)
            return ERROR;
    }
    Totsecs = 0;
    if (Filcnt == 0) {    /* bitch if we couldn't open ANY files */
        if (!Modem2) {
            Command = TRUE;
            Cmdstr = "echo \"sz: Can't open any requested files\"";
            if (getnak()) {
                Exitcode = 0200;
                canit();
            }
            if (!Zmodem) {
                canit();
            } else if (zsendcmd(Cmdstr, 1 + strlen(Cmdstr))) {
                Exitcode = 0200; canit();
            }
            Exitcode = 1;
            return OK;
        }
        canit();
        fprintf(stderr, "\r\nCan't open any requested files.\r\n");
        return ERROR;
    }
    if (Zmodem) {
        saybibi();
    } else if (!Modem2) {
        wctxpn("");
    }
    return OK;
}

/*
 * If called as sb use YMODEM protocol
 */
void chkinvok(char *s)
{
    char *p;

    p = s;
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
    if (s[0] == 's' && s[1] == 'b') {
        Nozmodem = TRUE;
        blklen = 1024;
    }
    if (s[0] == 's' && s[1] == 'x') {
        Modem2 = TRUE;
    }
}

void countem(int argc, char **argv)
{
    int c;
    struct stat f;

    for (Totalleft = 0, Filesleft = 0; --argc >= 0; ++argv) {
        f.st_size = -1;
        if (Verbose > 2) {
            fprintf(stderr, "\nCountem: %03d %s ", argc, *argv);
            fflush(stderr);
        }
        if (access(*argv, 04) >= 0 && stat(*argv, &f) >= 0) {
            c = f.st_mode & S_IFMT;
            if (c != S_IFDIR && c != S_IFBLK) {
                ++Filesleft;
                Totalleft += f.st_size;
            }
        }
        if (Verbose > 2)
            fprintf(stderr, " %ld", f.st_size);
    }
    if (Verbose > 2)
        fprintf(stderr, "\ncountem: Total %d %ld\n",
          Filesleft, Totalleft);
}

void chartest(int m)
{
    int n;

    mode(m);
    printf("\r\n\nCharacter Transparency Test Mode %d\r\n", m);
    printf("If Pro-YAM/ZCOMM is not displaying ^M hit ALT-V NOW.\r\n");
    printf("Hit Enter.\021");
    fflush(stdout);
    readline(500);

    for (n = 0; n < 256; ++n) {
        if (!(n % 8))
            printf("\r\n");
        printf("%02x ", n);
        fflush(stdout);
        sendline(n);
        flushmo();
        printf("  ");
        fflush(stdout);
        if (n == 127) {
            printf("Hit Enter.\021");  fflush(stdout);
            readline(500);
            printf("\r\n");  fflush(stdout);
        }
    }
    printf("\021\r\nEnter Characters, echo is in hex.\r\n");
    printf("Hit SPACE or pause 40 seconds for exit.\r\n");

    while (n != TIMEOUT && n != ' ') {
        n = readline(400);
        printf("%02x\r\n", n);
        fflush(stdout);
    }
    printf("\r\nMode %d character transparency test ends.\r\n", m);
    fflush(stdout);
}

int usage(void)
{
    const char *babble[] = {
        "Send file(s) with ZMODEM/YMODEM/XMODEM Protocol",
        "   (Y) = Option applies to YMODEM only",
        "   (Z) = Option applies to ZMODEM only",
        "Usage: sz [-2+abdefkLlNnquvwYy] [-] file ...",
        "   sz [-2Ceqv] -c COMMAND",
        "   sb [-2adfkquv] [-] file ...",
        "   sx [-2akquv] [-] file",
        "   2 Use 2 stop bits",
        "   + Append to existing destination file (Z)",
        "   a (ASCII) change NL to CR/LF",
        "   b Binary file transfer override",
        "   c send COMMAND (Z)",
        "   d Change '.' to '/' in pathnames (Y/Z)",
        "   e Escape all control characters (Z)",
        "   f send Full pathname (Y/Z)",
        "   i send COMMAND, ack Immediately (Z)",
        "   k Send 1024 byte packets (Y)",
        "   L N Limit subpacket length to N bytes (Z)",
        "   l N Limit frame length to N bytes (l>=L) (Z)",
        "   n send file if source newer (Z)",
        "   N send file if source newer or longer (Z)",
        "   o Use 16 bit CRC instead of 32 bit CRC (Z)",
        "   p Protect existing destination file (Z)",
        "   r Resume/Recover interrupted file transfer (Z)",
        "   q Quiet (no progress reports)",
        "   u Unlink file after transmission",
        "   v Verbose - provide debugging information",
        "   w N Window is N bytes (Z)",
        "   Y Yes, overwrite existing file, skip if not present at rx (Z)",
        "   y Yes, overwrite existing file (Z)",
        "- as pathname sends standard input as sPID.sz or environment ONAME",
        ""
    };
    const char **pp;

    for (pp = babble; **pp; ++pp) {
        fprintf(stderr, "%s\n", *pp);
    }
    fprintf(stderr, "%s by Chuck Forsberg, Omen Technology INC\n",
            VERSION);
    fprintf(stderr, "\t\t\042The High Reliability Software\042\n");
    cucheck();
    exit(0);
}

int main(int argc, char *argv[])
{
    char *cp;
    int npats, dm;
    char **patts;
    static char xXbuf[BUFSIZ];

    if ((cp = getenv("ZNULLS")) && *cp)
        Znulls = atoi(cp);
    if ((cp = getenv("SHELL")) && (substr(cp, "rsh") || substr(cp, "rksh")))
        Restricted = TRUE;
    from_cu();
    chkinvok(argv[0]);

    Rxtimeout = 600;
    npats = 0;
    if (argc < 2)
        usage();
    setbuf(stdout, xXbuf);
    while (--argc) {
        cp = *++argv;
        if (*cp++ == '-' && *cp) {
            while (*cp) {
                switch (*cp++) {
                case '\\':
                     *cp = toupper(*cp);
                     continue;
                case '+':
                    Lzmanag = ZMAPND;
                    break;
                case '2':
                    Twostop = TRUE;
                    break;
                case 'a':
                    Lzconv = ZCNL;
                    Ascii = TRUE;
                    break;
                case 'b':
                    Lzconv = ZCBIN;
                    break;
                case 'C':
                    if (--argc < 1) {
                        usage();
                    }
                    Cmdtries = atoi(*++argv);
                    break;
                case 'i':
                    Cmdack1 = ZCACK1;
                    /* **** FALL THROUGH TO **** */
                case 'c':
                    if (--argc != 1) {
                        usage();
                    }
                    Command = TRUE;
                    Cmdstr = *++argv;
                    break;
                case 'd':
                    ++Dottoslash;
                    /* **** FALL THROUGH TO **** */
                case 'f':
                    Fullname = TRUE;
                    break;
                case 'e':
                    Zctlesc = 1;
                    break;
                case 'k':
                    blklen = 1024;
                    break;
                case 'L':
                    if (--argc < 1) {
                        usage();
                    }
                    blkopt = atoi(*++argv);
                    if (blkopt < 24 || blkopt > 1024)
                        usage();
                    break;
                case 'l':
                    if (--argc < 1) {
                        usage();
                    }
                    Tframlen = atoi(*++argv);
                    if (Tframlen < 32 || Tframlen > 1024)
                        usage();
                    break;
                case 'N':
                    Lzmanag = ZMNEWL;
                    break;
                case 'n':
                    Lzmanag = ZMNEW;
                    break;
                case 'o':
                    Wantfcs32 = FALSE;
                    break;
                case 'p':
                    Lzmanag = ZMPROT;
                    break;
                case 'r':
                    Lzconv = ZCRESUM;
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
                case 'T':
                    if (++Test > 1) {
                        chartest(1);
                        chartest(2);
                        mode(0);
                        exit(0);
                    }
                    break;
                case 'u':
                    ++Unlinkafter;
                    break;
                case 'v':
                    ++Verbose;
                    break;
                case 'w':
                    if (--argc < 1) {
                        usage();
                    }
                    Txwindow = atoi(*++argv);
                    if (Txwindow < 256)
                        Txwindow = 256;
                    Txwindow = (Txwindow / 64) * 64;
                    Txwspac = Txwindow / 4;
                    if (blkopt > Txwspac
                     || (!blkopt && Txwspac < 1024))
                        blkopt = Txwspac;
                    break;
                case 'X':
                    ++Modem2;
                    break;
                case 'Y':
                    Lskipnocor = TRUE;
                    /* **** FALLL THROUGH TO **** */
                case 'y':
                    Lzmanag = ZMCLOB;
                    break;
                default:
                    usage();
                }
            }
        } else if (!npats && argc > 0) {
            if (argv[0][0]) {
                npats = argc;
                patts = argv;
                if (!strcmp(*patts, "-"))
                    iofd = 1;
            }
        }
    }
    if (npats < 1 && !Command && !Test)
        usage();
    if (Verbose) {
        if (freopen(LOGFILE, "a", stderr) == NULL) {
            printf("Can't open log file %s\n", LOGFILE);
            exit(0200);
        }
        setbuf(stderr, (char *)NULL);
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
    if (!Fromcu) {
        signal(SIGQUIT, SIG_IGN);
    }
    signal(SIGTERM, bibi);

    if (!Modem2) {
        if (!Nozmodem) {
            printf("rz\r");
            fflush(stdout);
        }
        countem(npats, patts);
        if (!Nozmodem) {
            stohdr(0L);
            if (Command)
                Txhdr[ZF0] = ZCOMMAND;
            zshhdr(ZRQINIT, Txhdr);
        }
    }
    fflush(stdout);

    if (Command) {
        if (getzrxinit()) {
            Exitcode = 0200; canit();
        } else if (zsendcmd(Cmdstr, 1 + strlen(Cmdstr))) {
            Exitcode = 0200; canit();
        }
    } else if (wcsend(npats, patts) == ERROR) {
        Exitcode = 0200;
        canit();
    }
    fflush(stdout);
    mode(0);
    dm = ((errcnt != 0) | Exitcode);
    if (dm) {
        cucheck();
        exit(dm);
    }
    putc('\n', stderr);
    exit(0);
    /*NOTREACHED*/
}
