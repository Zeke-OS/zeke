long double logbl(long double x)
{
#if __has_builtin(__builtin_logbl)
    return __builtin_logbl(x);
#else
#   error No builtin crt_logbl
#endif
}
