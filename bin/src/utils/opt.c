#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../utils.h"

char * catopt(char * s0, char * s1)
{
    char * cp;

    if (s0 && *s0) {
        cp = malloc(strlen(s0) + strlen(s1) + 1 + 1);
        if (!cp)
            exit(1);

        sprintf(cp, "%s,%s", s0, s1);
    } else {
        cp = strdup(s1);
    }

    if (s0)
        free(s0);
    return cp;
}

unsigned long opt2flags(struct optarr * optnames, size_t n_elem,
                        char ** options)
{
    const char delim[] = ",";
    char * option;
    char * lasts;
    char * remains = NULL;
    unsigned long flags = 0;

    if ((*options)[0] == '\0') {
        *options = "";
        return 0;
    }

    option = strtok_r(*options, delim, &lasts);
    if (!option)
        return flags;

    do {
        int match = 0;

        for (size_t i = 0; i < n_elem; i++) {
            struct optarr * o = optnames + i;

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

