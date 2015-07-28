/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include "fs.h"
#include "util.h"

static char  *modestr = "";
static mode_t mask    = 0;
static int    ret     = 0;

static void
chmodr(const char *path, struct stat *st, void *data, struct recursor *r)
{
	mode_t m;

	m = parsemode(modestr, st ? st->st_mode : 0, mask);
	if (chmod(path, m) < 0) {
		weprintf("chmod %s:", path);
		ret = 1;
	} else if (st && S_ISDIR(st->st_mode)) {
		recurse(path, NULL, r);
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-R [-H | -L | -P]] mode file ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct recursor r = { .fn = chmodr, .hist = NULL, .depth = 0, .maxdepth = 1,
	                      .follow = 'P', .flags = 0 };
	size_t i;

	argv0 = argv[0], argc--, argv++;

	for (; *argv && (*argv)[0] == '-'; argc--, argv++) {
		if (!(*argv)[1])
			usage();
		for (i = 1; (*argv)[i]; i++) {
			switch ((*argv)[i]) {
			case 'R':
				r.maxdepth = 0;
				break;
			case 'H':
			case 'L':
			case 'P':
				r.follow = (*argv)[i];
				break;
			case 'r': case 'w': case 'x': case 's': case 't':
				/* -[rwxst] are valid modes, so we're done */
				if (i == 1)
					goto done;
				/* fallthrough */
			case '-':
				/* -- terminator */
				if (i == 1 && !(*argv)[i + 1]) {
					argv++;
					argc--;
					goto done;
				}
				/* fallthrough */
			default:
				usage();
			}
		}
	}
done:
	mask = getumask();
	modestr = *argv;

	if (argc < 2)
		usage();

	for (--argc, ++argv; *argv; argc--, argv++)
		recurse(*argv, NULL, &r);

	return ret || recurse_status;
}
