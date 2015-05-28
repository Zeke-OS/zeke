long double fmaxl(long double x, long double y)
{
#if __has_builtin(__builtin_fmaxl)
    return __builtin_fmaxl(x, y);
#else
#   error No builtin fmaxl
#endif
}
