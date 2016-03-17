/* 7.11 Localization <locale.h>
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef _PDCLIB_LOCALE_H
#define _PDCLIB_LOCALE_H _PDCLIB_LOCALE_H

#include <sys/_PDCLIB_int.h>
__BEGIN_DECLS

#include <sys/types/_null.h>

/* The structure returned by localeconv().
 *
 *  The values for *_sep_by_space:
 *  0 - no space
 *  1 - if symbol and sign are adjacent, a space seperates them from the value;
 *      otherwise a space seperates the symbol from the value
 *  2 - if symbol and sign are adjacent, a space seperates them; otherwise a
 *      space seperates the sign from the value
 *
 *  The values for *_sign_posn:
 *  0 - Parentheses surround value and symbol
 *  1 - sign precedes value and symbol
 *  2 - sign succeeds value and symbol
 *  3 - sign immediately precedes symbol
 *  4 - sign immediately succeeds symbol
*/
struct lconv
{
    char * decimal_point;      /* decimal point character                     */
    char * thousands_sep;      /* character for seperating groups of digits   */
    char * grouping;           /* string indicating the size of digit groups  */
    char * mon_decimal_point;  /* decimal point for monetary quantities       */
    char * mon_thousands_sep;  /* thousands_sep for monetary quantities       */
    char * mon_grouping;       /* grouping for monetary quantities            */
    char * positive_sign;      /* string indicating nonnegative mty. qty.     */
    char * negative_sign;      /* string indicating negative mty. qty.        */
    char * currency_symbol;    /* local currency symbol (e.g. '$')            */
    char * int_curr_symbol;    /* international currency symbol (e.g. "USD"   */
    char frac_digits;          /* fractional digits in local monetary qty.    */
    char p_cs_precedes;        /* if currency_symbol precedes positive qty.   */
    char n_cs_precedes;        /* if currency_symbol precedes negative qty.   */
    char p_sep_by_space;       /* if it is seperated by space from pos. qty.  */
    char n_sep_by_space;       /* if it is seperated by space from neg. qty.  */
    char p_sign_posn;          /* positioning of positive_sign for mon. qty.  */
    char n_sign_posn;          /* positioning of negative_sign for mon. qty.  */
    char int_frac_digits;      /* Same as above, for international format     */
    char int_p_cs_precedes;    /* Same as above, for international format     */
    char int_n_cs_precedes;    /* Same as above, for international format     */
    char int_p_sep_by_space;   /* Same as above, for international format     */
    char int_n_sep_by_space;   /* Same as above, for international format     */
    char int_p_sign_posn;      /* Same as above, for international format     */
    char int_n_sign_posn;      /* Same as above, for international format     */
};

/**
 * @addtogroup setlocale Set the application global locale
 * The ISO C standards define an application global locale, which all
 * locale-dependent functions implicitly use. setlocale() may be used
 * to change this locale.
 * @sa lconv(3) uselocale(3) duplocale(3) freelocale(3)
 * @since Conforming to
 *        ISO/IEC 9899/AMD1:1995 ("ISO C90, Amendment 1"),
 *        ISO/IEC 9899:1999 ("ISO C99"),
 *        ISO/IEC 9899:2011 ("ISO C11"),
 *        IEEE Std 1003.1-2008 ("POSIX.1").
 * @{
 */

/* First arguments to setlocale().
   TODO: Beware, values might change before v0.6 is released.
*/
/**
 * Modifies all categories.
 */
#define LC_ALL      -1
/**
 * Changes the string collation order; affects strcoll() and strxfrm().
 */
#define LC_COLLATE  0
/**
 * Affects the behaviour of the character handling functions defined in
 * <ctype.h>, excluding isdigit() and isxdigit().
 */
#define LC_CTYPE    1
/**
 * Controls the currency-related information returned by localeconv().
 */
#define LC_MONETARY 2
/**
 * Controls the decimal point character used by the number formatting functions,
 * plus the non-monetary information returned by localeconv().
 */
#define LC_NUMERIC  3
/**
 * Controls the formatting used by the strftime() and wcsftime() functions.
 */
#define LC_TIME     4

/*
 * The category parameter can be any of the LC_* macros to specify if the call
 * to setlocale() shall affect the entire locale or only a portion thereof.
 * The category locale specifies which locale should be switched to, with "C"
 * being the minimal default locale, and "" being the locale-specific native
 * environment. A NULL pointer makes setlocale() return the *current* setting.
 * Otherwise, returns a pointer to a string associated with the specified
 * category for the new locale.
 */
char * setlocale( int category, const char * locale ) _PDCLIB_nothrow;

/**
 * @}
 */

/*
 * Returns a struct lconv initialized to the values appropriate for the current
 * locale setting.
 */
struct lconv * localeconv( void ) _PDCLIB_nothrow;

#if _PDCLIB_POSIX_MIN(2008)
#define LC_COLLATE_MASK  (1 << LC_COLLATE)
#define LC_CTYPE_MASK    (1 << LC_CTYPE)
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_NUMERIC_MASK  (1 << LC_NUMERIC)
#define LC_TIME_MASK     (1 << LC_TIME)
#define LC_ALL_MASK      (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MONETARY_MASK | \
                          LC_NUMERIC_MASK | LC_TIME_MASK)

/* POSIX locale type */
#include <sys/types/_locale_t.h>

/**
 * @addtogroup posix_locale POSIX extended locale functions
 * The ISO C standard defines the setlocale(3) function, which can be used for
 * modifying the application global locale. In multithreaded programs, it can
 * sometimes be useful to allow a thread to, either temporarily or permanently,
 * assume a different locale from the rest of the application. POSIX defines
 * extended locale functions which permit this.
 *
 * The function uselocale() is used to change the locale of the current thread.
 * If newlocale is the symbolic constant LC_GLOBAL_LOCALE, then the thread
 * locale will be set to the application global locale. If newlocale is NULL,
 * then the locale will not be changed. Otherwise, newlocale should be a value
 * returned from either of the newlocale() or duplocale() functions. The return
 * value will be the previous thread locale; or LC_GLOBAL_LOCALE if the thread
 * has no current locale.
 *
 * The function duplocale() is used to make an exact copy of an existing locale.
 * The passed locale object must have been returned from a call to either
 * duplocale() or newlocale().
 *
 * The function newlocale() is used to modify an existing locale or create a new
 * locale. The returned locale will have the properties defined by category_mask
 * set to the values from locale as per setlocale(3), with the remainder being
 * taken from either base (which must be a locale object previously returned by
 * duplocale() or newlocale()) if it is specified, or otherwise from the "C"
 * locale. It is undefined if newlocale() modifies base or frees it and creates
 * a new locale.
 *
 * The freelocale() function is used to free a locale previously created via
 * duplocale() or newlocale().
 * @sa lconv(3) setlocale(3)
 * @since Conforming to IEEE Std 1003.1-2008 ("POSIX.1").
 * @{
 */

/**
 * Global locale.
 */
extern struct _PDCLIB_locale _PDCLIB_global_locale;
#define LC_GLOBAL_LOCALE (&_PDCLIB_global_locale)

/**
 * New locale.
 * @throws EINVAL shall be returned if category_mask contains a bit which does
 *         not correspond to a valid category, and may be returned if locale
 *         is not a valid locale object.
 * @throws ENOMEM shall be returned if the system did not have enough memory to
 *         allocate a new locale object or read the locale data.
 * @throws ENOENT shall be returned if the locale specified does not contain
 *         data for all the specified categories.
 */
locale_t newlocale(int category_mask, const char *locale, locale_t base);

/**
 * Set the thread locale to newlocale.
 *
 * If newlocale is (locale_t)0, then doesn't change the locale and just returns
 * the existing locale.
 *
 * If newlocale is LC_GLOBAL_LOCALE, resets the thread's locale to use the
 * global locale.
 *
 * @returns the previous thread locale. If the thread had no previous locale,
 * returns the global locale.
 */
locale_t uselocale(locale_t newlocale);

/**
 * Returns a copy of loc.
 * @throws EINVAL may be returned if locale is not a valid locale.
 * @throws ENOMEM shall be returned if the system had insufficient memory to
 *         satisfy the request.
 */
locale_t duplocale(locale_t loc);

/**
 * Frees the passed locale object.
 */
void freelocale(locale_t loc);

/**
 * @}
 */

#endif

__END_DECLS
#endif

/**
 * @}
 */
