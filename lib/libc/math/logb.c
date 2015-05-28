double logb(double x)
{
#if __has_builtin(__builtin_logb)
    return __builtin_logb(x);
#else
#   error No builtin crt_logb
#endif
}
