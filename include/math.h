/*
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef _MATH_H_
#define _MATH_H_

#if __BSD_VISIBLE
#define HUGE    1.701411733192644270e38
#define LOGHUGE 39
#endif /* __BSD_VISIBLE */

#define HUGE_VAL    __builtin_huge_val()
#define HUGE_VALF   __builtin_huge_valf()
#define HUGE_VALL   __builtin_huge_vall()
#define INFINITY    __builtin_inff()
#define NAN         __builtin_nanf("")

#if __BSD_VISIBLE || __XSI_VISIBLE
#define M_E         2.7182818284590452354   /* e */
#define M_LOG2E     1.4426950408889634074   /* log 2e */
#define M_LOG10E    0.43429448190325182765  /* log 10e */
#define M_LN2       0.69314718055994530942  /* log e2 */
#define M_LN10      2.30258509299404568402  /* log e10 */
#define M_PI        3.14159265358979323846  /* pi */
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#define M_1_PI      0.31830988618379067154  /* 1/pi */
#define M_2_PI      0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */
#endif /* __BSD_VISIBLE || __XSI_VISIBLE */

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

double acos(double arg);
/* TODO acosf(), acosh(), acoshf(), acoshl(), acosl() */

double asin(double arg);
/* TODO asinf(), asinh(), asinhf(), asinhl(), asinl() */

double atan(double arg);
double atan2(double arg1, double arg2);
/* TODO atan2f(), atan2l(), atanf(), atanh(), atanhf(), atanhl(), atanl() */
/* TODO cbrt(), cbrtf(), cbrtl() */

double copysign(double x, double y);
float copysignf(float x, float y);
long double copysignl(long double x, long double y);

double cos(double arg);
double sin(double arg);

double sinh(double arg);
double cosh(double arg);

double tan(double arg);
double tanh(double arg);

double ceil(double d);
/* TODO ceilf(), ceill() */

double cos(double arg);
/* TODO cosf(), cosh(), coshf(), coshl(), cosl() */

float frexpf(float x, int * exp);
double frexp(double x, int * exp);

double j0(double arg);
double j1(double arg);
double jn(int n, double x);
double y0(double arg);
double y1(double arg);
double yn(int n, double x);

double erf(double arg);

double erfc(double arg);
/* TODO erfcf(), erfcl(), erff(), erfl() */

double exp(double arg);
/* TODO exp2(), exp2f(), exp2l(), expf(), expl() */
/* TODO expm1(), expm1f(), expm1l() */

double fabs(double arg);
float fabsf(float arg);
long double fabsl(long double arg);

double fmax(double x, double y);
float fmaxf(float x, float y);
long double fmaxl(long double x, long double y);

double floor(double d);

int isnanf(float x);
int isnan(double x);

int isinff(float x);
int isinf(double x);

double sqrt(double arg);

double log(double arg);
double log10(double arg);

double logb(double x);
float logbf(float x);
long double logbl(long double x);

float modff(float x, float * iptr);
double modf(double x, double * iptr);

double scalbln(double x, long n);
float scalblnf(float x, long n);
long double scalbnl(long double x, int n);

double pow(double arg1, double arg2);

float ldexpf(float x, int exp);
double ldexp(double x, int exp);

double fmod(double x, double y);

__END_DECLS
#endif /* KERNEL_INTERNAL */

#endif /* _MATH_H_ */

/**
 * @}
 */
