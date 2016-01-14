#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "themes.h"
#include "formatter.h"

#define BOLD      "01"
#define FAINT     "02"
#define STANDOUT  "03"
#define UNDERLINE "04"
#define BLINK     "05"
#define OVERLINE  "06"

#define BLACK     "30"
#define DARKGRAY  BLACK ";" BOLD
#define DARKRED   "31"
#define RED       DARKRED ";" BOLD
#define DARKGREEN "32"
#define GREEN     DARKGREEN ";" BOLD
#define BROWN     "33"
#define YELLOW    BROWN ";" BOLD
#define DARKBLUE  "34"
#define BLUE      DARKBLUE ";" BOLD
#define PURPLE    "35"
#define FUCHSIA   PURPLE ";" BOLD
#define TEAL      "36"
#define TURQUOISE TEAL ";" BOLD
#define LIGHTGRAY "37"
#define WHITE     LIGHTGRAY ";" BOLD

#define DARKTEAL   TURQUOISE
#define DARKYELLOW BROWN
#define FUSCIA     FUCHSIA

#define SEQ(x) "\e[" x "m"

// bold (1) + italic (3) + underline (4) (unused for now) + fg (38;2;R;G;B) + bg (48;2;R;G;B)
#define LONGEST_ANSI_ESCAPE_SEQUENCE \
    "\e[1;3;4;38;2;RRR;GGG;BBB;48;2;RRR;GGG;BBBm"

static const char * const map[] = {
    [ EOS ] = "",
    [ IGNORABLE ] = "",
    [ TEXT ] = "",
    [ TAG_PREPROC ] = "",
    [ NAME_BUILTIN ] = SEQ(TEAL),
    [ NAME_BUILTIN_PSEUDO ] = "",
    [ NAME_TAG ] = SEQ(TURQUOISE),
    [ NAME_ENTITY ] = "",
    [ NAME_ATTRIBUTE ] = SEQ(TEAL),
    [ NAME_VARIABLE ] = SEQ(DARKRED),
    [ NAME_VARIABLE_CLASS ] = SEQ(DARKRED),
    [ NAME_VARIABLE_GLOBAL ] = SEQ(DARKRED),
    [ NAME_VARIABLE_INSTANCE ] = SEQ(DARKRED),
    [ NAME_FUNCTION ] = SEQ(DARKGREEN),
    [ NAME_CLASS ] = SEQ(DARKGREEN ";" UNDERLINE),
    [ NAME_NAMESPACE ] = SEQ(DARKGREEN ";" UNDERLINE),
    [ PUNCTUATION ] = "",
    [ KEYWORD ] = SEQ(DARKBLUE),
    [ KEYWORD_DEFAULT ] = "",
    [ KEYWORD_BUILTIN ] = "",
    [ KEYWORD_CONSTANT ] = "",
    [ KEYWORD_DECLARATION ] = SEQ(DARKBLUE),
    [ KEYWORD_NAMESPACE ] = "",
    [ KEYWORD_PSEUDO ] = "",
    [ KEYWORD_RESERVED ] = "",
    [ KEYWORD_TYPE ] = SEQ(TEAL),
    [ OPERATOR ] = SEQ(PURPLE),
    [ NUMBER_FLOAT ] = SEQ(DARKBLUE),
    [ NUMBER_DECIMAL ] = SEQ(DARKBLUE),
    [ NUMBER_BINARY ] = SEQ(DARKBLUE),
    [ NUMBER_OCTAL ] = SEQ(DARKBLUE),
    [ NUMBER_HEXADECIMAL ] = SEQ(DARKBLUE),
    [ COMMENT_SINGLE ] = SEQ(LIGHTGRAY),
    [ COMMENT_MULTILINE ] = SEQ(LIGHTGRAY),
    [ COMMENT_DOCUMENTATION ] = SEQ(LIGHTGRAY),
    [ STRING_SINGLE ] = SEQ(BROWN),
    [ STRING_DOUBLE ] = SEQ(BROWN),
    [ STRING_BACKTICK ] = SEQ(BROWN),
    [ STRING_INTERNED ] = SEQ(BROWN),
    [ STRING_REGEX ] = SEQ(BROWN),
    [ SEQUENCE_ESCAPED ] = SEQ(DARKGRAY ";" BOLD),
    [ SEQUENCE_INTERPOLATED ] = SEQ(DARKGRAY),
    [ LITERAL_SIZE ] = SEQ(DARKBLUE),
    [ LITERAL_DURATION ] = SEQ(DARKBLUE),
    [ GENERIC_STRONG ] = "",
    [ GENERIC_HEADING ] = SEQ(WHITE ";" BOLD),
    [ GENERIC_SUBHEADING ] = SEQ(WHITE ";" BOLD),
    [ GENERIC_INSERTED ] = SEQ(DARKBLUE),
    [ GENERIC_DELETED ] = SEQ(DARKRED),
};

static const char *sequences[_TOKEN_COUNT];

static void cache_build(const Theme *theme)
{
    size_t i;

    for (i = 0; i < _TOKEN_COUNT; i++) {
        sequences[i] = NULL;
        if (theme->styles[i].flags) {
            char *w, buffer[STR_SIZE(LONGEST_ANSI_ESCAPE_SEQUENCE)];

            *buffer = '\0';
            w = stpcpy(buffer, "\e[");
            if (theme->styles[i].bold) {
                w = stpcpy(w, "1;");
            }
            if (theme->styles[i].italic) {
                w = stpcpy(w, "3;");
            }
            if (theme->styles[i].fg_set) {
                w += sprintf(w, "38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";", theme->styles[i].fg.r, theme->styles[i].fg.g, theme->styles[i].fg.b);
            }
            if (theme->styles[i].bg_set) {
                w += sprintf(w, "48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";", theme->styles[i].bg.r, theme->styles[i].bg.g, theme->styles[i].bg.b);
            }
            w[-1] = 'm'; // overwrite last ';'
            sequences[i] = strdup(buffer);
        }
    }
}

static void cache_free(void)
{
    size_t i;

    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (NULL != sequences[i]/* && '\0' != *sequences[i]*/) {
            free((void *) sequences[i]);
        }
    }
}

static int terminal_start_document(String *UNUSED(out), FormatterData *UNUSED(data))
{
#if 1
    size_t i;

    cache_build(theme_by_name("molokai"));
    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (NULL != sequences[i]/* && '\0' != *sequences[i]*/) {
            fputs(sequences[i], stdout);
        }
        fputs(tokens[i].name, stdout);
        if (NULL != sequences[i]) {
            fputs("\e[21;23;39;49;00m", stdout);
        }
        fputc('\n', stdout);
    }
    cache_free();
#endif
#if 0
// #define violet { 0xAF, 0x87, 0xFF } or { 175, 135, 255 }
    STRING_APPEND_STRING(out, "\e[38;2;175;135;255m");
    STRING_APPEND_STRING(out, "test");
    STRING_APPEND_STRING(out, "\e[39;49;00m");
#endif

    return 0;
}

static int terminal_end_document(String *UNUSED(out), FormatterData *UNUSED(data))
{
    return 0;
}

static int terminal_start_token(int token, String *out, FormatterData *UNUSED(data))
{
    if (0 != *map[token]) {
        string_append_string(out, map[token]);
    }

    return 0;
}

static int terminal_end_token(int UNUSED(token), String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "\e[39;49;00m");

    return 0;
}

static int terminal_write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len(out, token, token_len);

    return 0;
}

const FormatterImplementation _termfmt = {
    "Terminal",
    "Format tokens with ANSI color sequences, for output in a text console",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    terminal_start_document,
    terminal_end_document,
    terminal_start_token,
    terminal_end_token,
    terminal_write_token,
    NULL,
    NULL,
    sizeof(FormatterData),
    NULL
};

/*SHALL_API*/ const FormatterImplementation *termfmt = &_termfmt;
