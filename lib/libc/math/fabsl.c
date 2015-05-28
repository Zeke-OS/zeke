/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

long double fabsl(long double arg)
{
#if __has_builtin(__builtin_fabsl)
    return __builtin_fabsl((arg));
#else
    return (arg < 0) ? -arg : arg;
#endif
}
