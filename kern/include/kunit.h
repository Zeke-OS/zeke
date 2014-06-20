/**
 * @file kunit.h
 * @brief KUnit, a unit test framework for Zeke.
 *
 * Inspired by: http://www.jera.com/techinfo/jtns/jtn002.html
 */

/* Copyright (c) 2013, 2014, Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup KUnit
  * @{
  */

#include <autoconf.h>

#if configKUNIT == 0
#error kunit is required for tests
#endif

#pragma once
#ifndef KUNIT_H
#define KUNIT_H

#define KERNEL_INTERNAL
#include <kstring.h>
#include <kerror.h>
#include <sys/sysctl.h>

#define printf(fmt, ...) do { char msgbuf[80];              \
    ksprintf(msgbuf, sizeof(msgbuf), (fmt), __VA_ARGS__);   \
    kputs(msgbuf);                                          \
} while (0)

/**
 * Assert condition.
 * Checks if boolean value of test is true.
 * @param message shown if assert fails.
 * @param test condition, also shown if assert fails.
 */
#define ku_assert(message, test) do { if (!(test)) { \
        printf("FAILED: %s:%d: (%s)\n",              \
            __FILE__, __LINE__, #test);              \
        return message; }                            \
} while (0)
/** \example test_example.c
 * This is an example of how to use the pu_assert.
 */

/**
 * Assert equal.
 * Checks if left == right is true.
 * @param message shown if assert fails.
 * @param left value.
 * @param right value.
 */
#define ku_assert_equal(message, left, right) do { if (!(left == right)) { \
        printf("FAILED: %s:%d: %s == %s\n\tleft:\t%i\n\tright:\t%i\n",     \
            __FILE__, __LINE__, #left, #right, left, right);               \
        return message; }                                                  \
} while(0)
/** \example test_equal.c
 * This is an example of how to use the pu_assert_equal.
 */

/**
 * Assert pointer equal.
 * Checks if left == right is true.
 * @param message shown if assert fails.
 * @param left value.
 * @param right value.
 */
#define ku_assert_ptr_equal(message, left, right) do { if (!(left == right)) { \
        printf("FAILED: %s:%d: %s == %s\n\tleft:\t%i\n\tright:\t%i\n",         \
            __FILE__, __LINE__, #left, #right, (int)(left), (int)(right));     \
        return message; }                                                      \
} while(0)

/**
 * String equal.
 * Checks if left and right strings are equal (strcmp).
 * @param message shown if assert fails.
 * @param left null-terminated string.
 * @param right null-terminated string.
 */
#define ku_assert_str_equal(message, left, right) do {       \
    if (strcmp(left, right) != 0) {                          \
        printf("FAILED: %s:%d: %s equals %s\n\tleft:\t\"%s\"\n\tright:\t\"%s\"\n", \
            __FILE__, __LINE__, #left, #right, left, right); \
    return message; }                                        \
} while (0)
/** \example test_strings.c
 * This is an example of how to use the pu_assert_str_equal.
 */

/**
 * Assert integer arrays are equal.
 * Asserts that each integer element i of two arrays are equal (==).
 * @param message shown if assert fails.
 * @param left array.
 * @param right array.
 * @param size of the array tested.
 */
#define ku_assert_array_equal(message, left, right, size) do {    \
    int i;                                                        \
    for (i = 0; i < (int)(size); i++) {                           \
        if (!(left[i] == right[i])) {                             \
            printf("FAILED: %s:%d: integer array %s equals %s\n", \
                __FILE__, __LINE__, #left, #right);               \
            printf("\tleft[%i]:\t%i\n\tright[%i]:\t%i\n",         \
                i, left[i], i, right[i]);                         \
            return message; }                                     \
    }                                                             \
} while(0)
/** \example test_arrays.c
 * This is an example of how to use the pu_assert_array_equal.
 */

/**
 * Assert string arrays are equal.
 * Asserts that each string element i of two arrays are equal (strcmp).
 * @param message shown if assert fails.
 * @param left array of strings.
 * @param right array of strings.
 * @param size of the array tested.
 */
#define ku_assert_str_array_equal(message, left, right, size) do { \
    int i;                                                         \
    for (i = 0; i < (int)(size); i++) {                            \
        if (strcmp(left[i], right[i]) != 0) {                      \
            printf("FAILED: %s:%d: string array %s equals %s\n",   \
                __FILE__, __LINE__, #left, #right);                \
            printf("\tleft[%i]:\t\"%s\"\n\tright[%i]:\t\"%s\"\n",  \
                i, left[i], i, right[i]);                          \
            return message; }                                      \
    }                                                              \
} while(0)
/** \example test_strarrays.c
 * This is an example of how to use the pu_assert_str_array_equal.
 */

/**
 * Assert NULL.
 * Asserts that a pointer is null.
 * @param message shown if assert fails.
 * @param ptr a pointer variable.
 */
#define ku_assert_null(message, ptr) do { if ((void *)ptr) { \
        printf("FAILED: %s:%d: %s should be NULL\n",         \
            __FILE__, __LINE__, #ptr);                       \
        return message; }                                    \
} while (0)
/** \example test_null.c
 * This is an example of how to use the pu_assert_null.
 */

/**
 * Assert not NULL.
 * Asserts that a pointer isn't null.
 * @param message shown if assert fails.
 * @param ptr a pointer variable.
 */
#define ku_assert_not_null(message, ptr) do { if (!((void *)ptr)) { \
        printf("FAILED: %s:%d: %s should not be NULL\n",            \
            __FILE__, __LINE__, #ptr);                              \
        return message; }                                           \
} while (0)
/** \example test_null.c
 * This is an example of how to use the pu_assert_not_null.
 */


/**
 * Assert fail.
 * Always fails.
 * @param message that is shown.
 */
#define ku_assert_fail(message) do { kputs("FAILED: Assert fail\n");    \
    return message;                                                     \
} while (0)

#define KU_RUN  1 /*!< Marks that a particular test should be run. */
#define KU_SKIP 0 /*!< Marks that a particular test should be skipped. */

/**
 * Define and run test.
 * This is only used in all_tests() function to declare a test that should
 * be run.
 * @param test to be run.
 * @param run KU_RUN or KU_SKIP
 */
#define ku_def_test(test, run) do { char * message;             \
    char buf[80];                                               \
    if (run == KU_SKIP) {                                       \
        ksprintf(buf, sizeof(buf), "-%s, skipped\n", #test);    \
        kputs(buf);                                             \
        ku_tests_count++;                                       \
        ku_tests_skipped++;                                     \
        break;                                                  \
    }                                                           \
    ksprintf(buf, sizeof(buf), "-%s\n", #test);                 \
    kputs(buf);                                                 \
    setup();                                                    \
    message = test(); ku_tests_count++;                         \
    teardown();                                                 \
    if (message) {                                              \
        ksprintf(buf, sizeof(buf), "\t%s\n", message);          \
        kputs(buf);                                             \
    } else ku_tests_passed++;                                   \
} while (0)

/**
 * Run tests.
 * @deprecated Same as ku_def_test(test, KU_RUN).
 * @param test to be run.
 */
#define ku_run_test(test) pu_def_test(test, KU_RUN)

SYSCTL_DECL(_debug_test);

#define SYSCTL_TEST(group, tname) \
    int sysctl_test_##group_##tname(SYSCTL_HANDLER_ARGS) {                  \
    int ctl = 0, error;                                                     \
    error = sysctl_handle_int(oidp, &ctl, sizeof(ctl), req);                \
    if (!error && req->newptr && ctl) {                                     \
        ku_run_tests(&all_tests);                                           \
    }                                                                       \
    return error;                                                           \
}                                                                           \
SYSCTL_PROC(_debug_test_##group, OID_AUTO, tname, CTLTYPE_INT | CTLFLAG_RW, \
        NULL, 0, sysctl_test_##group_##tname, "I", "Unit test for " #tname ".")

extern int ku_tests_passed; /*!< Global tests passed counter. */
extern int ku_tests_skipped; /*! Global tests skipped counter */
extern int ku_tests_count; /*!< Global tests counter. */

/* Documented in kunit.c */
void ku_mod_description(char * str);
void ku_test_description(char * str);
int ku_run_tests(void (*all_tests)(void));

#endif /* KUNIT_H */

/**
  * @}
  */
