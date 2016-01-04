#include <stddef.h>
#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "themes.h"
#include "xtring.h"

extern SHALL_API const Theme monokai;
extern SHALL_API const Theme molokai;

static const Theme *available_themes[] = {
    &monokai,
    &molokai,
};

/**
 * Exposes the number of available builtin themes
 *
 * @note for external use only, keep using ARRAY_SIZE internally
 */
SHALL_API const size_t SHALL_THEME_COUNT = ARRAY_SIZE(available_themes);

/**
 * Executes the given callback for each builtin lexer implementation
 *
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void theme_each(void (*cb)(const Theme *, void *), void *data)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_themes); i++) {
        cb(available_themes[i], data);
    }
}

/**
 * Gets name of a theme
 *
 * @param theme the theme "instance"
 *
 * @return its name
 */
SHALL_API const char *theme_name(const Theme *theme)
{
    return theme->name;
}

/**
 * Gets a theme from its name.
 *
 * @param name the theme's name
 *
 * @return NULL or the Theme of the given name
 */
SHALL_API const Theme *theme_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_themes); i++) {
        if (0 == ascii_strcasecmp(name, available_themes[i]->name)) {
            return available_themes[i];
        }
    }

    return NULL;
}

static const Color undefined_color/* = { .r = 0, .g = 0, .b = 0 }*/;

// TODO: DRY (html_formatter)
static const char * const map[] = {
#define TOKEN(constant, description, cssclass) \
    cssclass,
#include "keywords.h"
#undef TOKEN
};

#define IDENT_STRING "  "

// TODO: DRY (html_formatter)
#define STRING_APPEND_STRING(string, suffix) \
    do { \
        string_append_string_len(string, suffix, STR_LEN(suffix)); \
    } while (0);

#define STRING_APPEND_IDENT(string) \
    do { \
        if (pretty_print) { \
            string_append_string_len(string, IDENT_STRING, STR_LEN(IDENT_STRING)); \
        } \
    } while (0);

#define STRING_APPEND_COLOR(string, color) \
    do { \
        string_append_formatted(string, "#%02X%02X%02X", color.r, color.g, color.b); \
    } while (0);

#include "hashtable.h"

struct hashed_style {
    union {
        Style style;
        ht_hash_t h;
    };
};

static ht_hash_t hash_style(ht_key_t data)
{
    Style *style;
    struct hashed_style h;

    style = (void *) data;
    memcpy(&h.style, style, sizeof(*style));

    return h.h;
}

/**
 * Generates CSS for a theme
 *
 * @param theme
 * @param scope
 * 
 * @return a string describing the theme in CSS format
 */
SHALL_API char *theme_css(const Theme *theme, const char *scope, bool pretty_print)
{
    size_t i;
    String *buffer;
    HashTable groups;
    Style **styles[_LAST_TOKEN][_LAST_TOKEN] = { { NULL } };

    buffer = string_new();
    hashtable_init(&groups, 0, hash_style, value_equal, NULL, NULL, NULL);
    // TODO: regroup output by token which share the same style
    for (i = 0; i < _LAST_TOKEN; i++) {
        if (' ' != *map[i]) {
            bool has_fg, has_bg;

            has_fg = 0 != memcmp(&theme->styles[i].fg, &undefined_color, sizeof(undefined_color));
            has_bg = 0 != memcmp(&theme->styles[i].bg, &undefined_color, sizeof(undefined_color));
            if (has_fg || has_bg || theme->styles[i].bold || theme->styles[i].italic) {
                if (NULL != scope) {
                    string_append_string(buffer, scope);
                    string_append_char(buffer, ' ');
                }
                string_append_char(buffer, '.');
                string_append_string(buffer, map[i]);
                STRING_APPEND_STRING(buffer, " {\n"); // TODO: add a description in comment
                if (has_bg) {
                    STRING_APPEND_IDENT(buffer);
                    STRING_APPEND_STRING(buffer, "background-color: ");
                    STRING_APPEND_COLOR(buffer, theme->styles[i].bg);
                    STRING_APPEND_STRING(buffer, ";\n");
                }
                if (has_fg) {
                    STRING_APPEND_IDENT(buffer);
                    STRING_APPEND_STRING(buffer, "color: ");
                    STRING_APPEND_COLOR(buffer, theme->styles[i].fg);
                    STRING_APPEND_STRING(buffer, ";\n");
                }
                if (theme->styles[i].bold) {
                    STRING_APPEND_IDENT(buffer);
                    STRING_APPEND_STRING(buffer, "font-weight: bold;\n");
                }
                if (theme->styles[i].italic) {
                    STRING_APPEND_IDENT(buffer);
                    STRING_APPEND_STRING(buffer, "font-style: italic;\n");
                }
                STRING_APPEND_STRING(buffer, "}\n");
            }
        }
    }
    hashtable_destroy(&groups);

    return string_orphan(buffer);
}
