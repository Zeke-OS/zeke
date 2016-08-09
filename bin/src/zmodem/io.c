#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "zmodem.h"

char linbuf[255];
int Lleft; /* number of characters in linbuf */

static void set_tty_read_timeout(int timeout, struct termios * orig)
{
    struct termios termios;

    tcgetattr(iofd, &termios);
    memcpy(orig, &termios, sizeof(struct termios));
    termios.c_lflag &= ~ICANON; /* Set non-canonical mode */
    termios.c_cc[VTIME] = timeout; /* Set timeout of 10.0 seconds */
    tcsetattr(iofd, TCSANOW, &termios);
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

/* sz.c version */
#if 0
/*
 * readline(timeout) reads character(s) from file descriptor 0
 * timeout is in tenths of seconds
 */
int readline(int timeout)
{
    int c;
    static char byt[1];

    fflush(stdout);
    if (setjmp(tohere)) {
        zperr("TIMEOUT");
        return TIMEOUT;
    }
    c = timeout / 10;
    if (c < 2)
        c = 2;
    if (Verbose > 5) {
        fprintf(stderr, "Timeout=%d Calling alarm(%d) ", timeout, c);
    }
    signal(SIGALRM, alrm);
    alarm(c);
    c = read(iofd, byt, 1);
    alarm(0);
    if (Verbose > 5)
        fprintf(stderr, "ret %x\n", byt[0]);
    if (c < 1)
        return TIMEOUT;
    return byt[0] & 0377;
}
#endif

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


/*
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
