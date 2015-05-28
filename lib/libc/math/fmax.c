double fmax(double x, double y)
{
#if __has_builtin(__builtin_fmax)
    return __builtin_fmax(x, y);
#else
#   error No builtin fmax
#endif
}
