#include <grp.h>
#include <string.h>
#include <sys/types.h>

struct group * getgrnam(const char * name)
{
    register struct group *p;

    setgrent();
    while ((p = getgrent()) && strcmp(p->gr_name, name));
    endgrent();
    return p;
}
