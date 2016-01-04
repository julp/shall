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

// TODO: DRY (html_formatter)
#define STRING_APPEND_STRING(string, suffix) \
    do { \
        string_append_string_len(string, suffix, STR_LEN(suffix)); \
    } while (0);

#define STRING_APPEND_COLOR(string, color) \
    do { \
        string_append_formatted(string, "#%02X%02X%02X", color.r, color.g, color.b); \
    } while (0);

/**
 * TODO
 *
 * @param theme
 * @param scope
 * 
 * @return a string describing the theme in CSS format
 */
SHALL_API char *theme_css(const Theme *theme, const char *scope, bool UNUSED(pretty_print))
{
    size_t i;
    String *buffer;

    buffer = string_new();
    for (i = 0; i < _LAST_TOKEN; i++) {
        if (NULL != scope) {
            string_append_string(buffer, scope);
            string_append_char(buffer, ' ');
        }
        string_append_string(buffer, map[i]);
        STRING_APPEND_STRING(buffer, "{\n");
        if (0 == memcmp(&theme->styles[i].bg, &undefined_color, sizeof(undefined_color))) {
            STRING_APPEND_STRING(buffer, "\tbackground-color: ");
            STRING_APPEND_COLOR(buffer, theme->styles[i].bg);
            STRING_APPEND_STRING(buffer, ";\n");
        }
        if (0 == memcmp(&theme->styles[i].fg, &undefined_color, sizeof(undefined_color))) {
            STRING_APPEND_STRING(buffer, "\tcolor: ");
            STRING_APPEND_COLOR(buffer, theme->styles[i].fg);
            STRING_APPEND_STRING(buffer, ";\n");
        }
        if (theme->styles[i].bold) {
            STRING_APPEND_STRING(buffer, "\tfont-weight: bold;\n");
        }
        if (theme->styles[i].italic) {
            STRING_APPEND_STRING(buffer, "\tfont-style: italic;\n");
        }
        STRING_APPEND_STRING(buffer, "}\n");
    }

    return string_orphan(buffer);
}
