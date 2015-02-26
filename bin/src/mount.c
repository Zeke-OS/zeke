#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mount.h>

struct opt {
    const int opt;
    const char * optname;
} optnames[] = {
    { MNT_RDONLY,       "ro" },
    { MNT_SYNCHRONOUS,  "sync" },
    { MNT_ASYNC,        "async" },
    { MNT_NOEXEC,       "noexec" },
    { MNT_NOSUID,       "nosuid" },
    { MNT_NOATIME,      "noatime" },
};

static char * catopt(char * s0, char * s1);
static unsigned long opt2flags(char ** options);

static void usage(void)
{
    fprintf(stderr,
            "usage: mount [-rw] [-o options] [-t type] source dest\n");
    exit(1);
}

int main(int argc, char * argv[])
{
    int ch, flags = 0;
    char * vfstype = NULL;
    char * options = NULL;
    char * src;
    char * dst;

    while ((ch = getopt(argc, argv, "o:rwt:")) != EOF) {
        switch (ch) {
        case 'o':
            if (*optarg)
                options = catopt(options, optarg);
            break;
        case 'r':
            flags |= MNT_RDONLY;
            break;
        case 't':
            if (vfstype) {
                fprintf(stderr, "mount: only one -t option may be specified");
                exit(1);
            }
            vfstype = optarg;
        case 'w':
            flags &= ~MNT_RDONLY;
            break;
        case '?':
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (!vfstype)
        vfstype = "auto";

    if (argc < 2) {
        usage();
    }
    src = argv[0];
    dst = argv[1];

    flags |= opt2flags(&options);

#if 0
    printf("mount: flags: %d, options: \"%s\", vfstype: \"%s\" "
            "src: \"%s\", dst: \"%s\"\n",
            flags, options, vfstype, src, dst);
#endif

    if (mount(src, dst, vfstype, flags, options))
        perror("mount: failed");

    return 0;
}

static char * catopt(char * s0, char * s1)
{
    char * cp;

    if (s0 && *s0) {
        cp = malloc(strlen(s0) + strlen(s1) + 1 + 1);
        if (!cp) {
            perror("Failed");
            exit(1);
        }

        sprintf(cp, "%s,%s", s0, s1);
    } else {
        cp = strdup(s1);
    }

    if (s0)
        free(s0);
    return cp;
}

static unsigned long opt2flags(char ** options)
{
    const char delim[] = ",";
    char * option;
    char * lasts;
    char * remains = NULL;
    unsigned long flags = 0;

    option = strtok_r(*options, delim, &lasts);
    if (!option)
        return flags;

    do {
        int match = 0;

        for (size_t i = 0; i < num_elem(optnames); i++) {
            struct opt * o = optnames + i;

            if (strcmp(o->optname, option))
                continue;

            flags |= o->opt;
            match = 1;
            break;
        }
        if (!match)
            remains = catopt(remains, option);
    } while ((option = strtok_r(NULL, delim, &lasts)));

    *options = remains;
    return flags;
}
