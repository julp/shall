#include <stdlib.h>

#include "cpp.h"
#include "utils.h"
#include "themes.h"

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
