/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

double fabs(double arg)
{
#if __has_builtin(__builtin_fabs)
    return __builtin_fabs((arg));
#else
    return (arg < 0) ? -arg : arg;
#endif
}
