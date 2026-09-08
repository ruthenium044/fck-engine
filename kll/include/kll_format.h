#ifndef FCK_KLL_FORMAT_H_INCLUDED
#define FCK_KLL_FORMAT_H_INCLUDED

// If issues, better casting!
#define kll_format(arena, str, ...) (arena)->format(arena, str, __VA_ARGS__)

#endif // !FCK_KLL_FORMAT_H_INCLUDED
