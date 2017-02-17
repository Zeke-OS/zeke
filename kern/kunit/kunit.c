/**
 * @file kunit.c
 * @brief KUnit, a testing framework for Zeke.
 *
 * Inspired by: http://www.jera.com/techinfo/jtns/jtn002.html
 */

/* Copyright (c) 2013, 2014, 2016, 2017
 *               Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <fs/procfs.h>
#include <fs/procfs_dbgfile.h>
#include "kunit.h"

__GLOBL(__start_set_kunit_test_module_sect);
__GLOBL(__stop_set_kunit_test_module_sect);
extern struct _kunit_test_module __start_set_kunit_test_module_sect;
extern struct _kunit_test_module __stop_set_kunit_test_module_sect;

/* Variables below are documented in kunit.h */
int ku_tests_passed;
int ku_tests_skipped;
int ku_tests_count;

/**
 * Test module description.
 * @param str a test module description string.
 */
void ku_mod_description(char * str)
{
#if defined(configKU_REPORT_ORIENTED)
    printf("Test module: %s\n", str);
#endif
}

/**
 * Test case description.
 * @param str a test case description string.
 */
void ku_test_description(char * str)
{
#if defined(configKU_REPORT_ORIENTED)
    printf("\t%s\n", str);
#endif
}

/**
 * Run KUnit tests.
 * This should be called in main().
 * @param all_tests pointer to a function containing actual test calls.
 */
int ku_run_tests(void (*all_tests)(void))
{
    ku_tests_passed = 0;
    ku_tests_skipped = 0;
    ku_tests_count = 0;

    all_tests();
    if (ku_tests_passed == ku_tests_count) {
        kputs("ALL TESTS PASSED\n");
    }

    printf("Test passed: %d/%d, skipped: %d\n\n",
        ku_tests_passed, ku_tests_count, ku_tests_skipped);

    return (ku_tests_passed + ku_tests_skipped) != ku_tests_count;
}

static int kunit_run(char * name)
{
    struct _kunit_test_module * mod = &__start_set_kunit_test_module_sect;
    struct _kunit_test_module * stop = &__stop_set_kunit_test_module_sect;

    if (mod == stop)
        return -EINVAL;

    while (mod < stop) {
        if (strcmp(name, mod->name) == 0) {
            mod->fn();
            return 0;
        }

        mod++;
    }

    return -EINVAL;
}

static int read_kunit(void * buf, size_t max, void * elem)
{
    struct _kunit_test_module * mod = elem;

    return ksprintf(buf, max, "%s\n", mod->name);
}

static ssize_t write_kunit(const void * buf, size_t bufsize)
{
    int err;

    if (!strvalid((char *)buf, bufsize))
        return -EINVAL;

    err = kunit_run((char *)buf);
    if (err)
        return err;

    return bufsize;
}

PROCFS_DBGFILE(kunit,
               &__start_set_kunit_test_module_sect,
               &__stop_set_kunit_test_module_sect,
               read_kunit, write_kunit);
