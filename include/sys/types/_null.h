#ifndef NULL

#if !defined(__cplusplus)
#define NULL    ((void *)0)
#else
#if __cplusplus >= 201103L
#define NULL    nullptr
#elif defined(__GNUG__) && defined(__GNUC__) && __GNUC__ >= 4
#define NULL    __null
#else
#if defined(__LP64__)
#define NULL    (0L)
#else
#define NULL    0
#endif  /* __LP64__ */
#endif  /* __GNUG__ */
#endif  /* !__cplusplus */

#endif
