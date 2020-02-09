/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

static int   hflag = 0;
static uid_t uid = -1;
static gid_t gid = -1;
static int   ret = 0;

static void
chownpwgr(const char *path, struct stat *st, void *data, struct recursor *r)
{
    char *chownf_name;
    int (*chownf)(const char *, uid_t, gid_t);

    if (r->follow == 'P' || (r->follow == 'H' && r->depth) ||
        (hflag && !(r->depth))) {
        chownf_name = "lchown";
        chownf = lchown;
    } else {
        chownf_name = "chown";
        chownf = chown;
    }

    if (chownf(path, uid, gid) < 0) {
        weprintf("%s %s:", chownf_name, path);
        ret = 1;
    } else if (st && S_ISDIR(st->st_mode)) {
        recurse(path, NULL, r);
    }
}

static void
usage(void)
{
    eprintf("usage: %s [-h] [-R [-H | -L | -P]] [owner][:[group]] file ...\n",
            argv0);
}

int
main(int argc, char *argv[])
{
    struct group *gr;
    struct passwd *pw;
    struct recursor r = {
        .fn = chownpwgr, .hist = NULL, .depth = 0,
        .maxdepth = 1, .follow = 'P', .flags = 0 };
    char *owner;
    char *group;

    ARGBEGIN {
    case 'h':
        hflag = 1;
        break;
    case 'r':
    case 'R':
        r.maxdepth = 0;
        break;
    case 'H':
    case 'L':
    case 'P':
        r.follow = ARGC();
        break;
    default:
        usage();
    } ARGEND;

    if (argc < 2)
        usage();

    owner = argv[0];
    if ((group = strchr(owner, ':')))
        *group++ = '\0';

    if (owner && *owner) {
        errno = 0;
        pw = getpwnam(owner);
        if (pw) {
            uid = pw->pw_uid;
        } else {
            if (errno)
                eprintf("getpwnam %s:", owner);
            uid = estrtonum(owner, 0, UINT_MAX);
        }
    }
    if (group && *group) {
        errno = 0;
        gr = getgrnam(group);
        if (gr) {
            gid = gr->gr_gid;
        } else {
            if (errno)
                eprintf("getgrnam %s:", group);
            gid = estrtonum(group, 0, UINT_MAX);
        }
    }
    for (argc--, argv++; *argv; argc--, argv++)
        recurse(*argv, NULL, &r);

    return ret || recurse_status;
}
