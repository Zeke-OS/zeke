/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../fs.h"
#include "../util.h"

int recurse_status = 0;

void
recurse(const char *path, void *data, struct recursor *r)
{
	struct dirent *d;
	struct history *new, *h;
	struct stat st, dst;
	DIR *dp;
	int (*statf)(const char *, struct stat *);
	char subpath[PATH_MAX], *statf_name;

	if (r->follow == 'P' || (r->follow == 'H' && r->depth)) {
		statf_name = "lstat";
		statf = lstat;
	} else {
		statf_name = "stat";
		statf = stat;
	}

	if (statf(path, &st) < 0) {
		if (!(r->flags & SILENT)) {
			weprintf("%s %s:", statf_name, path);
			recurse_status = 1;
		}
		return;
	}
	if (!S_ISDIR(st.st_mode)) {
		(r->fn)(path, &st, data, r);
		return;
	}

	new = emalloc(sizeof(struct history));
	new->prev  = r->hist;
	r->hist    = new;
	new->dev   = st.st_dev;
	new->ino   = st.st_ino;

	for (h = new->prev; h; h = h->prev)
		if (h->ino == st.st_ino && h->dev == st.st_dev)
			return;

	if (!(dp = opendir(path))) {
		if (!(r->flags & SILENT)) {
			weprintf("opendir %s:", path);
			recurse_status = 1;
		}
		return;
	}

	if (!r->depth && (r->flags & DIRFIRST))
		(r->fn)(path, &st, data, r);

	if (!r->maxdepth || r->depth + 1 < r->maxdepth) {
		while ((d = readdir(dp))) {
			if (r->follow == 'H') {
				statf_name = "lstat";
				statf = lstat;
			}
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
			estrlcpy(subpath, path, sizeof(subpath));
			if (path[strlen(path) - 1] != '/')
				estrlcat(subpath, "/", sizeof(subpath));
			estrlcat(subpath, d->d_name, sizeof(subpath));
			if (statf(subpath, &dst) < 0) {
				if (!(r->flags & SILENT)) {
					weprintf("%s %s:", statf_name, subpath);
					recurse_status = 1;
				}
			} else if ((r->flags & SAMEDEV) && dst.st_dev != st.st_dev) {
				continue;
			} else {
				r->depth++;
				(r->fn)(subpath, &dst, data, r);
				r->depth--;
			}
		}
	}

	if (!r->depth) {
		if (!(r->flags & DIRFIRST))
			(r->fn)(path, &st, data, r);

		for (; r->hist; ) {
			h = r->hist;
			r->hist = r->hist->prev;
			free(h);
		}
	}

	closedir(dp);
}
