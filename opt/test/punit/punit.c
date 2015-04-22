/**
 * @file punit.c
 * @brief PUnit, a portable unit testing framework for C.
 *
 * Inspired by: http://www.jera.com/techinfo/jtns/jtn002.html
 */

/* Copyright (c) 2013, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @addtogroup PUnit
 * @{
 */

#include <stdio.h>
#include "punit.h"

/* Variables below are documented in punit.h */
int pu_tests_passed = 0;
int pu_tests_skipped = 0;
int pu_tests_count = 0;

/**
 * Test module description.
 * @param str a test module description string.
 */
void pu_mod_description(char * str)
{
#if PU_REPORT_ORIENTED == 1
    printf("Test module: %s\n", str);
#endif
}

/**
 * Test case description.
 * @param str a test case description string.
 */
void pu_test_description(char * str)
{
#if PU_REPORT_ORIENTED == 1
    printf("\t%s\n", str);
#endif
}

/**
 * Run PUnit tests.
 * This should be called in main().
 * @param all_tests pointer to a function containing actual test calls.
 */
int pu_run_tests(void (*all_tests)(void))
{
    all_tests();
    if (pu_tests_passed == pu_tests_count) {
        printf("ALL TESTS PASSED\n");
    }

    printf("Test passed: %d/%d, skipped: %d\n\n",
        pu_tests_passed, pu_tests_count, pu_tests_skipped);

    return (pu_tests_passed + pu_tests_skipped) != pu_tests_count;
}

/**
 * @}
 */
