#ifndef _STDDEF_H
#define _STDDEF_H
#include "types.h"
typedef unsigned int size_t; // for 32-bit kernel
typedef int ptrdiff_t;
#undef NULL
#define NULL ((void *)0)
#define offsetof(st, m) __builtin_offsetof(st, m)

#endif
