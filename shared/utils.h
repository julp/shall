#pragma once

#include <stddef.h> /* size_t */

int ascii_toupper(int);
int strcmp_l(const char *, size_t, const char *, size_t);
int strncmp_l(const char *, size_t, const char *, size_t, size_t);
int ascii_strcasecmp(const char *, const char *);
int ascii_strcasecmp_l(const char *, size_t, const char *, size_t);
int ascii_strncasecmp_l(const char *, size_t, const char *, size_t, size_t);
char *memstr(const char *, const char *, size_t, const char *);
int ascii_memcasecmp(const char *, const char *, size_t);

#define KMP_INSENSITIVE (1<<0)
// #define KMP_PATTERN_DUP (1<<1)

void *kmp_init(const char *, size_t, unsigned int);
char *kmp_search_next(const char *, size_t, void *);
char *kmp_search_first(const char *, size_t, void *);
void kmp_finalize(void *);
