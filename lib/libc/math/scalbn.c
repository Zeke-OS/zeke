double scalbln(double x, long n)
{
#if __has_builtin(__builtin_scalbn)
    return __builtin_scalbn(x, n);
#else
#   error No builtin scalbn
#endif
}
