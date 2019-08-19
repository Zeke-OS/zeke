/*-
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static void pretty(struct passwd * pw);
static void current(void);
static void user(struct passwd * pw);
static void group(struct passwd * pw, int nflag);
static struct passwd * who(char * u);
static void usage(void);

int main(int argc, char * argv[])
{
    struct passwd *pw;
    int Gflag, ch, gflag, id, nflag, pflag, rflag, uflag;

    Gflag = gflag = nflag = pflag = rflag = uflag = 0;
    while ((ch = getopt(argc, argv, "Ggnpru")) != EOF)
        switch (ch) {
        case 'G':
            Gflag = 1;
            break;
        case 'g':
            gflag = 1;
            break;
        case 'n':
            nflag = 1;
            break;
        case 'p':
            pflag = 1;
            break;
        case 'r':
            rflag = 1;
            break;
        case 'u':
            uflag = 1;
            break;
        default:
            usage();
        }
    argc -= optind;
    argv += optind;

    switch (Gflag + gflag + pflag + uflag) {
    case 1:
        break;
    case 0:
        if (!nflag && !rflag)
            break;
        /* FALLTHROUGH */
    default:
        usage();
    }

    pw = *argv ? who(*argv) : NULL;

    if (gflag) {
        struct group *gr;

        id = pw ? pw->pw_gid : rflag ? getgid() : getegid();
        if (nflag && (gr = getgrgid(id)))
            printf("%s\n", gr->gr_name);
        else
            printf("%d\n", id);
        exit(0);
    }

    if (uflag) {
        id = pw ? pw->pw_uid : rflag ? getuid() : geteuid();
        if (nflag && (pw = getpwuid(id)))
            printf("%s\n", pw->pw_name);
        else
            printf("%d\n", id);
        exit(0);
    }

    if (Gflag) {
        group(pw, nflag);
        exit(0);
    }

    if (pflag) {
        pretty(pw);
        exit(0);
    }

    if (pw)
        user(pw);
    else
        current();
    exit(0);
}

static void pretty(struct passwd * pw)
{
    uid_t eid, rid;

    if (pw) {
        printf("uid\t%s\n", pw->pw_name);
        printf("groups\t");
        group(pw, 1);
    } else {
        char *login;

        if ((login = getlogin()) == NULL) {
            perror("getlogin");
            exit(1);
        }

        pw = getpwuid(rid = getuid());
        if (pw == NULL || strcmp(login, pw->pw_name))
            printf("login\t%s\n", login);
        if (pw)
            printf("uid\t%s\n", pw->pw_name);
        else
            printf("uid\t%u\n", rid);

        if ((eid = geteuid()) != rid) {
            if ((pw = getpwuid(eid))) {
                printf("euid\t%s", pw->pw_name);
            } else {
                printf("euid\t%u", eid);
            }
        }
        if ((rid = getgid()) != getegid()) {
            struct group *gr;

            if ((gr = getgrgid(rid))) {
                printf("rgid\t%s\n", gr->gr_name);
            } else {
                printf("rgid\t%u\n", rid);
            }
        }
        printf("groups\t");
        group(NULL, 1);
    }
}

static void current(void)
{
    struct group *gr;
    struct passwd *pw;
    uid_t id, eid, lastid;
    int ngroups;
    gid_t groups[NGROUPS_MAX];

    id = getuid();
    printf("uid=%u", id);
    if ((pw = getpwuid(id)))
        printf("(%s)", pw->pw_name);
    if ((eid = geteuid()) != id) {
        printf(" euid=%u", eid);
        if ((pw = getpwuid(eid)))
            printf("(%s)", pw->pw_name);
    }
    id = getgid();
    printf(" gid=%u", id);
    if ((gr = getgrgid(id)))
        printf("(%s)", gr->gr_name);
    if ((eid = getegid()) != id) {
        printf(" egid=%u", eid);
        if ((gr = getgrgid(eid)))
            printf("(%s)", gr->gr_name);
    }
    ngroups = getgroups(NGROUPS_MAX, groups);
    if (ngroups > 0) {
        int cnt;
        char * fmt;

        for (fmt = " groups=%u", lastid = -1, cnt = 0; cnt < ngroups;
             fmt = ", %u", lastid = id) {
            id = groups[cnt++];
            if (lastid == id)
                continue;
            printf(fmt, id);
            if ((gr = getgrgid(id)))
                printf("(%s)", gr->gr_name);
        }
    }
    printf("\n");
}

static void user(struct passwd * pw)
{
    struct group * gr;
    gid_t groups[NGROUPS_MAX + 1];
    int cnt, id, lastid, ngroups;
    char * fmt;

    id = pw->pw_uid;
    printf("uid=%d(%s)", id, pw->pw_name);
    printf(" gid=%u", pw->pw_gid);
    if ((gr = getgrgid(pw->pw_gid)) != NULL)
        printf("(%s)", gr->gr_name);
    ngroups = NGROUPS_MAX + 1;
    (void) getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
    fmt = " groups=%u";
    for (lastid = -1, cnt = 0; cnt < ngroups; ++cnt) {
        if (lastid == (id = groups[cnt]))
            continue;
        printf(fmt, id);
        fmt = " %u";
        if ((gr = getgrgid(id)))
            printf("(%s)", gr->gr_name);
        lastid = id;
    }
    printf("\n");
}

static void group(struct passwd * pw, int nflag)
{
    int cnt, lastid, ngroups;
    gid_t groups[NGROUPS_MAX + 1];
    char *fmt;

    if (pw) {
        ngroups = NGROUPS_MAX + 1;
        (void) getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
    } else {
        groups[0] = getgid();
        ngroups = getgroups(NGROUPS_MAX, groups + 1) + 1;
    }

    fmt = nflag ? "%s" : "%u";
    for (lastid = -1, cnt = 0; cnt < ngroups; ++cnt) {
        int id;

        if (lastid == (id = groups[cnt]))
            continue;
        if (nflag) {
            struct group *gr;

            if ((gr = getgrgid(id)))
                printf(fmt, gr->gr_name);
            else
                printf(*fmt == ' ' ? " %u" : "%u", id);
            fmt = " %s";
        } else {
            printf(fmt, id);
            fmt = " %u";
        }
        lastid = id;
    }
    printf("\n");
}

static struct passwd * who(char * u)
{
    struct passwd *pw;
    long id;
    char *ep;

    /*
     * Translate user argument into a pw pointer.  First, try to
     * get it as specified.  If that fails, try it as a number.
     */
    if ((pw = getpwnam(u)))
        return pw;
    id = strtol(u, &ep, 10);
    if (*u && !*ep && (pw = getpwuid(id)))
        return pw;

    fprintf(stderr, "%s: No such user", u);
    exit(1);
}

static void usage(void)
{
    (void)fprintf(stderr, "usage: id [user]\n");
    (void)fprintf(stderr, "       id -G [-n] [user]\n");
    (void)fprintf(stderr, "       id -g [-nr] [user]\n");
    (void)fprintf(stderr, "       id -u [-nr] [user]\n");
    exit(1);
}
