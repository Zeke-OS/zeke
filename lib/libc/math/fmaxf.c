float fmaxf(float x, float y)
{
#if __has_builtin(__builtin_fmaxf)
    return __builtin_fmaxf(x, y);
#else
#   error No builtin fmaxf
#endif
}
