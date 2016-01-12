#include "themes.h"

#define grey         { 0x99, 0x99, 0x99 }
#define white        { 0xFF, 0xFF, 0xFF }
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

const SHALL_API Theme monokai = {
    "monokai",
    {
#define COMMON_STYLE FG(dimgrey), .italic = true
        [ COMMENT_SINGLE ]         = { COMMON_STYLE },
        [ COMMENT_MULTILINE ]      = { COMMON_STYLE },
        [ COMMENT_DOCUMENTATION ]  = { COMMON_STYLE, .bold = true },
#undef COMMON_STYLE
        [ GENERIC_INSERTED ]       = { FG(white), BG(dimgreen) },
        [ GENERIC_DELETED ]        = { FG(white), BG(dimred) },
        [ GENERIC_HEADING ]        = { FG(grey) },
        [ GENERIC_STRONG ]         = { .bold = true },
        [ GENERIC_SUBHEADING ]     = { FG(light_grey) },
#define COMMON_STYLE FG(soft_cyan), .bold = true
        [ KEYWORD ]                = { COMMON_STYLE },
        [ KEYWORD_DEFAULT ]        = { COMMON_STYLE },
        [ KEYWORD_BUILTIN ]        = { COMMON_STYLE },
        [ KEYWORD_CONSTANT ]       = { COMMON_STYLE },
        [ KEYWORD_DECLARATION ]    = { COMMON_STYLE },
        [ KEYWORD_PSEUDO ]         = { COMMON_STYLE },
        [ KEYWORD_RESERVED ]       = { COMMON_STYLE },
        [ KEYWORD_TYPE ]           = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE FG(bright_pink), .bold = true
        [ KEYWORD_NAMESPACE ]      = { COMMON_STYLE },
        [ OPERATOR ]               = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE FG(light_violet)
        [ NUMBER_FLOAT ]           = { COMMON_STYLE },
        [ NUMBER_DECIMAL ]         = { COMMON_STYLE },
        [ NUMBER_BINARY ]          = { COMMON_STYLE },
        [ NUMBER_OCTAL ]           = { COMMON_STYLE },
        [ NUMBER_HEXADECIMAL ]     = { COMMON_STYLE },
        [ STRING_SINGLE ]          = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE FG(soft_yellow)
        [ STRING_DOUBLE ]          = { COMMON_STYLE },
        [ STRING_BACKTICK ]        = { COMMON_STYLE },
        [ STRING_INTERNED ]        = { COMMON_STYLE },
#undef COMMON_STYLE
#define COMMON_STYLE FG(bright_green), .bold = true
        [ NAME_CLASS ]             = { COMMON_STYLE },
        [ NAME_FUNCTION ]          = { COMMON_STYLE },
#undef COMMON_STYLE
//         [ NAME_CONSTANT ]          = { FG(soft_cyan) },
#define COMMON_STYLE FG(whitish)
        [ NAME_BUILTIN_PSEUDO ]    = { COMMON_STYLE },
        [ NAME_BUILTIN ]           = { COMMON_STYLE },
        [ NAME_ENTITY ]            = { COMMON_STYLE },
        [ NAME_NAMESPACE ]         = { COMMON_STYLE },
        [ NAME_VARIABLE_CLASS ]    = { COMMON_STYLE },
        [ NAME_VARIABLE_GLOBAL ]   = { COMMON_STYLE },
        [ NAME_VARIABLE_INSTANCE ] = { COMMON_STYLE },
        [ NAME_VARIABLE ]          = { COMMON_STYLE },
#undef COMMON_STYLE
        [ NAME_TAG ]               = { FG(bright_pink) },
    }
};
