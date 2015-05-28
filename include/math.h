/*
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

#define HUGE    1.701411733192644270e38
#define LOGHUGE 39

double acos(double arg);
/* TODO acosf(), acosh(), acoshf(), acoshl(), acosl() */

double asin(double arg);
/* TODO asinf(), asinh(), asinhf(), asinhl(), asinl() */

double atan(double arg);
double atan2(double arg1, double arg2);
/* TODO atan2f(), atan2l(), atanf(), atanh(), atanhf(), atanhl(), atanl() */
/* TODO cbrt(), cbrtf(), cbrtl() */

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

double floor(double d);

int isnanf(float x);
int isnan(double x);

int isinff(float x);
int isinf(double x);

double sqrt(double arg);

double log(double arg);
double log10(double arg);

float modff(float x, float * iptr);
double modf(double x, double * iptr);

double pow(double arg1, double arg2);

float ldexpf(float x, int exp);
double ldexp(double x, int exp);

double fmod(double x, double y);

#endif /* _MATH_H_ */

/**
 * @}
 */
