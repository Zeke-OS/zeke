long double copysignl(long double x, long double y)
{
#if __has_builtin(__builtin_copysignl)
    return __builtin_copysignl(x, y);
#else
#   error No builtin copysignl
#endif
}
