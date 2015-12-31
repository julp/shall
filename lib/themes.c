# include <stdint.h> /* uint\d+_t */
# include "cpp.h"
# include "bool.h"
#include "tokens.h"

typedef struct {
    uint8_t r, g, b;
} color_t;

typedef struct {
    color_t fg, bg;
    bool bold, italic;
} attribute_t;

typedef struct {
    const char *name;
    attribute_t attributes[_LAST_TOKEN];
} theme_t;

#define white        { 0xFF, 0xFF, 0xFF }

#define grey         { 0x99, 0x99, 0x99 }
#define dimred       { 0x49, 0x31, 0x31 }
#define whitish      { 0xF8, 0xF8, 0xF2 }
#define dimgrey      { 0x75, 0x71, 0x5E }
#define dimgreen     { 0x32, 0x49, 0x32 }
#define soft_cyan    { 0x66, 0xd9, 0xef }
#define light_grey   { 0xAA, 0xAA, 0xAA }
#define soft_yellow  { 0xE6, 0xDB, 0x74 }
#define bright_pink  { 0xF9, 0x26, 0x72 }
#define bright_green { 0xA6, 0xE2, 0x2E }
#define light_violet { 0xAE, 0x81, 0xFF }

const theme_t monokai = {
    "monokai",
    {
#define COMMON_STYLE .fg = dimgrey, .italic = TRUE
        [ COMMENT_SINGLE ]         = { COMMON_STYLE },
        [ COMMENT_MULTILINE ]      = { COMMON_STYLE },
        [ COMMENT_DOCUMENTATION ]  = { COMMON_STYLE, .bold = TRUE },
#undef COMMON_STYLE
        [ GENERIC_INSERTED ]       = { .fg = white, .bg = dimgreen },
        [ GENERIC_DELETED ]        = { .fg = white, .bg = dimred },
        [ GENERIC_HEADING ]        = { .fg = grey },
        [ GENERIC_STRONG ]         = { .bold = TRUE },
        [ GENERIC_SUBHEADING ]     = { .fg = light_grey },
#define COMMON_STYLE .fg = soft_cyan, .bold = TRUE
        [ KEYWORD ]                = { COMMON_STYLE },
        [ KEYWORD_DEFAULT ]        = { COMMON_STYLE },
        [ KEYWORD_BUILTIN ]        = { COMMON_STYLE },
        [ KEYWORD_CONSTANT ]       = { COMMON_STYLE },
        [ KEYWORD_DECLARATION ]    = { COMMON_STYLE },
        [ KEYWORD_PSEUDO ]         = { COMMON_STYLE },
        [ KEYWORD_RESERVED ]       = { COMMON_STYLE },
        [ KEYWORD_TYPE ]           = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = bright_pink, .bold = TRUE
        [ KEYWORD_NAMESPACE ]      = { COMMON_STYLE },
        [ OPERATOR ]               = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = light_violet
        [ NUMBER_FLOAT ]           = { COMMON_STYLE },
        [ NUMBER_DECIMAL ]         = { COMMON_STYLE },
        [ NUMBER_BINARY ]          = { COMMON_STYLE },
        [ NUMBER_OCTAL ]           = { COMMON_STYLE },
        [ NUMBER_HEXADECIMAL ]     = { COMMON_STYLE },
        [ STRING_SINGLE ]          = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = soft_yellow
        [ STRING_DOUBLE ]          = { COMMON_STYLE },
        [ STRING_BACKTICK ]        = { COMMON_STYLE },
        [ STRING_INTERNED ]        = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = bright_green, .bold = TRUE
        [ NAME_CLASS ]             = { COMMON_STYLE },
        [ NAME_FUNCTION ]          = { COMMON_STYLE },
#undef COMMON_STYLE
//         [ NAME_CONSTANT ]          = { .fg = soft_cyan },
#define COMMON_STYLE .fg = whitish
        [ NAME_BUILTIN_PSEUDO ]    = { COMMON_STYLE },
        [ NAME_BUILTIN ]           = { COMMON_STYLE },
        [ NAME_ENTITY ]            = { COMMON_STYLE },
        [ NAME_NAMESPACE ]         = { COMMON_STYLE },
        [ NAME_VARIABLE_CLASS ]    = { COMMON_STYLE },
        [ NAME_VARIABLE_GLOBAL ]   = { COMMON_STYLE },
        [ NAME_VARIABLE_INSTANCE ] = { COMMON_STYLE },
        [ NAME_VARIABLE ]          = { COMMON_STYLE },
#undef COMMON_STYLE
        [ NAME_TAG ]               = { .fg = bright_pink },
    }
};
