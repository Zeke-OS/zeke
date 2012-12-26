#include <stdio.h>
#include "minunit.h"

int tests_run = 0;

int mu_run_tests(char * (*all_tests)(void))
{
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n\n", tests_run);

    return result != 0;
}
