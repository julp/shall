#ifndef UTILS_H

# define UTILS_H

int ascii_toupper(int);
int strcmp_l(const char *, size_t, const char *, size_t);
int strncmp_l(const char *, size_t, const char *, size_t, size_t);
int ascii_strcasecmp(const char *, const char *);
int ascii_strcasecmp_l(const char *, size_t, const char *, size_t);
int ascii_strncasecmp_l(const char *, size_t, const char *, size_t, size_t);
char *memstr(const char *, const char *, size_t, const char *);

#endif /* !UTILS_H */
