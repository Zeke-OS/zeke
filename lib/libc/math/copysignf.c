float copysignf(float x, float y)
{
#if __has_builtin(__builtin_copysignf)
    return __builtin_copysignf(x, y);
#else
#   error No builtin copysignf
#endif
}
