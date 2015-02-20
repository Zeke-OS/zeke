#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define PRINTFLAG(x, name, flg) \
    if ((x) & (flg)) printf(" %s", name);

int main(void)
{
    struct termios t;

    tcgetattr(1, &t);

#define FORALL_IFLAGS(apply) \
    apply(BRKINT) \
    apply(ICRNL) \
    apply(IGNBRK) \
    apply(IGNCR) \
    apply(IGNPAR) \
    apply(INLCR) \
    apply(INPCK) \
    apply(ISTRIP) \
    apply(IXANY) \
    apply(IXOFF) \
    apply(IXON) \
    apply(PARMRK)

#define FORALL_OFLAGS(apply) \
    apply(OPOST) \
    apply(ONLCR) \
    apply(OCRNL) \
    apply(ONOCR) \
    apply(ONLRET) \
    apply(OFDEL)

#define FORALL_CFLAGS(apply) \
    apply(CIGNORE) \
    apply(CSIZE) \
    apply(CS5) \
    apply(CS6) \
    apply(CS7) \
    apply(CS8) \
    apply(CSTOPB) \
    apply(CREAD) \
    apply(PARENB) \
    apply(PARODD) \
    apply(HUPCL) \
    apply(CLOCAL)

#define FORALL_LFLAGS(apply) \
    apply(ECHO) \
    apply(ECHOE) \
    apply(ECHOK) \
    apply(ECHONL) \
    apply(ICANON) \
    apply(IEXTEN) \
    apply(ISIG) \
    apply(NOFLSH) \
    apply(TOSTOP)
    /*apply(DEFECHO)*/

#define PIFLAGS(flag) \
    PRINTFLAG(t.c_iflag, #flag, flag)

#define POFLAGS(flag) \
    PRINTFLAG(t.c_oflag, #flag, flag)

#define PCFLAGS(flag) \
    PRINTFLAG(t.c_cflag, #flag, flag)

#define PLFLAGS(flag) \
    PRINTFLAG(t.c_lflag, #flag, flag)

    printf("t.c_iflag:");
    FORALL_IFLAGS(PIFLAGS)

    printf("\nt.c_oflag:");
    FORALL_OFLAGS(POFLAGS)

    printf("\nt.c_cflag:");
    FORALL_CFLAGS(PCFLAGS)

    printf("\nt.c_lflag:");
    FORALL_LFLAGS(PLFLAGS)

    printf("\nispeed: %d\n", cfgetispeed(&t));
    printf("ospeed: %d\n", cfgetospeed(&t));

    return 0;
}
