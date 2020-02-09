KUnit
=====

KUnit is a PUnit based portable unit testing framework that is modified to allow
running unit tests in kernel space, called in-kernel unit testing.

When kunit is compiled with the kernel tests are exposed in sysctl MIB tree
under debug.test. Any test can be invoked by writing a value other than zero
to the corresponding variable in MIB. While test is executing the user thread
is blocked until test tests have been run. Test results are written at
"realtime" to the kernel logger.

KUnit was originally inspired by: http://www.jera.com/techinfo/jtns/jtn002.html


How to implement a new test group
---------------------------------

TODO


Assertions
----------

+ `ku_assert(message, test)` - Checks if boolean value of test is true
+ `ku_assert_equal(message, left, right)` - Checks if `left == right` is true
+ `ku_assert_ptr_equal(message, left, right)` - Checks if left and right
  pointers are equal
+ `ku_assert_str_equal(message, left, right)` - Checks if left and right
  strings are equal (strcmp)
+ `ku_assert_array_equal(message, left, right, size)` - Asserts that each
  integer element i of two arrays are equal (strcmp).
+ `ku_assert_str_array_equal(message, left, right, size)` - Asserts that each
  string element i of two arrays are equal (==).
+ `ku_assert_null(message, ptr)` - Asserts that a pointer is null.
+ `ku_assert_not_null(message, ptr)` - Asserts that a pointer isn't null.
+ `ku_assert_fail(message)` - Always fails


License 
-------

Copyright (c) 2013 - 2015, Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

