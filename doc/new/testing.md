Testing
=======

PUnit
-----

PUnit is a portable unit test framework for C, it’s used for userland
testing in Zeke, mainly for testing libc functions and testing that some
kernel system call interfaces work properly when called from the user
space.

### Assertions

  - `pu_assert(message, test)` - Checks if boolean value of test is true

  - `pu_assert_equal(message, left, right)` - Checks if `left == right`
    is true

  - `pu_assert_ptr_equal(message, left, right)` - Checks if left and
    right pointers are equal

  - `pu_assert_str_equal(message, left, right)` - Checks if left and
    right strings are equal (strcmp)

  - `pu_assert_array_equal(message, left, right, size)` - Asserts that
    each integer element i of two arrays are equal (`strcmp()`)

  - `ku_assert_str_array_equal(message, left, right, size)` - Asserts
    that each string element i of two arrays are equal (`==`)

  - `pu_assert_null(message, ptr)` - Asserts that a pointer is null

  - `pu_assert_not_null(message, ptr)` - Asserts that a pointer isn’t
    null

  - `ku_assert_fail(message)` - Always fails

KUnit - Kernel Unit Test Framework
----------------------------------

KUnit is a PUnit based portable unit testing framework that is modified
to allow running unit tests in kernel space, called in-kernel unit
testing. This is done because it’s not easily possible to compile kernel
code to user space to link against with unit test code that could be run
in user space.

When kunit is compiled with the kernel tests are exposed in sysctl MIB
tree under debug.test. Any test can be invoked by writing a value other
than zero to the corresponding variable in MIB. While test is executing
the user thread is blocked until test tests have been run. Test results
are written at "realtime" to the kernel logger.

KUnit and unit tests are built into the kernel if `configKUNIT` flag is
set in config file. This creates a `debug.test` tree in sysctl
<span data-acronym-label="MIB" data-acronym-form="singular+short">MIB</span>.
Unit tests are executed by writing value other than zero to the
corresponding variable name in MIB. Tests results are then written to
the kernel log. Running thread is blocked until requested test has
returned.

### Assertions

  - `ku_assert(message, test)` - Checks if boolean value of test is true

  - `ku_assert_equal(message, left, right)` - Checks if `left == right`
    is true

  - `ku_assert_ptr_equal(message, left, right)` - Checks if left and
    right pointers are equal

  - `ku_assert_str_equal(message, left, right)` - Checks if left and
    right strings are equal (strcmp)

  - `ku_assert_array_equal(message, left, right, size)` - Asserts that
    each integer element i of two arrays are equal (`strcmp()`)

  - `ku_assert_str_array_equal(message, left, right, size)` - Asserts
    that each string element i of two arrays are equal (`==`)

  - `ku_assert_null(message, ptr)` - Asserts that a pointer is null

  - `ku_assert_not_null(message, ptr)` - Asserts that a pointer isn’t
    null

  - `ku_assert_fail(message)` - Always fails
