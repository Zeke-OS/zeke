/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Concatenate files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>

char * cmd;
int bflg, eflg, nflg, sflg, tflg, uflg, vflg;
int spaced, col, lno, inlin, ibsize, obsize;

static int copyopt(FILE * file);
static int unbufcat(FILE * file);
static int fastcat(FILE * file);

int main(int argc, char ** argv)
{
    int fflg = 0;
    int dev = 0, ino = -1;
    struct stat statb;
    int retval = 0;
    int (*catfn)(FILE * file);

    cmd = argv[0];

    lno = 1;
    for ( ; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
        switch (argv[1][1]) {
        case 0:
            break;
        case 'u': /* Unbuffered output. */
            setbuf(stdout, (char *)NULL);
            uflg++;
            continue;
        case 'n': /* Numbered lines. */
            nflg++;
            continue;
        case 'b': /* Omit line numbers for empty lines and set -n. */
            bflg++;
            nflg++;
            continue;
        case 'v': /* Display non-printable characters. */
            vflg++;
            continue;
        case 's': /* Remove adjacent empty lines. */
            sflg++;
            continue;
        case 'e': /* Display $ at the end of each line. */
            eflg++;
            vflg++;
            continue;
        case 't': /* Display tabs as ^I. */
            tflg++;
            vflg++;
            continue;
        }
        break;
    }
    if (fstat(fileno(stdout), &statb) == 0) {
        statb.st_mode &= S_IFMT;
        if (statb.st_mode != S_IFCHR && statb.st_mode != S_IFBLK) {
            dev = statb.st_dev;
            ino = statb.st_ino;
        }
        obsize = statb.st_blksize;
    } else {
        obsize = 0;
    }
    if (argc < 2) {
        argc = 2;
        fflg++;
    }
    if (nflg || sflg || vflg) {
        catfn = copyopt;
    } else if (uflg) {
        catfn = unbufcat;
    } else { /* no flags specified */
        catfn = fastcat;
    }

    while (--argc > 0) {
        FILE * fi;

        /* Select fi */
        if (fflg || ((*++argv)[0] == '-' && (*argv)[1] == '\0')) {
            fi = stdin;
        } else if ((fi = fopen(*argv, "r")) == NULL) {
            perror(*argv);
            retval = 1;
            continue;
        }

        if (fstat(fileno(fi), &statb) == 0) {
            if ((statb.st_mode & S_IFMT) == S_IFREG &&
                statb.st_dev == dev && statb.st_ino == ino) {
                fprintf(stderr, "%s: input %s is output\n", cmd,
                   fflg ? "-" : *argv);
                fclose(fi);
                retval = 1;
                continue;
            }
            ibsize = statb.st_blksize;
        } else {
            ibsize = 0;
        }

        retval |= catfn(fi);

        if (fi != stdin)
            fclose(fi);
        else
            clearerr(fi);       /* reset sticky EOF */
        if (ferror(stdout)) {
            fprintf(stderr, "%s: output write error\n", cmd);
            retval = 1;
            break;
        }
    }

    return retval;
}

static int copyopt(FILE * file)
{
    int c;

top:
    c = getc(file);
    if (c == EOF)
        return 0;
    if (c == '\n') {
        if (inlin == 0) {
            if (sflg && spaced)
                goto top;
            spaced = 1;
        }
        if (nflg && bflg == 0 && inlin == 0)
            printf("%6d\t", lno++);
        if (eflg)
            putchar('$');
        putchar('\n');
        inlin = 0;
        goto top;
    }
    if (nflg && inlin == 0)
        printf("%6d\t", lno++);
    inlin = 1;
    if (vflg) {
        if (tflg == 0 && c == '\t')
            putchar(c);
        else {
            if (c > 0177) {
                printf("M-");
                c &= 0177;
            }
            if (c < ' ')
                printf("^%c", c+'@');
            else if (c == 0177)
                printf("^?");
            else
                putchar(c);
        }
    } else
        putchar(c);
    spaced = 0;
    goto top;
}

static int unbufcat(FILE * file)
{
    int c;

    while ((c = getc(file)) != EOF) {
        putchar(c);
    }

    return 0;
}

static int fastcat(FILE * file)
{
    int    fd, buffsize, n, nwritten;
    char   * buff;

    fd = fileno(file);

    if (obsize)
        buffsize = obsize;  /* common case, use output blksize */
    else if (ibsize)
        buffsize = ibsize;
    else
        buffsize = BUFSIZ;

    if ((buff = malloc(buffsize)) == NULL) {
        perror("cat: no memory");
        return 1;
    }

    /*
     * Note that on some systems (V7), very large writes to a pipe
     * return less than the requested size of the write.
     * In this case, multiple writes are required.
     */
    while ((n = read(fd, buff, buffsize)) > 0) {
        int offset = 0;
        do {
            nwritten = write(fileno(stdout), &buff[offset], n);
            if (nwritten <= 0) {
                perror("cat: write error");
                exit(2);
            }
            offset += nwritten;
        } while ((n -= nwritten) > 0);
    }

    free(buff);
    if (n < 0) {
        perror("cat: read error");
        return 1;
    }

    return 0;
}
