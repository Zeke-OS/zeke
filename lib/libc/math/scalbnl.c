long double scalblnl(long double x, int n)
{
#if __has_builtin(__builtin_scalbnl)
    return __builtin_scalbnl(x, n);
#else
#   error No builtin scalbnl
#endif
}
