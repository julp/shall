#ifndef THEMES_H

# define THEMES_H

# include <stddef.h> /* size_t */
# include <stdint.h> /* uint\d+_t */
# include "bool.h"
# include "tokens.h"
# include "shall.h"

typedef struct {
    uint8_t r, g, b;
} Color;

typedef struct {
    Color fg, bg;
    union {
        struct {
            uint8_t fg_set:1;
            uint8_t bg_set:2;
            uint8_t italic:3;
            uint8_t bold:4;
        };
        uint8_t flags;
    };
} Style;

#define FG(color) \
    .fg = color, .fg_set = TRUE

#define BG(color) \
    .bg = color, .bg_set = TRUE

typedef struct {
    const char *name;
    Style styles[_LAST_TOKEN];
} Theme;

SHALL_API const char *theme_name(const Theme *);

SHALL_API void theme_each(void (*) (const Theme *, void *), void *);

SHALL_API const Theme *theme_by_name(const char *);

SHALL_API char *theme_export_as_css(const Theme *, const char *, bool);

#endif /* THEMES_H */
