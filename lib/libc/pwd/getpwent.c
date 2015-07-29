/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

static FILE *pw_fp;
static struct passwd pw_entry;
static int pw_stayopen;
static char line[256];

static int start_pw(void)
{
    if (pw_fp) {
        rewind(pw_fp);
        return 1;
    }
    pw_fp = fopen(_PATH_PASSWD, "r");
    if (pw_fp)
        return 1;
    return 0;
}

static char * get_next_line(FILE * fp)
{
    while (fgets(line, sizeof(line), fp)) {
        char * cp;
        int ch;

        cp = strchr(line, '\n');
        if (cp) {
            *cp = '\0';
            return line;
        }

        /* skip lines that are too long */
        while ((ch = fgetc(fp)) != '\n' && ch != EOF);
    }

    return NULL;
}

static int scanpw(void)
{
    while (get_next_line(pw_fp)) {
        char * cp;
        char * bp;

        bp = line;
        pw_entry.pw_name = strsep(&bp, ":");
        pw_entry.pw_passwd = strsep(&bp, ":");
        cp = strsep(&bp, ":");
        if (!cp)
            continue;
        pw_entry.pw_uid = atoi(cp);
        cp = strsep(&bp, ":");
        if (!cp)
            continue;
        pw_entry.pw_gid = atoi(cp);
        pw_entry.pw_gecos = strsep(&bp, ":");
        pw_entry.pw_dir = strsep(&bp, ":");
        pw_entry.pw_shell = strsep(&bp, ":");
        if (!pw_entry.pw_shell)
            continue;

        return 1;
    }

    return 0;
}

static void getpw(void)
{
    static char pwbuf[50];
    long pos;
    int fd, n;
    char *p;

    if (geteuid())
        return;

    if ((fd = open(_PATH_SHADOW, O_RDONLY, 0)) < 0) {
        return;
    }

    pos = atol(pw_entry.pw_passwd);
    if (lseek(fd, pos, SEEK_SET) != pos) {
        goto bad;
    }
    if ((n = read(fd, pwbuf, sizeof(pwbuf) - 1)) < 0) {
        goto bad;
    }
    pwbuf[n] = '\0';
    for (p = pwbuf; *p; ++p) {
        if (*p == ':') {
            *p = '\0';
            pw_entry.pw_passwd = pwbuf;
            break;
        }
    }
bad:
    (void)close(fd);
}

struct passwd * getpwent(void)
{
    if (!pw_fp && !start_pw())
        return NULL;
    if (!scanpw())
        return 0;

    if (pw_entry.pw_passwd[0] != '$')
        getpw();

    return &pw_entry;
}

struct passwd * getpwnam(char * nam)
{
    int rval;

    if (!start_pw())
        return NULL;
    for (rval = 0; scanpw();) {
        if (!strcmp(nam, pw_entry.pw_name)) {
            rval = 1;
            break;
        }
    }
    if (!pw_stayopen)
        endpwent();
    if (!rval)
        return NULL;

    if (pw_entry.pw_passwd[0] != '$')
        getpw();

    return &pw_entry;
}

struct passwd * getpwuid(uid_t uid)
{
    int rval;

    if (!start_pw())
        return NULL;
    for (rval = 0; scanpw();) {
        if (pw_entry.pw_uid == uid) {
            rval = 1;
            break;
        }
    }
    if (!pw_stayopen)
        endpwent();
    if (!rval)
        return 0;

    if (pw_entry.pw_passwd[0] != '$')
        getpw();

    return &pw_entry;
}

int setpwent(void)
{
    return setpassent(0);
}

int setpassent(int stayopen)
{
    if (!start_pw())
        return 0;
    pw_stayopen = stayopen;
    return 1;
}

void endpwent(void)
{
    if (pw_fp) {
        (void)fclose(pw_fp);
        pw_fp = 0;
    }
}
