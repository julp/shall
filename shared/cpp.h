#pragma once

#include "machine.h"

#if GCC_VERSION >= 4003 || __has_attribute(hot)
# define HOT __attribute__((hot))
#else
# define HOT
#endif /* HOT */

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif /* !__has_builtin */

#ifndef typeof
# define typeof __typeof__
#endif /* !typeof */

#if __has_builtin(__builtin_expect)
# define EXPECTED(condition)   __builtin_expect(!!(condition), 1)
# define UNEXPECTED(condition) __builtin_expect(!!(condition), 0)
#else
# define EXPECTED(condition)   (condition)
# define UNEXPECTED(condition) (condition)
#endif /* __builtin_expect */

#define mem_new(type)           malloc((sizeof(type)))
#define mem_new_n(type, n)      malloc((sizeof(type) * (n)))
#define mem_new_n0(type, n)     calloc((n), (sizeof(type)))
#define mem_renew(ptr, type, n) realloc((ptr), (sizeof(type) * (n)))

#ifndef MAX
# define MAX(a, b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a > _b ? _a : _b; })
#endif /* !MAX */

#ifndef MIN
# define MIN(a, b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a < _b ? _a : _b; })
#endif /* !MIN */

#define HAS_FLAG(value, flag) \
    (0 != ((value) & (flag)))

#define SET_FLAG(value, flag) \
    ((value) |= (flag))

#define UNSET_FLAG(value, flag) \
    ((value) &= ~(flag))

#ifdef DEBUG
# undef NDEBUG
# include <stdio.h>
# define debug(format, ...) \
    fprintf(stderr, format "\n", ## __VA_ARGS__)
#else
# ifndef NDEBUG
#  define NDEBUG
# endif /* !NDEBUG */
# define debug(format, ...) \
    /* NOP */
#endif /* DEBUG */
#include <assert.h>
