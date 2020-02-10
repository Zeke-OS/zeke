#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "zmodem.h"

/*
 * Max value for HOWMANY is 255.
 *   A larger value reduces system overhead but may evoke kernel bugs.
 *   133 corresponds to an XMODEM/CRC sector
 */
#define HOWMANY 133

int iofd;               /*!< File descriptor for ioctls & reads */
char linbuf[255];
int Lleft;              /* number of characters in linbuf */
int Readnum = HOWMANY;  /* Number of bytes to ask for in read() from modem */
int Fromcu;             /* Were called from cu or yam */
int Twostop;            /* Use two stop bits */

static void set_tty_read_timeout(int timeout, struct termios * orig)
{
    struct termios termios;

    tcgetattr(iofd, &termios);
    memcpy(orig, &termios, sizeof(struct termios));
    termios.c_lflag &= ~ICANON; /* Set non-canonical mode */
    termios.c_cc[VTIME] = timeout; /* Set timeout of 10.0 seconds */
    tcsetattr(iofd, TCSANOW, &termios);
}

/**
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM or YMODEM-g
 *  1: save old tty stat, set raw mode
 *  0: restore original tty mode
 */
int mode(int n)
{
    static struct termios oldtty;
    static struct termios tty;
    static int did0 = FALSE;

    vfile("mode:%d", n);
    switch (n) {
    case 2:     /* Un-raw mode used by sz, sb when -g detected */
        if (!did0)
            (void) tcgetattr(iofd, &oldtty);
        tty = oldtty;

        tty.c_iflag = BRKINT|IXON;

        tty.c_oflag = 0;    /* Transparent output */

        tty.c_cflag &= ~PARENB; /* Disable parity */
        tty.c_cflag |= CS8; /* Set character size = 8 */
        if (Twostop)
            tty.c_cflag |= CSTOPB;  /* Set two stop bits */


        tty.c_lflag = ISIG;
        tty.c_cc[VINTR] = Zmodem ? 03 : 030;  /* Interrupt char */
        tty.c_cc[VQUIT] = -1;           /* Quit char */
        tty.c_cc[VMIN] = 3;  /* This many chars satisfies reads */
        tty.c_cc[VTIME] = 1;    /* or in this many tenths of seconds */

        (void) tcsetattr(iofd, TCSANOW, &tty);
        did0 = TRUE;
        return OK;
    case 1:
    case 3:
        if (!did0)
            (void) tcgetattr(iofd, &oldtty);
        tty = oldtty;

        tty.c_iflag = n == 3 ? (IGNBRK | IXOFF) : IGNBRK;

         /* No echo, crlf mapping, INTR, QUIT, delays, no erase/kill */
        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

        tty.c_oflag = 0;    /* Transparent output */

        tty.c_cflag &= ~PARENB; /* Same baud rate, disable parity */
        tty.c_cflag |= CS8; /* Set character size = 8 */
        if (Twostop)
            tty.c_cflag |= CSTOPB;  /* Set two stop bits */
        tty.c_cc[VMIN] = HOWMANY; /* This many chars satisfies reads */
        tty.c_cc[VTIME] = 1;    /* or in this many tenths of seconds */
        (void) tcsetattr(iofd, TCSANOW, &tty);
        did0 = TRUE;
        Baudrate = cfgetospeed(&tty);
        return OK;
    case 0:
        if (!did0)
            return ERROR;
        /* Wait for output to drain, flush input queue, restore
         * modes and restart output.
         */
        (void)tcsetattr(iofd, TCSAFLUSH, &oldtty);
        /* TODO Enable the following code when tcflow is implemented. */
#if 0
        (void)tcflow(iofd, TCOON);
#endif
        return OK;
    default:
        return ERROR;
    }
}

void sendbrk(void)
{
    tcsendbreak(iofd, 1);
}

/*
 * Purge the modem input queue of all characters
 */
void purgeline(void)
{
    Lleft = 0;
    tcflush(iofd, TCIFLUSH);
}

/*
 * This version of readline is reasoably well suited for
 * reading many characters.
 *  (except, currently, for the Regulus version!)
 *
 * timeout is in tenths of seconds
 */
int readline(int timeout)
{
    static char *cdq;   /* pointer for removing chars from linbuf */
    struct termios orig_termios;

    if (--Lleft >= 0) {
        if (Verbose > 8) {
            fprintf(stderr, "%02x ", *cdq & 0377);
        }
        return *cdq++ & 0377;
    }

    if (Verbose > 5) {
        fprintf(stderr, "Calling read: timeout=%d  Readnum=%d ",
                timeout, Readnum);
    }

    set_tty_read_timeout(timeout, &orig_termios);
    Lleft = read(iofd, cdq = linbuf, Readnum);
    tcsetattr(iofd, TCSANOW, &orig_termios);

    if (Verbose > 5) {
        fprintf(stderr, "Read returned %d bytes\n", Lleft);
    }
    if (Lleft < 1) {
        Lleft = 0;
        return TIMEOUT;
    }
    --Lleft;
    if (Verbose > 8) {
        fprintf(stderr, "%02x ", *cdq & 0377);
    }
    return *cdq++ & 0377;
}

/* send cancel string to get the other end to shut up */
void canit(void)
{
    const char canistr[] = {
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0
    };

    printf(canistr);
    Lleft = 0;    /* Do read next time ... */
    fflush(stdout);
}

/*
 *  Debugging information output interface routine
 */
void vfile(const char * format, ...)
{
    if (Verbose > 2) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

/*
 * Log an error
 */
void zperr(const char * format, ...)
{
    va_list args;

    if (Verbose <= 0)
        return;
    fprintf(stderr, "Retry %d: ", errors);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

/*
 * Local console output simulation
 */
void bttyout(int c)
{
    if (Verbose || Fromcu)
        putc(c, stderr);
}

/**
 *  Send a character to modem.  Small is beautiful.
 */
void sendline(int c)
{
    char d;

    d = c;
    if (Verbose > 6)
        fprintf(stderr, "Sendline: %x\n", c);
    write(STDOUT_FILENO, &d, 1);
}

void flushmo(void)
{
    fflush(stdout);
}

/**
 * return 1 iff stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
void from_cu(void)
{
    struct stat a;
    struct stat b;

    fstat(1, &a);
    fstat(2, &b);
    Fromcu = a.st_rdev != b.st_rdev;
    return;
}

void cucheck(void)
{
    if (Fromcu)
        fprintf(stderr, "Please read the manual page BUGS chapter!\r\n");
}

/**
 *  Return non 0 iff something to read from io descriptor f.
 */
int rdchk(int f)
{
    int lf;

    ioctl(f, FIONREAD, &lf);
    return lf;
}
