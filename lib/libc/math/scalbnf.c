float scalblnf(float x, long n)
{
#if __has_builtin(__builtin_scalbnf)
    return __builtin_scalbnf(x, n);
#else
#   error No builtin scalbnf
#endif
}
