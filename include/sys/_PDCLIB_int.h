#ifndef __PDCLIB_INT_H
#define __PDCLIB_INT_H __PDCLIB_INT_H
#pragma clang system_header

/* PDCLib internal integer logic <_PDCLIB_int.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* -------------------------------------------------------------------------- */
/* You should not have to edit anything in this file; if you DO have to, it   */
/* would be considered a bug / missing feature: notify the author(s).         */
/* -------------------------------------------------------------------------- */

#include <sys/_PDCLIB_config.h>
#include <sys/_PDCLIB_aux.h>

/* -------------------------------------------------------------------------- */
/* Limits of native datatypes                                                 */
/* -------------------------------------------------------------------------- */
/* The definition of minimum limits for unsigned datatypes is done because    */
/* later on we will "construct" limits for other abstract types:              */
/* USHRT -> _PDCLIB_ + USHRT + _MIN -> _PDCLIB_USHRT_MIN -> 0                 */
/* INT -> _PDCLIB_ + INT + _MIN -> _PDCLIB_INT_MIN -> ... you get the idea.   */
/* -------------------------------------------------------------------------- */

/* <stdint.h> "fastest" types and their limits                                */
/* -------------------------------------------------------------------------- */
/* This is, admittedly, butt-ugly. But at least it's ugly where the average   */
/* user of PDCLib will never see it, and makes <_PDCLIB_config.h> much        */
/* cleaner.                                                                   */
/* -------------------------------------------------------------------------- */

typedef _PDCLIB_fast8          _PDCLIB_int_fast8_t;
typedef unsigned _PDCLIB_fast8 _PDCLIB_uint_fast8_t;
#define _PDCLIB_INT_FAST8_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST8 ), _MIN )
#define _PDCLIB_INT_FAST8_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST8 ), _MAX )
#define _PDCLIB_UINT_FAST8_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_FAST8 ), _MAX )

typedef _PDCLIB_fast16          _PDCLIB_int_fast16_t;
typedef unsigned _PDCLIB_fast16 _PDCLIB_uint_fast16_t;
#define _PDCLIB_INT_FAST16_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST16 ), _MIN )
#define _PDCLIB_INT_FAST16_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST16 ), _MAX )
#define _PDCLIB_UINT_FAST16_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_FAST16 ), _MAX )

typedef _PDCLIB_fast32          _PDCLIB_int_fast32_t;
typedef unsigned _PDCLIB_fast32 _PDCLIB_uint_fast32_t;
#define _PDCLIB_INT_FAST32_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST32 ), _MIN )
#define _PDCLIB_INT_FAST32_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST32 ), _MAX )
#define _PDCLIB_UINT_FAST32_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_FAST32 ), _MAX )

typedef _PDCLIB_fast64          _PDCLIB_int_fast64_t;
typedef unsigned _PDCLIB_fast64 _PDCLIB_uint_fast64_t;
#define _PDCLIB_INT_FAST64_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST64 ), _MIN )
#define _PDCLIB_INT_FAST64_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_FAST64 ), _MAX )
#define _PDCLIB_UINT_FAST64_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_FAST64 ), _MAX )

/* -------------------------------------------------------------------------- */
/* Various <stddef.h> typedefs and limits                                     */
/* -------------------------------------------------------------------------- */

typedef _PDCLIB_ptrdiff     _PDCLIB_ptrdiff_t;
#define _PDCLIB_PTRDIFF_MIN _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_PTRDIFF ), _MIN )
#define _PDCLIB_PTRDIFF_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_PTRDIFF ), _MAX )

#define _PDCLIB_SIG_ATOMIC_MIN _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_SIG_ATOMIC ), _MIN )
#define _PDCLIB_SIG_ATOMIC_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_SIG_ATOMIC ), _MAX )

typedef __SIZE_TYPE__     _PDCLIB_size_t;
#define _PDCLIB_SIZE_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_SIZE ), _MAX )

typedef _PDCLIB_wint      _PDCLIB_wint_t;
#ifndef __cplusplus
    typedef _PDCLIB_wchar     _PDCLIB_wchar_t;
#else
    typedef wchar_t _PDCLIB_wchar_t;
#endif
#define _PDCLIB_WCHAR_MIN _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_WCHAR ), _MIN )
#define _PDCLIB_WCHAR_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_WCHAR ), _MAX )

typedef _PDCLIB_intptr          _PDCLIB_intptr_t;
typedef unsigned _PDCLIB_intptr _PDCLIB_uintptr_t;
#define _PDCLIB_INTPTR_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_INTPTR ), _MIN )
#define _PDCLIB_INTPTR_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_INTPTR ), _MAX )
#define _PDCLIB_UINTPTR_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_INTPTR ), _MAX )

typedef _PDCLIB_intmax          _PDCLIB_intmax_t;
typedef unsigned _PDCLIB_intmax _PDCLIB_uintmax_t;
#define _PDCLIB_INTMAX_MIN  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_INTMAX ), _MIN )
#define _PDCLIB_INTMAX_MAX  _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_, _PDCLIB_INTMAX ), _MAX )
#define _PDCLIB_UINTMAX_MAX _PDCLIB_concat( _PDCLIB_concat( _PDCLIB_U, _PDCLIB_INTMAX ), _MAX )
#define _PDCLIB_INTMAX_C( value )  _PDCLIB_concat( value, _PDCLIB_INTMAX_LITERAL )
#define _PDCLIB_UINTMAX_C( value ) _PDCLIB_concat( value, _PDCLIB_concat( u, _PDCLIB_INTMAX_LITERAL ) )

/* -------------------------------------------------------------------------- */
/* Various <time.h> internals                                                 */
/* -------------------------------------------------------------------------- */

typedef _PDCLIB_time            _PDCLIB_time_t;
typedef _PDCLIB_clock           _PDCLIB_clock_t;

#if !defined(_PDCLIB_DEFINE_STRUCT_TIMESPEC)
#define _PDCLIB_DEFINE_STRUCT_TIMESPEC()    \
    struct timespec {                       \
        time_t tv_sec;                      \
        long tv_nsec;                       \
    };
#endif

#if !defined(_PDCLIB_DEFINE_STRUCT_TM)
#define _PDCLIB_DEFINE_STRUCT_TM()          \
    struct tm {                             \
        int tm_sec;                         \
        int tm_min;                         \
        int tm_hour;                        \
        int tm_mday;                        \
        int tm_mon;                         \
        int tm_year;                        \
        int tm_wday;                        \
        int tm_yday;                        \
        int tm_isdst;                       \
    };
#endif

/* -------------------------------------------------------------------------- */
/* Internal data types                                                        */
/* -------------------------------------------------------------------------- */

/* Structure required by both atexit() and exit() for handling atexit functions */
struct _PDCLIB_exitfunc_t
{
    struct _PDCLIB_exitfunc_t * next;
    void (*func)( void );
};

/* -------------------------------------------------------------------------- */
/* Declaration of helper functions (implemented in functions/_PDCLIB).        */
/* -------------------------------------------------------------------------- */

/* This is the main function called by atoi(), atol() and atoll().            */
_PDCLIB_intmax_t _PDCLIB_atomax( const char * s );

/* Two helper functions used by strtol(), strtoul() and long long variants.   */
const char * _PDCLIB_strtox_prelim( const char * p, char * sign, int * base );
_PDCLIB_uintmax_t _PDCLIB_strtox_main( const char ** p, unsigned int base, _PDCLIB_uintmax_t error, _PDCLIB_uintmax_t limval, int limdigit, char * sign );

/* Digits arrays used by various integer conversion functions */
extern char _PDCLIB_digits[];
extern char _PDCLIB_Xdigits[];

/* -------------------------------------------------------------------------- */
/* locale / wchar / uchar                                                     */
/* -------------------------------------------------------------------------- */

typedef unsigned __INT16_TYPE__ _PDCLIB_char16_t;
typedef unsigned __INT32_TYPE__ _PDCLIB_char32_t;

typedef struct _PDCLIB_mbstate {
    union {
        /* Is this the best way to represent this? Is this big enough? */
        unsigned __INT64_TYPE__     _St64[15];
        unsigned __INT32_TYPE__     _St32[31];
        unsigned __INT16_TYPE__     _St16[62];
        unsigned char               _StUC[124];
        signed   char               _StSC[124];
        char                        _StC [124];
    };

    /* c16/related functions: Surrogate storage
     *
     * If zero, no surrogate pending. If nonzero, surrogate.
     */
    unsigned __INT16_TYPE__ _Surrogate;

    /* In cases where the underlying codec is capable of regurgitating a
     * character without consuming any extra input (e.g. a surrogate pair in a
     * UCS-4 to UTF-16 conversion) then these fields are used to track that
     * state. In particular, they are used to buffer/fake the input for mbrtowc
     * and similar functions.
     *
     * See _PDCLIB_encoding.h for values of _PendState and the resultant value
     * in _PendChar.
     */
    unsigned char _PendState;
             char _PendChar;
} _PDCLIB_mbstate_t;

typedef struct _PDCLIB_charcodec *_PDCLIB_charcodec_t;
typedef struct _PDCLIB_locale    *_PDCLIB_locale_t;
typedef struct lconv              _PDCLIB_lconv_t;

_PDCLIB_size_t _PDCLIB_mb_cur_max( void );

/* -------------------------------------------------------------------------- */
/* stdio                                                                      */
/* -------------------------------------------------------------------------- */

/* Position / status structure for getpos() / fsetpos(). */
typedef struct _PDCLIB_fpos
{
    _PDCLIB_int_fast64_t offset; /* File position offset */
    _PDCLIB_mbstate_t    mbs;    /* Multibyte parsing state */
} _PDCLIB_fpos_t;

typedef struct _PDCLIB_fileops  _PDCLIB_fileops_t;
typedef union  _PDCLIB_fd       _PDCLIB_fd_t;
typedef struct _PDCLIB_file     _PDCLIB_file_t; // Rename to _PDCLIB_FILE?

/* Status structure required by _PDCLIB_print(). */
struct _PDCLIB_status_t
{
    /* XXX This structure is horrible now. scanf needs its own */

    int              base;   /* base to which the value shall be converted   */
    _PDCLIB_int_fast32_t flags; /* flags and length modifiers                */
    unsigned         n;      /* print: maximum characters to be written (snprintf) */
                             /* scan:  number matched conversion specifiers  */
    unsigned         i;      /* number of characters read/written            */
    unsigned         current;/* chars read/written in the CURRENT conversion */
    unsigned         width;  /* specified field width                        */
    int              prec;   /* specified field precision                    */

    union {
        void *           ctx;    /* context for callback */
        const char *     s;      /* input string for scanf */
    };

    union {
        _PDCLIB_size_t ( *write ) ( void *p, const char *buf, _PDCLIB_size_t size );
        _PDCLIB_file_t *stream;  /* for scanf */
    };
    _PDCLIB_va_list  arg;    /* argument stack                               */
};

#endif
