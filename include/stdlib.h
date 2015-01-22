/* 7.20 General utilities <stdlib.h>
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

/**
 * @addtogroup LIBC
 * @{
  */

#ifndef _PDCLIB_STDLIB_H
#define _PDCLIB_STDLIB_H _PDCLIB_STDLIB_H

#include <sys/_PDCLIB_int.h>
__BEGIN_DECLS

#if !defined(_SIZE_T)
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#include <sys/_null.h>

#ifndef _PDCLIB_MB_CUR_MAX_DEFINED
#define _PDCLIB_MB_CUR_MAX_DEFINED
#define MB_CUR_MAX (_PDCLIB_mb_cur_max())
#endif

/* Numeric conversion functions */

/* TODO: atof(), strtof(), strtod(), strtold() */

double atof( const char * nptr ) _PDCLIB_nothrow;
double strtod( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr ) _PDCLIB_nothrow;
float strtof( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr ) _PDCLIB_nothrow;
long double strtold( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr ) _PDCLIB_nothrow;

/* Seperate the character array nptr into three parts: A (possibly empty)
   sequence of whitespace characters, a character representation of an integer
   to the given base, and trailing invalid characters (including the terminating
   null character). If base is 0, assume it to be 10, unless the integer
   representation starts with 0x / 0X (setting base to 16) or 0 (setting base to
   8). If given, base can be anything from 0 to 36, using the 26 letters of the
   base alphabet (both lowercase and uppercase) as digits 10 through 35.
   The integer representation is then converted into the return type of the
   function. It can start with a '+' or '-' sign. If the sign is '-', the result
   of the conversion is negated.
   If the conversion is successful, the converted value is returned. If endptr
   is not a NULL pointer, a pointer to the first trailing invalid character is
   returned in *endptr.
   If no conversion could be performed, zero is returned (and nptr in *endptr,
   if endptr is not a NULL pointer). If the converted value does not fit into
   the return type, the functions return LONG_MIN, LONG_MAX, ULONG_MAX,
   LLONG_MIN, LLONG_MAX, or ULLONG_MAX respectively, depending on the sign of
   the integer representation and the return type, and errno is set to ERANGE.
*/
/* There is strtoimax() and strtoumax() in <inttypes.h> operating on intmax_t /
   uintmax_t, if the long long versions do not suit your needs.
*/
long int strtol( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr, int base ) _PDCLIB_nothrow;
long long int strtoll( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr, int base ) _PDCLIB_nothrow;
unsigned long int strtoul( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr, int base ) _PDCLIB_nothrow;
unsigned long long int strtoull( const char * _PDCLIB_restrict nptr, char * * _PDCLIB_restrict endptr, int base ) _PDCLIB_nothrow;

/* These functions are the equivalent of (int)strtol( nptr, NULL, 10 ),
   strtol( nptr, NULL, 10 ) and strtoll(nptr, NULL, 10 ) respectively, with the
   exception that they do not have to handle overflow situations in any defined
   way.
   (PDCLib does not simply forward these to their strtox() equivalents, but
   provides a simpler atox() function that saves a couple of tests and simply
   continues with the conversion in case of overflow.)
*/
int atoi( const char * nptr ) _PDCLIB_nothrow;
long int atol( const char * nptr ) _PDCLIB_nothrow;
long long int atoll( const char * nptr ) _PDCLIB_nothrow;

/* Pseudo-random sequence generation functions */

extern unsigned long int _PDCLIB_seed;

#define RAND_MAX 32767

/* Returns the next number in a pseudo-random sequence, which is between 0 and
   RAND_MAX.
   (PDCLib uses the implementation suggested by the standard document, which is
   next = next * 1103515245 + 12345; return (unsigned int)(next/65536) % 32768;)
*/
int rand( void ) _PDCLIB_nothrow;

/* Initialize a new pseudo-random sequence with the starting seed. Same seeds
   result in the same pseudo-random sequence. The default seed is 1.
*/
void srand( unsigned int seed ) _PDCLIB_nothrow;

/* Memory management functions */

/* Allocate a chunk of heap memory of given size. If request could not be
   satisfied, return NULL. Otherwise, return a pointer to the allocated
   memory. Memory contents are undefined.
*/
void * malloc( size_t size ) _PDCLIB_nothrow;

/* Allocate a chunk of heap memory that is large enough to hold nmemb elements
   of the given size, and zero-initialize that memory. If request could not be
   satisfied, return NULL. Otherwise, return a pointer to the allocated
   memory.
*/
void * calloc( size_t nmemb, size_t size ) _PDCLIB_nothrow;

/* De-allocate a chunk of heap memory previously allocated using malloc(),
   calloc(), or realloc(), and pointed to by ptr. If ptr does not match a
   pointer previously returned by the mentioned allocation functions, or
   free() has already been called for this ptr, behaviour is undefined.
*/
void free( void * ptr ) _PDCLIB_nothrow;

/* Resize a chunk of memory previously allocated with malloc() and pointed to
   by ptr to the given size (which might be larger or smaller than the original
   size). Returns a pointer to the reallocated memory, or NULL if the request
   could not be satisfied. Note that the resizing might include a memcpy()
   from the original location to a different one, so the return value might or
   might not equal ptr. If size is larger than the original size, the value of
   memory beyond the original size is undefined. If ptr is NULL, realloc()
   behaves like malloc().
*/
void * realloc( void * ptr, size_t size ) _PDCLIB_nothrow;

/* Communication with the environment */

/* These two can be passed to exit() or _Exit() as status values, to signal
   successful and unsuccessful program termination, respectively. EXIT_SUCCESS
   can be replaced by 0. How successful or unsuccessful program termination are
   signaled to the environment, and what happens if exit() or _Exit() are being
   called with a value that is neither of the three, is defined by the hosting
   OS and its glue function.
*/
#define EXIT_SUCCESS _PDCLIB_SUCCESS
#define EXIT_FAILURE _PDCLIB_FAILURE

/* Initiate abnormal process termination, unless programm catches SIGABRT and
   does not return from the signal handler.
   This implementantion flushes all streams, closes all files, and removes any
   temporary files before exiting with EXIT_FAILURE.
   abort() does not return.
*/
_PDCLIB_noreturn void abort( void ) _PDCLIB_nothrow;

/* Register a function that will be called on exit(), or when main() returns.
   At least 32 functions can be registered this way, and will be called in
   reverse order of registration (last-in, first-out).
   Returns zero if registration is successfull, nonzero if it failed.
*/
int atexit( void (*func)( void ) ) _PDCLIB_nothrow;

/* Normal process termination. Functions registered by atexit() (see above) are
   called, streams flushed, files closed and temporary files removed before the
   program is terminated with the given status. (See comment for EXIT_SUCCESS
   and EXIT_FAILURE above.)
   exit() does not return.
*/
_PDCLIB_noreturn void exit( int status ) _PDCLIB_nothrow;

/* Normal process termination. Functions registered by atexit() (see above) are
   NOT CALLED. This implementation DOES flush streams, close files and removes
   temporary files before the program is teminated with the given status. (See
   comment for EXIT_SUCCESS and EXIT_FAILURE above.)
   _Exit() does not return.
*/
_PDCLIB_noreturn void _Exit( int status ) _PDCLIB_nothrow;

/* If string is a NULL pointer, system() returns nonzero if a command processor
   is available, and zero otherwise. If string is not a NULL pointer, it is
   passed to the command processor. If system() returns, it does so with a
   value that is determined by the hosting OS and its glue function.
*/
int system( const char * string ) _PDCLIB_nothrow;

/* Searching and sorting */

/* Do a binary search for a given key in the array with a given base pointer,
   which consists of nmemb elements that are of the given size each. To compare
   the given key with an element from the array, the given function compar is
   called (with key as first parameter and a pointer to the array member as
   second parameter); the function should return a value less than, equal to,
   or greater than 0 if the key is considered to be less than, equal to, or
   greater than the array element, respectively.
   The function returns a pointer to the first matching element found, or NULL
   if no match is found.

   ** May throw **
*/
void * bsearch( const void * key, const void * base, size_t nmemb, size_t size, int (*compar)( const void *, const void * ) );

/* Do a quicksort on an array with a given base pointer, which consists of
   nmemb elements that are of the given size each. To compare two elements from
   the array, the given function compar is called, which should return a value
   less than, equal to, or greater than 0 if the first argument is considered
   to be less than, equal to, or greater than the second argument, respectively.
   If two elements are compared equal, their order in the sorted array is not
   specified.

   ** May throw **
*/
void qsort( void * base, size_t nmemb, size_t size, int (*compar)( const void *, const void * ) );

/* Integer arithmetic functions */

/* Return the absolute value of the argument. Note that on machines using two-
   complement's notation (most modern CPUs), the largest negative value cannot
   be represented as positive value. In this case, behaviour is unspecified.
*/
int abs( int j ) _PDCLIB_nothrow;
long int labs( long int j ) _PDCLIB_nothrow;
long long int llabs( long long int j ) _PDCLIB_nothrow;

/* These structures each have a member quot and a member rem, of type int (for
   div_t), long int (for ldiv_t) and long long it (for lldiv_t) respectively.
   The order of the members is platform-defined to allow the div() functions
   below to be implemented efficiently.
*/
typedef struct _PDCLIB_div_t     div_t;
typedef struct _PDCLIB_ldiv_t   ldiv_t;
typedef struct _PDCLIB_lldiv_t lldiv_t;

/* Return quotient (quot) and remainder (rem) of an integer division in one of
   the structs above.
*/
div_t div( int numer, int denom ) _PDCLIB_nothrow;
ldiv_t ldiv( long int numer, long int denom ) _PDCLIB_nothrow;
lldiv_t lldiv( long long int numer, long long int denom ) _PDCLIB_nothrow;

/* TODO: Multibyte / wide character conversion functions */

/* TODO: Macro MB_CUR_MAX */

/*
int mblen( const char * s, size_t n );
int mbtowc( wchar_t * _PDCLIB_restrict pwc, const char * _PDCLIB_restrict s, size_t n );
int wctomb( char * s, wchar_t wc );
size_t mbstowcs( wchar_t * _PDCLIB_restrict pwcs, const char * _PDCLIB_restrict s, size_t n );
size_t wcstombs( char * _PDCLIB_restrict s, const wchar_t * _PDCLIB_restrict pwcs, size_t n );
*/

/* Environ */
char * getenv(const char * name);
int setenv(const char * name, const char * value, int rewrite);
int unsetenv(const char * name);

/*
 * pty
 */

/**
 * Open a pty device.
 */
int posix_openpt(int flags);

/**
 * Grant access to the slave pty device.
 */
int grantpt(int fildes);

int unlockpt(int fildes);
char * ptsname(int fildes);

__END_DECLS
#endif

/**
 * @}
 */
