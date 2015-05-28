float logbf(float x)
{
#if __has_builtin(__builtin_logbf)
    return __builtin_logbf(x);
#else
#   error No builtin crt_logbf
#endif
}
