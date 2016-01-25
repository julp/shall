#ifndef THEMES_H

# define THEMES_H

# include <stddef.h> /* size_t */
# include <stdbool.h>
# include <stdint.h> /* uint\d+_t */
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
            uint8_t underline:5;
        };
        uint8_t flags;
    };
} Style;

#define FG(color) \
    .fg = color, .fg_set = true

#define BG(color) \
    .bg = color, .bg_set = true

#define HL(color) \
    .hl = color, .hl_set = true

struct Theme {
    const char *name;
#if 0
    // color to hightlight lines
    Color hl;
    bool hl_set;
    // global background color
    Color bg;
    bool bg_set;
#endif
    Style styles[_TOKEN_COUNT];
};

SHALL_API const char *theme_name(const Theme *);

SHALL_API void theme_each(void (*) (const Theme *, void *), void *);

SHALL_API const Theme *theme_by_name(const char *);

SHALL_API char *theme_export_as_css(const Theme *, const char *, bool);

#endif /* THEMES_H */
