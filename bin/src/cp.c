/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

int iflag;
int rflag;
int pflag;

static char * argv0;

static int copy(char * from, char * to);
static int rcopy(char * from, char * to);
static int setimes(char * path, struct stat * statp);
static void Perror(char * s);

int main(int argc, char ** argv)
{
    struct stat stb;
    int rc, i;

    argv0 = argv[0];

    argc--, argv++;
    while (argc > 0 && **argv == '-') {
        (*argv)++;
        while (**argv) {
            switch (*(*argv)++) {
            case 'i':
                iflag++;
                break;

            case 'R':
            case 'r':
                rflag++;
                break;

            case 'p':   /* preserve mtimes, atimes, and modes */
                pflag++;
                (void)umask(0);
                break;

            default:
                goto usage;
            }
        }
        argc--;
        argv++;
    }
    if (argc < 2)
        goto usage;
    if (argc > 2) {
        if (stat(argv[argc-1], &stb) < 0)
            goto usage;
        if ((stb.st_mode&S_IFMT) != S_IFDIR)
            goto usage;
    }
    rc = 0;
    for (i = 0; i < argc - 1; i++) {
        rc |= copy(argv[i], argv[argc - 1]);
    }
    exit(rc);
usage:
    fprintf(stderr,
        "Usage: cp [-ip] f1 f2; or: cp [-irp] f1 ... fn d2\n");
    exit(1);
}

#define MAXBSIZE 1000
static int copy(char * from, char * to)
{
    int fold, fnew, n, exists;
    char destname[MAXPATHLEN + 1], buf[MAXBSIZE];
    struct stat stfrom, stto;

    fold = open(from, O_RDONLY);
    if (fold < 0) {
        Perror(from);
        return 1;
    }
    if (fstat(fold, &stfrom) < 0) {
        Perror(from);
        (void)close(fold);
        return 1;
    }
    if (stat(to, &stto) >= 0 &&
        (stto.st_mode&S_IFMT) == S_IFDIR) {
        char * last;

        last  = strrchr(from, '/');
        if (last)
            last++;
        else
            last = from;
        if (strlen(to) + strlen(last) >= sizeof destname - 1) {
            fprintf(stderr, "%s: %s/%s: Name too long", argv0, to, last);
            (void)close(fold);
            return 1;
        }
        (void)sprintf(destname, "%s/%s", to, last);
        to = destname;
    }
    if (rflag && (stfrom.st_mode & S_IFMT) == S_IFDIR) {
        int fixmode = 0;    /* cleanup mode after rcopy */

        (void)close(fold);
        if (stat(to, &stto) < 0) {
            if (mkdir(to, (stfrom.st_mode & 07777) | 0700) < 0) {
                Perror(to);
                return 1;
            }
            fixmode = 1;
        } else if ((stto.st_mode & S_IFMT) != S_IFDIR) {
            fprintf(stderr, "%s: %s: Not a directory.\n", argv0, to);
            return 1;
        } else if (pflag)
            fixmode = 1;
        n = rcopy(from, to);
        if (fixmode)
            (void)chmod(to, stfrom.st_mode & 07777);
        return n;
    }

    if ((stfrom.st_mode & S_IFMT) == S_IFDIR) {
        fprintf(stderr, "%s: %s: Is a directory (copying as plain file).\n",
                argv0, from);
    }

    exists = stat(to, &stto) == 0;
    if (exists) {
        if (stfrom.st_dev == stto.st_dev &&
            stfrom.st_ino == stto.st_ino) {
            fprintf(stderr, "%s: %s and %s are identical (not copied).\n",
                    argv0, from, to);
            (void)close(fold);
            return 1;
        }
        if (iflag && isatty(fileno(stdin))) {
            int i, c;

            fprintf(stderr, "overwrite %s? ", to);
            i = c = getchar();
            while (c != '\n' && c != EOF) {
                c = getchar();
            }
            if (i != 'y') {
                (void)close(fold);
                return 1;
            }
        }
    }
    fnew = creat(to, stfrom.st_mode & 07777);
    if (fnew < 0) {
        Perror(to);
        (void)close(fold); return(1);
    }
    if (exists && pflag)
        (void)fchmod(fnew, stfrom.st_mode & 07777);

    for (;;) {
        n = read(fold, buf, sizeof buf);
        if (n == 0)
            break;
        if (n < 0) {
            Perror(from);
            (void)close(fold);
            (void)close(fnew);
            return 1;
        }
        if (write(fnew, buf, n) != n) {
            Perror(to);
            (void)close(fold);
            (void)close(fnew);
            return 1;
        }
    }
    (void)close(fold);
    (void)close(fnew);
    if (pflag)
        return setimes(to, &stfrom);
    return 0;
}

static int rcopy(char * from, char * to)
{
    DIR *fold = opendir(from);
    struct stat statb;
    int errs = 0;
    char fromname[MAXPATHLEN + 1];

    if (fold == 0 || (pflag && fstat(fold->dd_fd, &statb) < 0)) {
        Perror(from);
        return 1;
    }
    for (;;) {
        struct dirent * dp;

        dp = readdir(fold);
        if (dp == 0) {
            closedir(fold);
            if (pflag)
                return setimes(to, &statb) + errs;
            return errs;
        }
        if (dp->d_ino == 0)
            continue;
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        if (strlen(from) + 1 + strlen(dp->d_name) >= sizeof fromname - 1) {
            fprintf(stderr, "%s: %s/%s: Name too long.\n",
                    argv0, from, dp->d_name);
            errs++;
            continue;
        }
        (void)sprintf(fromname, "%s/%s", from, dp->d_name);
        errs += copy(fromname, to);
    }
}

static int setimes(char * path, struct stat * statp)
{
    struct timespec times[2];

    times[0] = statp->st_atime;
    times[1] = statp->st_mtime;
    if (utimensat(AT_FDCWD, path, times, 0)) {
        Perror(path);
        return 1;
    }
    return 0;
}

static void Perror(char * s)
{
    fprintf(stderr, "%s: ", argv0);
    perror(s);
}
