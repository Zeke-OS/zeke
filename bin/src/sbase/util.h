/* See LICENSE file for copyright and license details. */
#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>

#include "arg.h"

#define UTF8_POINT(c) (((c) & 0xc0) != 0x80)

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#undef LIMIT
#define LIMIT(x, a, b)  (x) = (x) < (a) ? (a) : (x) > (b) ? (b) : (x)

#define LEN(x) (sizeof (x) / sizeof *(x))

extern char *argv0;

void *ecalloc(size_t, size_t);
void *emalloc(size_t);
void *erealloc(void *, size_t);
char *estrdup(const char *);
char *estrndup(const char *, size_t);
void *encalloc(int, size_t, size_t);
void *enmalloc(int, size_t);
void *enrealloc(int, void *, size_t);
char *enstrdup(int, const char *);
char *enstrndup(int, const char *, size_t);

void enfshut(int, FILE *, const char *);
void efshut(FILE *, const char *);
int  fshut(FILE *, const char *);

void enprintf(int, const char *, ...);
void eprintf(const char *, ...);
void weprintf(const char *, ...);

double estrtod(const char *);

#undef strcasestr
char *strcasestr(const char *, const char *);

size_t estrlcat(char *, const char *, size_t);
size_t estrlcpy(char *, const char *, size_t);

/* misc */
void enmasse(int, char **, int (*)(const char *, const char *, int));
mode_t getumask(void);
char *humansize(off_t);
mode_t parsemode(const char *, mode_t, mode_t);
void putword(FILE *, const char *);
#undef strtonum
long long strtonum(const char *, long long, long long, const char **);
long long enstrtonum(int, const char *, long long, long long);
long long estrtonum(const char *, long long, long long);
