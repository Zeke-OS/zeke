PUnit   {#mainpage}
=====

PUnit, a portable unit testing framework for C.
Inspired by: http://www.jera.com/techinfo/jtns/jtn002.html

Complete documentation: http://ninjaware.github.com/punit/

Setting up the directory tree
-----------------------------

    .
    |-example_prj_target1   (Your test suite)
    |---bin
    |---obj
    |-example_prj_target2
    |---bin
    |---obj
    |-punit                 (PUnit)


How to implement a new test suite
---------------------------------

1. `cp -R example_prj name_of_your_project`
2. Update Makefile in your new project directory according to instructions in
   the file
3. Write new tests and name the test files in following manner:
   `test_<name>.c`
4. Write a per test makefile `test_<name>.mk` which specifies source file for
   the test.
5. Run `make`. If everything went well PUnit should automatically determine
   and build all needed source modules, build the tests and finally run the
   tests.


Assertions
----------

+ `pu_assert(message, test)` - Checks if boolean value of test is true
+ `pu_assert_equal(message, left, right)` - Checks if `left == right` is true
+ `pu_assert_ptr_equal(message, left, right)` - Checks if left and right
  pointers are equal
+ `pu_assert_str_equal(message, left, right)` - Checks if left and right
  strings are equal (strcmp)
+ `pu_assert_double_equal(message, left, right, delta)` - Checks if left and
  right doubles are appoximately equal
+ `pu_assert_array_equal(message, left, right, size)` - Asserts that each
  integer element i of two arrays are equal (strcmp).
+ `pu_assert_str_array_equal(message, left, right, size)` - Asserts that each
  string element i of two arrays are equal (==).
+ `pu_assert_null(message, ptr)` - Asserts that a pointer is null.
+ `pu_assert_not_null(message, ptr)` - Asserts that a pointer isn't null.
+ `pu_assert_fail(message)` - Always fails

See examples in: `examples/example_prj`


Mock functions
--------------

Mock functions can be quite useful especially when testing some OS modules or
embedded software.

    /* Override func */
    void my_func(void);
    void my_func(void) { }
    #define func my_func

    #include "func.h"

Note: Do not build func.c i.e. do not add it to the name of the test module.


License 
-------

Copyright (c) 2013, Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
Copyright (c) 2012, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

