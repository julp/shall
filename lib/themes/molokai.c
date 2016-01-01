#include "themes.h"

#define red        { 0xF9, 0x26, 0x72 }
#define blue       { 0x66, 0xD9, 0xEF }
#define grey       { 0x40, 0x3D, 0x3D }
#define white      { 0xF8, 0xF8, 0xF2 }
#define black      { 0x1B, 0x1D, 0x1E }
#define green      { 0xA6, 0xE2, 0x2E }
#define violet     { 0xAF, 0x87, 0xFF }
#define yellow     { 0xD7, 0xD7, 0x87 }
#define dark_blue  { 0x5E, 0x5D, 0x83 }
#define light_grey { 0x46, 0x54, 0x57 }

const SHALL_API Theme molokai = {
    "molokai",
    {
#define COMMON_STYLE .fg = dark_blue, .italic = TRUE
        [ COMMENT_SINGLE ]         = { COMMON_STYLE },
        [ COMMENT_MULTILINE ]      = { COMMON_STYLE },
#undef COMMON_STYLE
        [ COMMENT_DOCUMENTATION ]  = { .fg = light_grey, .italic = TRUE },
        [ GENERIC_INSERTED ]       = { .fg = green },
        [ GENERIC_DELETED ]        = { .fg = red },
        [ GENERIC_HEADING ]        = { .fg = grey },
        [ GENERIC_STRONG ]         = { .bold = TRUE },
        [ GENERIC_SUBHEADING ]     = { .fg = light_grey },
#define COMMON_STYLE .fg = blue, .bold = TRUE
        [ KEYWORD ]                = { COMMON_STYLE },
//         [ KEYWORD_DEFAULT ]        = { COMMON_STYLE },
//         [ KEYWORD_BUILTIN ]        = { COMMON_STYLE },
        [ KEYWORD_CONSTANT ]       = { COMMON_STYLE },
        [ KEYWORD_DECLARATION ]    = { COMMON_STYLE },
        [ KEYWORD_PSEUDO ]         = { COMMON_STYLE },
        [ KEYWORD_RESERVED ]       = { COMMON_STYLE },
        [ KEYWORD_TYPE ]           = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = red, .bold = TRUE
        [ KEYWORD_NAMESPACE ]      = { COMMON_STYLE },
        [ OPERATOR ]               = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = violet
        [ NUMBER_FLOAT ]           = { COMMON_STYLE },
        [ NUMBER_DECIMAL ]         = { COMMON_STYLE },
        [ NUMBER_BINARY ]          = { COMMON_STYLE },
        [ NUMBER_OCTAL ]           = { COMMON_STYLE },
        [ NUMBER_HEXADECIMAL ]     = { COMMON_STYLE },
        [ SEQUENCE_ESCAPED ]       = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = yellow
        [ STRING_REGEX ]           = { COMMON_STYLE },
        [ STRING_SINGLE ]          = { COMMON_STYLE },
        [ STRING_DOUBLE ]          = { COMMON_STYLE },
        [ STRING_BACKTICK ]        = { COMMON_STYLE },
        [ STRING_INTERNED ]        = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE .fg = green, .bold = TRUE
        [ NAME_CLASS ]             = { COMMON_STYLE },
        [ NAME_FUNCTION ]          = { COMMON_STYLE },
#undef COMMON_STYLE
//         [ NAME_CONSTANT ]          = { .fg = blue },
#define COMMON_STYLE .fg = white
        [ NAME_BUILTIN_PSEUDO ]    = { COMMON_STYLE },
        [ NAME_BUILTIN ]           = { COMMON_STYLE },
        [ NAME_ENTITY ]            = { COMMON_STYLE },
        [ NAME_NAMESPACE ]         = { COMMON_STYLE },
        [ NAME_VARIABLE_CLASS ]    = { COMMON_STYLE },
        [ NAME_VARIABLE_GLOBAL ]   = { COMMON_STYLE },
        [ NAME_VARIABLE_INSTANCE ] = { COMMON_STYLE },
        [ NAME_VARIABLE ]          = { COMMON_STYLE },
#undef COMMON_STYLE
        [ NAME_TAG ]               = { .fg = red },
//         [ TEXT ]                   = { .fg = white, .bg = black },
    }
};
