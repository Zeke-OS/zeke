/* ARM floating point run-time ABI.
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0043d/IHI0043D_rtabi.pdf
 *
 * Licenced under the ISC license (similar to the MIT/Expat license)
 *
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2011-2012 Jörg Mische <bobbl@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <limits.h>

#define clz_64(x) (__builtin_clzl(x))
#define clz_32(x) (__builtin_clzl(x)-32)
#define clz_16(x) (__builtin_clzl(x)-48)

typedef uint16_t float16;
typedef float float32;
typedef double float64;

typedef union {
    uint32_t u;
    float f;
} uf32_t;

typedef union {
    uint64_t u;
    double f;
} uf64_t;


#define F32_MANT_WIDTH      23
#define F64_MANT_WIDTH      52
#define F128_MANT_WIDTH     112

#define _MANT_WIDTH(width)  F ## width ## _MANT_WIDTH
#define _INT_FAST(width)    int_fast ## width ## _t
#define _UINT_FAST(width)   uint_fast ## width ## _t
#define _FLOAT(width)       float ## width
#define _UF(width)      uf ## width ## _t
#define _CLZ(width)     clz_ ## width
#define _UMUL_PPMM(width)   UMUL_PPMM_ ## width
#define MANT_WIDTH(width)   _MANT_WIDTH(width)
#define INT_FAST(width)     _INT_FAST(width)
#define UINT_FAST(width)    _UINT_FAST(width)
#define FLOAT(width)        _FLOAT(width)
#define UF(width)       _UF(width)
#define CLZ(width)      _CLZ(width)
#define UMUL_PPMM(width)    _UMUL_PPMM(width)

#define SIGN_MASK(width)    ((UINT_FAST(width))1<<(width-1))
#define EXP_MASK(width)     (((UINT_FAST(width))1<<(width-1))-((UINT_FAST(width))1<<MANT_WIDTH(width)))
#define MANT_MASK(width)    (((UINT_FAST(width))1<<MANT_WIDTH(width))-1)
#define SIGNALLING_MASK(width)  ((UINT_FAST(width))1<<(MANT_WIDTH(width)-1))
#define NOTANUM(width)      (((UINT_FAST(width))1<<(width-1))|((((UINT_FAST(width))1<<(width-MANT_WIDTH(width)))-1)<<(MANT_WIDTH(width)-1)))
#define EXP_MAX(width)      ((1L<<(width-1-MANT_WIDTH(width)))-1)
#define BIAS(width)     ((1L<<(width-2-MANT_WIDTH(width)))-1)
#define EXTRACT_EXP(width, f)   (((f)>>MANT_WIDTH(width))&EXP_MAX(width))
#define EXTRACT_MANT(width, f)  ((f)&MANT_MASK(width))

#define EXP_TYPE(width)     int16_t

/**
 *  shift right the value v by s and round to nearest or even
 */
#define SHR_NEAREST_EVEN(v, s) (((v) + (1L<<((s)-1))-1 + (((v)>>((s)))&1))>>(s))


double __aeabi_ui2d(unsigned i);
double __aeabi_l2d(long long i);
double __aeabi_ul2d(unsigned long long i);
float __aeabi_i2f(int i);
float __aeabi_ui2f(unsigned i);
float __aeabi_l2f(long long i);

/*
 * Standard double precision floating-point arithmetic helper functions
 */

#if 0
/**
 * double-precision addition.
 */
double __aeabi_dadd(double a, double b)
{
}

/**
 * double-precision division, n / d
 */
double __aeabi_ddiv(double n, double d)
{
}

/**
 * double-precision multiplication.
 */
double __aeabi_dmul(double a, double b)
{
}

/**
 * double-precision reverse subtraction, y – x
 */
double __aeabi_drsub(double x, double y)
{
}

/**
 * double-precision subtraction, x – y
 */
double __aeabi_dsub(double x, double y)
{
}

/*
 * double precision floating-point comparison helper functions
 */

/**
 * non-excepting equality comparison [1], result in PSR ZC flags
 */
void __aeabi_cdcmpeq(double, double)
{
}

/**
 * 3-way (<, =, ?>) compare [1], result in PSR ZC flags
 */
void __aeabi_cdcmple(double, double)
{
}

/**
 * reversed 3-way (<, =, ?>) compare [1], result in PSR ZC flags.
 */
void __aeabi_cdrcmple(double, double)
{
}

/**
 * result (1, 0) denotes (=, ?<>) [2], use for C == and !=
 */
int __aeabi_dcmpeq(double, double)
{
}

/**
 * result (1, 0) denotes (<, ?>=) [2], use for C <
 */
int __aeabi_dcmplt(double, double)
{
}

/**
 * result (1, 0) denotes (<=, ?>) [2], use for C <=
 */
int __aeabi_dcmple(double, double)
{
}

/**
 * result (1, 0) denotes (>=, ?<) [2], use for C >=
 */
int __aeabi_dcmpge(double, double)
{
}

/**
 * result (1, 0) denotes (>, ?<=) [2], use for C >
 */
int __aeabi_dcmpgt(double, double)
{
}

/**
 * result (1, 0) denotes (?, <=>) [2], use for C99 isunordered()
 */
int __aeabi_dcmpun(double, double)
{
}


/*
 * Standard single precision floating-point arithmetic helper functions.
 */

/**
 * single-precision addition
 */
float __aeabi_fadd(float, float)
{
}

/**
 * single-precision division, n / d
 */
float __aeabi_fdiv(float n, float d)
{
}

/**
 * single-precision multiplication.
 */
float __aeabi_fmul(float, float)
{
}

/**
 * single-precision reverse subtraction, y – x
 */
float __aeabi_frsub(float x, float y)
{
}

/**
 * single-precision subtraction, x – y
 */
float __aeabi_fsub(float x, float y)
{
}


/*
 * Standard single precision floating-point comparison helper functions
 */

/**
 * non-excepting equality comparison [1], result in PSR ZC flags
 */
void __aeabi_cfcmpeq(float, float)
{
}

/**
 * 3-way (<, =, ?>) compare [1], result in PSR ZC flags
 */
void __aeabi_cfcmple(float, float)
{
}

/**
 * reversed 3-way (<, =, ?>) compare [1], result in PSR ZC flags
 */
void __aeabi_cfrcmple(float, float)
{
}

/**
 * result (1, 0) denotes (=, ?<>) [2], use for C == and !=
 */
int __aeabi_fcmpeq(float, float)
{
}

/**
 * result (1, 0) denotes (<, ?>=) [2], use for C <
 */
int __aeabi_fcmplt(float, float)
{
}

/**
 * result (1, 0) denotes (<=, ?>) [2], use for C <=
 */
int __aeabi_fcmple(float, float)
{
}

/**
 * result (1, 0) denotes (>=, ?<) [2], use for C >=
 */
int __aeabi_fcmpge(float, float)
{
}

/**
 * result (1, 0) denotes (>, ?<=) [2], use for C >
 */
int __aeabi_fcmpgt(float, float)
{
}

/**
 * result (1, 0) denotes (?, <=>) [2], use for C99 isunordered().
 */
int __aeabi_fcmpun(float, float)
{
}


/*
 * Standard floating-point to integer conversions.
 */

/**
 * double to integer C-style conversion.
 */
int __aeabi_d2iz(double)
{
}

/**
 * double to unsigned C-style conversion.
 */
unsigned __aeabi_d2uiz(double)
{
}

/**
 * double to long long C-style conversion.
 */
long long __aeabi_d2lz(double)
{
}

/**
 * double to unsigned long long C-style conversion.
 */
unsigned long long __aeabi_d2ulz(double)
{
}

/**
 * float (single precision) to integer C-style conversion.
 */
int __aeabi_f2iz(float)
{
}

/**
 * float (single precision) to unsigned C-style conversion.
 */
unsigned __aeabi_f2uiz(float)
{
}

/**
 * float (single precision) to long long C-style conversion.
 */
long long __aeabi_f2lz(float)
{
}

/**
 * float to unsigned long long C-style conversion.
 */
unsigned long long __aeabi_f2ulz(float)
{
}


/*
 * Standard conversions between floating types.
 */

/**
 * double to float (single precision) conversion.
 */
float __aeabi_d2f(double)
{
}

/**
 * float (single precision) to double conversion.
 */
double __aeabi_f2d(float)
{
}

/**
 * IEEE 754 binary16 storage format (VFP half precision) to
 * binary32 (float) conversion.
 */
float __aeabi_h2f(short hf)
{
}

/**
 * IEEE 754 binary16 storage format (VFP alt half precision) to
 * binary32 (float) conversion.
 */
float __aeabi_h2f_alt(short hf)
{
}

/**
 * IEEE 754 binary32 (float) to binary16 storage format (VFP
 * half precision) conversion.
 */
short __aeabi_f2h(float f)
{
}

/**
 * IEEE 754 binary32 (float) to binary16 storage format (VFP alt
 * half precision) conversion.
 */
short __aeabi_f2h_alt(float f)
{
}

/**
 * IEEE 754 binary64 (double) to binary16 storage format (VFP
 * half precision) conversion.
 */
short __aeabi_d2h(double)
{
}

/**
 * IEEE 754 binary64 (double) to binary16 storage format (VFP alt
 * half precision) conversion.
 */
short __aeabi_d2h_alt(double)
{
}
#endif


/*
 * Standard integer to floating-point conversions.
 */

/**
 * integer to double conversion.
 */
double __aeabi_i2d(int i)
{
    return __aeabi_l2d(i);
}

/**
 * unsigned to double conversion.
 */
double __aeabi_ui2d(unsigned i)
{
    return __aeabi_l2d(i);
}

/**
 * long long to double conversion.
 */
double __aeabi_l2d(long long i)
{
    uf64_t r;
    r.u = 0;

    if (i < 0) {
        r.u = SIGN_MASK(64);
        i = -i;
    }

    if (i != 0) {
        uint_fast64_t shift = clz_64(i);
        r.u |= ((BIAS(64)+64-1-shift) << MANT_WIDTH(64))
            + SHR_NEAREST_EVEN((i<<shift)&0x7fffffffffffffff, 64-1-MANT_WIDTH(64));
        /* the + instead of a | guarantees that in the case of an overflow
         * by the rounding the exponent is increased by 1 */
    }

    return r.f;
}

/**
 * unsigned long long to double conversion.
 */
double __aeabi_ul2d(unsigned long long i)
{
    return __aeabi_l2d(i);
}

/**
 * integer to float (single precision) conversion.
 */
float __aeabi_i2f(int i)
{
    return __aeabi_l2f(i);
}

/**
 * unsigned to float (single precision) conversion.
 */
float __aeabi_ui2f(unsigned i)
{
    return __aeabi_l2f(i);
}

/**
 * long long to float (single precision) conversion.
 */
float __aeabi_l2f(long long i)
{
    uf32_t r;
    r.u = 0;

    if (i < 0) {
        r.u = SIGN_MASK(32);
        i = -i;
    }

    if (i != 0) {
        uint_fast64_t shift = clz_64(i);
        r.u |= ((BIAS(32)+64-1-shift)<<MANT_WIDTH(32))
            + SHR_NEAREST_EVEN((i<<shift)&0x7fffffffffffffff, 64-1-MANT_WIDTH(32));
        /* the + instead of a | guarantees that in the case of an overflow
         * by the rounding the exponent is increased by 1 */
    }

    return r.f;
}

#if 0
/**
 * unsigned long long to float (single precision) conversion.
 */
float __aeabi_ul2f(unsigned long long)
{
}
#endif
