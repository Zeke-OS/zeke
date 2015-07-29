/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980, 1983, 1987, 1988
                 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>
#include <zeke.h>

#define TIMEOUT 300

char * argv0;
struct flags {
    int f           : 1;
    int p           : 1;
    int ask         : 1;
    int passwd_req  : 1;
} flags;

static char hostname[HOST_NAME_MAX];
static char * username;
static char password[11];
#if 0
static char * ttyn;
#endif
struct  passwd * pwd;
extern char ** environ;
char * envinit[1];
char shell[MAXPATHLEN + 2];


void timedout(int sig)
{
    fprintf(stderr, "Login timed out after %d seconds\n", TIMEOUT);
    exit(EX_OK);
}

static void install_sighandlers(void)
{
    signal(SIGALRM, timedout);
    /* TODO Set alarm */
#if 0
    alarm(TIMEOUT);
#endif
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT, SIG_IGN);
}

static void reset_sighandlers(void)
{
    signal(SIGALRM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_IGN);
}

static void getloginname(void)
{
    int ch;
    char * p;
    static char nbuf[MAXLOGNAME];

    for (;;) {
        printf("login: ");
        fflush(stdout);
        /* TODO We'd like to flush stdin but it causes SIGBUS on second login */
#if 0
        fflush(stdin);
#endif
        for (p = nbuf; (ch = getchar()) != '\r'; ) {
            char ebuf[1];
            if (ch == EOF) {
                exit(EX_OK);
            }

            /* TODO Request for ECHO instead. */
            ebuf[0] = ch;
            write(fileno(stdout), ebuf, sizeof(ebuf));

            if (p < nbuf + MAXLOGNAME - 1)
                *p++ = ch;
        }
        printf("\n");
        if (p > nbuf && nbuf[0] == '-') {
            fprintf(stderr, "login names may not start with '-'.\n");
        } else {
            *p = '\0';
            username = nbuf;
            break;
        }
    }
}

static void getpass(void)
{
    struct termios ttyb;
    tcflag_t flags;
    register char * p;
    register int c;
    int fd;
    FILE * fi = stdin; /* stdin as a fallback */

    fd = open("/dev/tty", O_RDONLY | O_TTY_INIT);
    if (fd >= 0) {
        fi = fdopen(fd, "r");
        if (fi)
            setbuf(fi, NULL);
    }

    tcgetattr(fileno(fi), &ttyb);
    flags = ttyb.c_lflag;
    ttyb.c_lflag &= ~ECHO;
    tcsetattr(fileno(fi), 0,  &ttyb);

    fprintf(stderr, "Password: ");

    for (p = password; (c = fgetc(fi)) != '\r' && c != EOF;) {
        if (p < &password[sizeof(password) - 1])
            *p++ = c;
    }
    *p = '\0';
    fprintf(stderr, "\n");
    ttyb.c_lflag = flags;
    tcsetattr(fileno(fi), 0,  &ttyb);
    if (fi != stdin)
        fclose(fi);
}

static int initgroups(char * uname, int agroup)
{
    gid_t groups[NGROUPS_MAX];
    struct group *grp;
    int i, ngroups = 0;

    if (agroup >= 0)
        groups[ngroups++] = agroup;
    setgrent();
    while ((grp = getgrent())) {
        if (grp->gr_gid == agroup)
            continue;
        for (i = 0; grp->gr_mem[i]; i++)
            if (!strcmp(grp->gr_mem[i], uname)) {
                if (ngroups == NGROUPS_MAX) {
                        fprintf(stderr, "%s is in too many groups\n", uname);
                    goto toomany;
                }
                groups[ngroups++] = grp->gr_gid;
            }
    }
toomany:
    endgrent();
    if (setgroups(ngroups, groups) < 0) {
        perror("setgroups");
        return -1;
    }
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "usage: %s [-fp] [username]\n", argv0);
    exit(EX_USAGE);
}

int main(int argc, char * argv[])
{
    int ch, cnt, failures = 0;
    char * p;
    char * salt;

    argv0 = argv[0];

    install_sighandlers();

    while ((ch = getopt(argc, argv, "fh:p")) != EOF) {
        switch (ch) {
        case 'f': /* Skip second authentication */
            flags.f = 1;
            break;
        case 'h':
            if (getuid()) {
                fprintf(stderr, "%s: -h for super-user only.\n", argv0);
                exit(EX_USAGE);
            }
            /* TODO Remove domain */
            strlcpy(hostname, optarg, sizeof(hostname));
            break;
        case 'p':
            flags.p = 1;
            break;
        case '?':
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;
    if (*argv) {
        username = *argv;
        flags.ask = 0;
    } else {
        flags.ask = 1;
    }

    if (hostname[0] == '\0')
        gethostname(hostname, sizeof(hostname));

    /* TODO Reset tty params? */

    closeall(2);
    /* TODO Get tty name */
#if 0
    ttyn = ttyname();
#endif

#if 0
    openlog("login", LOG_ODELAY, LOG_AUTH);
#endif

    for (cnt = 0;; flags.ask = 1) {
        if (flags.ask) {
            flags.f = 0;
            getloginname();
        }

        pwd = getpwnam(username);
        if (!pwd) {
            salt = "xx";
            goto nouser;
        }
        salt = pwd->pw_passwd;

        if (flags.f) {
            uid_t uid = getuid();

            flags.passwd_req = pwd->pw_uid == 0 || (uid && uid != pwd->pw_uid);
        }

        if (!flags.passwd_req || !*pwd->pw_passwd) {
            break;
        }

nouser:
        getpass();
        p = crypt(password, salt);
        memset(password, 0, sizeof(password));
        if (pwd && !strcmp(p, pwd->pw_passwd))
            break;

        printf("Login incorrect\n");
        failures++;
        if (++cnt > 3) {
            if (cnt >= 10) {
                sleep(5);
                exit(EX_OSERR);
            }
            sleep((unsigned)((cnt - 3) * 5));
        }
    }

    /* Login ok before timeout. */
    /* TODO Reset alarm */
#if 0
    alarm(0);
#endif

    endpwent();

    if (chdir(pwd->pw_dir) < 0) {
        printf("No directory %s!\n", pwd->pw_dir);
        if (chdir("/") < 0)
            exit(EX_OSERR);
        pwd->pw_dir = "/";
        printf("Logging in with home = \"/\".\n");
    }

    /* TODO utmp? */

    /* TODO chown & chmod tty */
#if 0
    chown(ttyn, pwd->pw_uid,
          (gr = getgrnam("tty")) ? gr->gr_gid : pwd->pw_gid);
    chmod(ttyn, 0620);
#endif

    /* TODO Create a new session */
    setgid(pwd->pw_gid);
    initgroups(username, pwd->pw_gid);

    if (*pwd->pw_shell == '\0')
        pwd->pw_shell = _PATH_BSHELL;

    if (!flags.p)
        environ = envinit;
    setenv("HOME", pwd->pw_dir, 1);
    setenv("SHELL", pwd->pw_shell, 1);
    /* TODO (void)setenv("TERM", term, 0); */
    setenv("TERM", "vt100", 1);
    (void)setenv("USER", pwd->pw_name, 1);
    (void)setenv("PATH", _PATH_STDPATH, 0);

    /* TODO motd */

    reset_sighandlers();

    if (setlogin(pwd->pw_name) < 0)
        fprintf(stderr, "%s: setlogin(): %s\n", argv0, strerror(errno));

    strlcpy(shell, (p = strrchr(pwd->pw_shell, '/')) ? p + 1 : pwd->pw_shell,
            sizeof(shell));

    setuid(pwd->pw_uid);
    execlp(pwd->pw_shell, shell, NULL);
    fprintf(stderr, "%s: no shell: %s\n", argv0, strerror(errno));

    return EX_OSERR;
}
