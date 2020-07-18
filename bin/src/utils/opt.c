#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../utils.h"

char * catopt(char * s0, const char * s1)
{
    char * cp;
    size_t s0_len;

    s0_len = s0 ? strlen(s0) : 0;
    cp = realloc(s0, s0_len + strlen(s1) + 2);
    if (s0_len > 0) {
        cp[s0_len] = ',';
        strcpy(cp + s0_len + 1, s1);
    } else {
        strcpy(cp, s1);
    }

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

    if (*options == NULL || (*options)[0] == '\0') {
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

