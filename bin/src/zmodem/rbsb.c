/*
 *
 *  Rev 05-05-1988
 *  This file contains Unix specific code for setting terminal modes,
 *  very little is specific to ZMODEM or YMODEM per se (that code is in
 *  sz.c and rz.c).  The CRC-16 routines used by XMODEM, YMODEM, and ZMODEM
 *  are also in this file, a fast table driven macro version
 *
 *  V7/BSD HACKERS:  SEE NOTES UNDER mode(2) !!!
 *
 *   This file is #included so the main file can set parameters such as HOWMANY.
 *   See the main files (rz.c/sz.c) for compile instructions.
 */

#ifdef V7
#include <sys/types.h>
#include <sys/stat.h>
#include <sgtty.h>
#define OS "V7/BSD"
#ifdef LLITOUT
long Locmode;       /* Saved "local mode" for 4.x BSD "new driver" */
long Locbit = LLITOUT;  /* Bit SUPPOSED to disable output translations */
#include <strings.h>
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#define OS "POSIX"

#include <stdio.h>

#include "zmodem.h"

#if HOWMANY  > 255
#error Howmany must be 255 or less
#endif

/*
 * return 1 iff stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
int Fromcu;     /* Were called from cu or yam */

void from_cu(void)
{
    struct stat a, b;

    fstat(1, &a); fstat(2, &b);
    Fromcu = a.st_rdev != b.st_rdev;
    return;
}

void cucheck(void)
{
    if (Fromcu)
        fprintf(stderr, "Please read the manual page BUGS chapter!\r\n");
}


struct {
    unsigned baudr;
    int speedcode;
} speeds[] = {
    { 110,    B110 },
    { 300,    B300 },
#ifdef B600
    { 600,    B600 },
#endif
    { 1200,   B1200 },
    { 2400,   B2400 },
    { 4800,   B4800 },
    { 9600,   B9600 },
#ifdef EXTA
    { 19200,  EXTA },
    { 38400,  EXTB },
#endif
    { 0, 0 }
};

int Twostop;        /* Use two stop bits */


#ifndef READCHECK
#ifdef FIONREAD
#define READCHECK
/*
 *  Return non 0 iff something to read from io descriptor f
 */
int rdchk(int f)
{
    static long lf;

    ioctl(f, FIONREAD, &lf);
    return lf;
}
#endif
#ifdef SV
#define READCHECK
#include <fcntl.h>

char checked = '\0' ;
/*
 * Nonblocking I/O is a bit different in System V, Release 2
 */
int rdchk(int f)
{
    int lf, savestat;

    savestat = fcntl(f, F_GETFL) ;
    fcntl(f, F_SETFL, savestat | O_NDELAY) ;
    lf = read(f, &checked, 1) ;
    fcntl(f, F_SETFL, savestat) ;
    return lf;
}
#endif
#endif


static unsigned getspeed(int code)
{
    for (int n = 0; speeds[n].baudr; ++n)
        if (speeds[n].speedcode == code)
            return speeds[n].baudr;
    return 38400;   /* Assume fifo if ioctl failed */
}

struct termios oldtty, tty;

int iofd; /*!< File descriptor for ioctls & reads */

/**
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM or YMODEM-g
 *  1: save old tty stat, set raw mode
 *  0: restore original tty mode
 */
int mode(int n)
{
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
