/**
 *******************************************************************************
 * @file    termios.h
 * @author  Olli Vanhoja
 * @brief   Define values for termios.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

/** @addtogroup LIBC
 * @{
 */

#pragma once
#ifndef TERMIOS_H
#define TERMIOS_H

#ifndef _PID_T_DECLARED
typedef int pid_t; /*!< Process ID. */
#define _PID_T_DECLARED
#endif

/*
 * Control characters.
 * There are used as indexes into c_cc array in termios struct.
 */
#define VEOF    0   /*!< EOF character. */
#define VEOL    1   /*!< EOL character. */
#define VERASE  3   /*!< ERASE character. */
#define VINTR   4   /*!< INTR character. */
#define VKILL   5   /*!< KILL character. */
#define VMIN    6   /*!< MIN value. */
#define VQUIT   7   /*!< QUIT character. */
#define VSTART  8   /*!< START character. */
#define VSTOP   9   /*!< STOP character. */
#define VSUSP   10  /*!< SUSP character. */
#define VTIME   11  /*!< TIME value. */

#define NCCS    16  /*!< Size of the array c_cc for control characters. */

/*
 * Input mode flags.
 * These are used with the c_iflag field in termios struct.
 */
#define BRKINT  0x0001  /*!< Signal interrupt on break. */
#define ICRNL   0x0002  /*!< Map CR to NL on input. */
#define IGNBRK  0x0004  /*!< Ignore break condition. */
#define IGNCR   0x0008  /*!< Ignore CR. */
#define IGNPAR  0x0010  /*!< Ignore characters with parity errors. */
#define INLCR   0x0020  /*!< Map NL to CR on input. */
#define INPCK   0x0040  /*!< Enable input parity check. */
#define ISTRIP  0x0080  /*!< Strip character. */
#define IXANY   0x0100  /*!< Enable any character to restart output. */
#define IXOFF   0x0200  /*!< Enable start/stop input control. */
#define IXON    0x0400  /*!< Enable start/stop output control. */
#define PARMRK  0x0800  /*!< Mark parity errors. */

/*
 * Output mode flags.
 * These are used with the c_oflag field in termios struct.
 */
#define OPOST   0x01    /*!< Post-process output. */
#define ONLCR   0x02    /*!< Map NL to CR-NL on output. */
#define OCRNL   0x04    /*!< Map CR to NL on output. */
#define ONOCR   0x08    /*!< No CR output at column 0. */
#define ONLRET  0x10    /*!< NL performs CR function.  */
#define OFDEL   0x20    /*!< Fill is DEL. */

/*
 * Control mode flags.
 * These are used with the c_cflag field in termios struct.
 */
#define CIGNORE 0x0001  /*!< Ignore control flags. */
#define CSIZE   0x0030  /*!< Character size. */
#define     CS5 0x0000  /*!< 5 bits */
#define     CS6 0x0010  /*!< 7 bits */
#define     CS7 0x0020  /*!< 8 bits */
#define     CS8 0x0030
#define CSTOPB  0x0100  /*!< Send two stop bits, else one. */
#define CREAD   0x0200  /*!< Enable receiver. */
#define PARENB  0x0400  /*!< Parity enable. */
#define PARODD  0x0800  /*!< Odd parity, else even. */
#define HUPCL   0x1000  /*!< Hang up on last close. */
#define CLOCAL  0x2000  /*!< Ignore modem status lines. */

/*
 * Local mode flags.
 * These are used with the c_lflag field in termios struct.
 */
#define ECHO    0x0001  /*!< Enable echo. */
#define ECHOE   0x0002  /*!< Echo erase character as error-correcting backspace. */
#define ECHOK   0x0004  /*!< Echo KILL. */
#define ECHONL  0x0008  /*!< Echo NL. */
#define ICANON  0x0010  /*!< Canonical input (erase and kill processing). */
#define IEXTEN  0x0020  /*!< Enable extended input character processing. */
#define ISIG    0x0040  /*!< Enable signals. */
#define NOFLSH  0x0080  /*!< Disable flush after interrupt or quit. */
#define TOSTOP  0x0100  /*!< Send SIGTTOU for background output. */

/*
 * Baud rates.
 */
#define B0      0
#define B50     50
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B7200   7200
#define B14400  14400
#define B28800  28800
#define B57600  57600
#define B76800  76800
#define B115200 115200
#define B230400 230400
#define B460800 460800
#define B921600 921600

/*
 * Atribute Selection.
 * Symbolic constants for use with tcsetattr().
 */
#define TCSANOW     0 /*!< Change attributes immediately. */
#define TCSADRAIN   1 /*!< Change attributes when output has drained. */
#define TCSAFLUSH   2 /*!< Change attributes when output has drained;
                       *   also flush pending input. */

/*
 * Line Control.
 * Symbolic constants for use with tcflush() and tcflow().
 */
#define TCIFLUSH    1 /*!< Flush pending input. */
#define TCIOFLUSH   2 /*!< Flush both pending input and untransmitted output. */
#define TCOFLUSH    3 /*!< Flush untransmitted output. */

#define TCIOFF      1 /*!< Transmit a STOP character, intended to suspend input
                       *   data. */
#define TCION       2 /*!< Transmit a START character, intended to restart input
                       *   data. */
#define TCOOFF      3 /*!< Suspend output. */
#define TCOON       4 /*!< Restart output. */

typedef unsigned int tcflag_t;  /*!< Type for terminal modes. */
typedef unsigned char cc_t;     /*!< Terminal special characters. */
typedef unsigned int speed_t;   /*!< Terminal baud rates. */

struct termios {
    tcflag_t    c_iflag;    /*!< Input flags. */
    tcflag_t    c_oflag;    /*!< Output flags. */
    tcflag_t    c_cflag;    /*!< Control flags. */
    tcflag_t    c_lflag;    /*!< Local flags. */
    cc_t        c_cc[NCCS]; /*!< Control chars. */
    speed_t     c_ispeed;   /*!< Input speed. */
    speed_t     c_ospeed;   /*!< Output speed. */
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int     cfsetispeed(struct termios *, speed_t);
int     cfsetospeed(struct termios *, speed_t);
int     tcdrain(int);
int     tcflow(int, int);
int     tcflush(int, int);
int     tcgetattr(int, struct termios *);
pid_t   tcgetsid(int);
int     tcsendbreak(int, int);
int     tcsetattr(int, int, const struct termios *);
__END_DECLS
#endif

#endif /* TERMIOS_H */

/**
 * @}
 */
