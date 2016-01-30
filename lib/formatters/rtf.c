#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "cpp.h"
#include "tokens.h"
#include "themes.h"
#include "formatter.h"
#include "hashtable.h"
#include "string_builder.h"

typedef struct {
    const Theme *theme ALIGNED(sizeof(OptionValue));
    struct {
        size_t prefix_len;
        const char *prefix;
    } sequences[_TOKEN_COUNT];
} RTFFormatterData;

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define LONGEST_FORMAT_STRING \
    "{\\b\\i\\ul\\cb" STRINGIFY(INT_MAX) "\\cf" STRINGIFY(INT_MAX)

STRING_BUILDER_DECL(STR_SIZE(LONGEST_FORMAT_STRING));

static ht_hash_t hash_color(const Color *color)
{
    struct hashed_color {
        union {
            Color color;
            ht_hash_t h;
        };
    } h;

    memcpy(&h.color, color, sizeof(*color));

    return h.h;    
}

static int rtf_start_document(String *out, FormatterData *data)
{
    size_t i;
    const Theme *theme;
    enum { FG, BG, COUNT };
    RTFFormatterData *mydata;
    int map[COUNT][_TOKEN_COUNT];

    mydata = (RTFFormatterData *) data;
    // TODO: define a default theme in shall itself
    if (NULL == (theme = mydata->theme)) {
        theme = theme_by_name("molokai");
    }
    STRING_APPEND_STRING(out, "{\\rtf1\\ansi\\uc0\\deff0{\\fonttbl{\\f0\\fmodern\\fprq1\\fcharset0");
    // TODO: append fontface here?
    STRING_APPEND_STRING(out, ";}}{\\colortbl;");
    {
        int index;
        HashTable colors;

        index = 1; // rtf color index in colortbl starts at 1?
        // there is at most 2 * _TOKEN_COUNT (the same foreground and background shared by all token types)
        hashtable_init(&colors, 2 * _TOKEN_COUNT, value_hash, NULL, NULL, NULL, NULL);
        for (i = 0; i < _TOKEN_COUNT; i++) {
            if (theme->styles[i].flags & (ATTR_MASK(FG_BIT)|ATTR_MASK(BG_BIT))) {
                int *ptr;
    
                if (theme->styles[i].fg_set) {
                    if (hashtable_direct_put(&colors, HT_PUT_ON_DUP_KEY_PRESERVE, hash_color(&theme->styles[i].fg), &map[FG][i], &ptr)) {
                        string_append_formatted(out, "\\red%d\\green%d\\blue%d;", theme->styles[i].fg.r, theme->styles[i].fg.g, theme->styles[i].fg.b);
                        map[FG][i] = index++;
                    } else {
                        map[FG][i] = *ptr;
                    }
                }
                if (theme->styles[i].bg_set) {
                    if (hashtable_direct_put(&colors, HT_PUT_ON_DUP_KEY_PRESERVE, hash_color(&theme->styles[i].bg), &map[BG][i], &ptr)) {
                        string_append_formatted(out, "\\red%d\\green%d\\blue%d;", theme->styles[i].bg.r, theme->styles[i].bg.g, theme->styles[i].bg.b);
                        map[BG][i] = index++;
                    } else {
                        map[BG][i] = *ptr;
                    }
                }
            }
        }
        hashtable_destroy(&colors);
    }
    STRING_APPEND_STRING(out, "}\\f0 ");
    for (i = 0; i < _TOKEN_COUNT; i++) {
        mydata->sequences[i].prefix_len = 0;
        mydata->sequences[i].prefix = NULL;
        if (theme->styles[i].flags) {
            string_builder_t sb;

            STRING_BUILDER_INIT(sb);
            STRING_BUILDER_APPEND_1(sb, '{');
            if (theme->styles[i].bold) {
                STRING_BUILDER_APPEND(sb, "\\b");
            }
            if (theme->styles[i].italic) {
                STRING_BUILDER_APPEND(sb, "\\i");
            }
            if (theme->styles[i].bg_set) {
                STRING_BUILDER_APPEND_FORMATTED(sb, "\\cb%d", map[BG][i]);
            }
            if (theme->styles[i].fg_set) {
                STRING_BUILDER_APPEND_FORMATTED(sb, "\\cf%d", map[FG][i]);
            }
            STRING_BUILDER_DUP_INTO(sb, mydata->sequences[i].prefix);
        }
    }

    return 0;
}

#if 0
static int rtf_end_document(String *out, FormatterData *data)
{
    size_t i;
    RTFFormatterData *mydata;

    mydata = (RTFFormatterData *) data;

    return 0;
}
#endif

static int rtf_start_token(int token, String *out, FormatterData *data)
{
    RTFFormatterData *mydata;

    mydata = (RTFFormatterData *) data;
    if (mydata->sequences[token].prefix_len > 0) {
        string_append_string_len(out, mydata->sequences[token].prefix, mydata->sequences[token].prefix_len);
    }

    return 0;
}

static int rtf_end_token(int token, String *out, FormatterData *data)
{
    RTFFormatterData *mydata;

    mydata = (RTFFormatterData *) data;
    if (mydata->sequences[token].prefix_len > 0) {
        string_append_char(out, '}');
    }

    return 0;
}

// TODO:
// - replace \R with \par
// - handle Unicode character (\uXXXX)
// - escape (with backslash) '{', '}' and '\\'
static int rtf_write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len(out, token, token_len);

    return 0;
}

static void rtf_finalize(FormatterData *data)
{
    size_t i;
    RTFFormatterData *mydata;

    mydata = (RTFFormatterData *) data;
    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (mydata->sequences[i].prefix_len > 0) {
            free((void *) mydata->sequences[i].prefix);
        }
    }
}

const FormatterImplementation _rtffmt = {
    "RTF",
    "Format tokens for forums using bbcode syntax to format post",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    rtf_start_document,
    NULL/*rtf_end_document*/,
    rtf_start_token,
    rtf_end_token,
    rtf_write_token,
    NULL,
    NULL,
    rtf_finalize,
    sizeof(RTFFormatterData),
        (/*const*/ FormatterOption /*const*/ []) {
        { S("theme"), OPT_TYPE_THEME, offsetof(RTFFormatterData, theme), OPT_DEF_THEME, "the theme to use" },
        END_OF_OPTIONS
    }
};

/*SHALL_API*/ const FormatterImplementation *rtffmt = &_rtffmt;
