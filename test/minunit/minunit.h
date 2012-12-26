 /* @file minunit.h
  * http://www.jera.com/techinfo/jtns/jtn002.html
  */
 #define mu_assert(message, test) do { if (!(test)) return message; } while (0)
 #define mu_run_test(test) do { char *message = test(); tests_run++; \
                                if (message) return message; } while (0)
extern int tests_run;

#define MU_TEST_BUILD 1 /*!< This definition can be used to exclude included
                         * files and  souce code that are not needed for unit
                         * tests. */

int mu_run_tests(char * (*all_tests)(void));
