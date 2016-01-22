#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cpp.h"
#include "tokens.h"
#include "themes.h"
#include "formatter.h"

typedef struct {
    const Theme *theme ALIGNED(sizeof(OptionValue));
    struct {
        size_t prefix_len;
        const char *prefix;
        size_t suffix_len;
        const char *suffix;
    } sequences[_TOKEN_COUNT];
} BBCodeFormatterData;

#define LONGEST_OPENING_TAG \
    "[color=#XXXXXX][b][u][i]"

typedef struct {
    char *w; // offset to write
    char buffer[STR_SIZE(LONGEST_OPENING_TAG)];
} string_builder_t;

#define INIT_STRING_BUILDER(sb) \
    do { \
        *(sb).buffer = '\0'; \
        (sb).w = (sb).buffer; \
    } while (0);

#define STRING_BUILDER_APPEND(sb, string) \
    (sb).w = stpcpy((sb).w, string)

#define STRING_BUILDER_APPEND_FORMATTED(sb, fmt, ...) \
    (sb).w += sprintf((sb).w, fmt, ## __VA_ARGS__)

#define STRING_BUILDER_DUP_INTO(sb, out) \
    do { \
        out = strdup((sb).buffer); \
        out##_len = (sb).w - (sb).buffer; \
    } while(0);

static int bbcode_start_document(String *UNUSED(out), FormatterData *data)
{
    size_t i;
    const Theme *theme;
    BBCodeFormatterData *mydata;

    mydata = (BBCodeFormatterData *) data;
    // TODO: define a default theme in shall itself
    if (NULL == (theme = mydata->theme)) {
        theme = theme_by_name("molokai");
    }
    for (i = 0; i < _TOKEN_COUNT; i++) {
        mydata->sequences[i].prefix_len = mydata->sequences[i].suffix_len = 0;
        mydata->sequences[i].prefix = mydata->sequences[i].suffix = NULL;
        if (theme->styles[i].flags) { // TODO: exclude bg_set bit
            enum { OPENING_TAG, CLOSING_TAG, _TAG_COUNT };
            string_builder_t sb[_TAG_COUNT];

            INIT_STRING_BUILDER(sb[OPENING_TAG]);
            INIT_STRING_BUILDER(sb[CLOSING_TAG]);
            if (theme->styles[i].bold) {
                STRING_BUILDER_APPEND(sb[OPENING_TAG], "[b]");
            }
            if (theme->styles[i].italic) {
                STRING_BUILDER_APPEND(sb[OPENING_TAG], "[i]");
            }
            if (theme->styles[i].fg_set) {
                STRING_BUILDER_APPEND_FORMATTED(sb[OPENING_TAG], "[color=#%" PRIx8 "%" PRIx8 "%" PRIx8 "]", theme->styles[i].fg.r, theme->styles[i].fg.g, theme->styles[i].fg.b);
            }
            if (theme->styles[i].fg_set) {
                STRING_BUILDER_APPEND(sb[CLOSING_TAG], "[/color]");
            }
            if (theme->styles[i].italic) {
                STRING_BUILDER_APPEND(sb[CLOSING_TAG], "[/i]");
            }
            if (theme->styles[i].bold) {
                STRING_BUILDER_APPEND(sb[CLOSING_TAG], "[/b]");
            }
            STRING_BUILDER_DUP_INTO(sb[OPENING_TAG], mydata->sequences[i].prefix);
            STRING_BUILDER_DUP_INTO(sb[CLOSING_TAG], mydata->sequences[i].suffix);
        }
    }

    return 0;
}

static int bbcode_end_document(String *UNUSED(out), FormatterData *data)
{
    size_t i;
    BBCodeFormatterData *mydata;

    mydata = (BBCodeFormatterData *) data;
    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (mydata->sequences[i].prefix_len > 0) {
            free((void *) mydata->sequences[i].prefix);
            free((void *) mydata->sequences[i].suffix);
        }
    }

    return 0;
}

static int bbcode_start_token(int token, String *out, FormatterData *data)
{
    BBCodeFormatterData *mydata;

    mydata = (BBCodeFormatterData *) data;
    if (mydata->sequences[token].prefix_len > 0) {
        string_append_string_len(out, mydata->sequences[token].prefix, mydata->sequences[token].prefix_len);
    }

    return 0;
}

static int bbcode_end_token(int token, String *out, FormatterData *data)
{
    BBCodeFormatterData *mydata;

    mydata = (BBCodeFormatterData *) data;
    if (mydata->sequences[token].suffix_len > 0) {
        string_append_string_len(out, mydata->sequences[token].suffix, mydata->sequences[token].suffix_len);
    }

    return 0;
}

static int bbcode_write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len(out, token, token_len);

    return 0;
}

const FormatterImplementation _bbcodefmt = {
    "BBCode",
    "Format tokens for forums using bbcode syntax to format post",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    bbcode_start_document,
    bbcode_end_document,
    bbcode_start_token,
    bbcode_end_token,
    bbcode_write_token,
    NULL,
    NULL,
    sizeof(BBCodeFormatterData),
        (/*const*/ FormatterOption /*const*/ []) {
        { S("theme"), OPT_TYPE_THEME, offsetof(BBCodeFormatterData, theme), OPT_DEF_THEME, "the theme to use" },
        END_OF_FORMATTER_OPTIONS
    }
};

/*SHALL_API*/ const FormatterImplementation *bbcodefmt = &_bbcodefmt;