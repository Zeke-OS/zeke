#include <stddef.h>
#include <ctype.h>
#include "zmodem.h"

/* make string s lower case */
void uncaps(char *s)
{
    for (; *s; ++s) {
        if (isupper(*s))
            *s = tolower(*s);
    }
}

/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
int IsAnyLower(char *s)
{
    for (; *s; ++s) {
        if (islower(*s))
            return TRUE;
    }
    return FALSE;
}

/*
 * substr(string, token) searches for token in string s
 * returns pointer to token within string if found, NULL otherwise
 */
char * substr(char *s, char *t)
{
    char *ss;
    char *tt;

    /* search for first char of token */
    for (ss = s; *s; s++) {
        if (*s == *t) {
            /* compare token with substring */
            for (ss = s, tt = t;;) {
                if (*tt == 0)
                    return s;
                if (*ss++ != *tt++)
                    break;
            }
        }
    }
    return NULL;
}
