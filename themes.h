#ifndef THEMES_H

# define THEMES_H

# include <stddef.h> /* size_t */
# include <stdint.h> /* uint\d+_t */
# include "bool.h"
# include "tokens.h"
# include "shall.h"

typedef struct {
    uint8_t r, g, b;
} color_t;

typedef struct {
    color_t fg, bg;
    bool bold, italic;
} style_t;

typedef struct {
    const char *name;
    style_t styles[_LAST_TOKEN];
} Theme;

SHALL_API void theme_each(void (*) (const Theme *, void *), void *);

SHALL_API const Theme *theme_by_name(const char *);

#endif /* THEMES_H */
