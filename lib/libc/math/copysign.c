double copysign(double x, double y)
{
#if __has_builtin(__builtin_copysign)
    return __builtin_copysign(x, y);
#else
#   error No builtin copysign
#endif
}
