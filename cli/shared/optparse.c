/**
 * @file cli/shared/optparse.c
 * @brief helpers for options storing
 */

#include <stdlib.h>
#include <string.h>

#include "optparse.h"

/**
 * TODO:
 *
 * caller have to free(option->name)
 */
void option_parse(const char *optarg, Option *option)
{
    char *p;

    option->name = strdup(optarg);
    if (NULL == (p = strchr(option->name, '='))) {
        option->value = (char *) "";
        option->value_len = 0;
        option->name_len = strlen(optarg);
    } else {
        *p = '\0';
        option->value = p + 1;
        option->value_len = strlen(option->value);
        option->name_len = p - option->name - 1;
    }
}

/**
 * Initialiaze a store of string pairs (key/value)
 *
 * @param opt the container
 */
void options_store_init(OptionsStore *opt)
{
    opt->options = NULL;
    opt->options_len = opt->options_size = 0;
}

/**
 * Parse a raw value in the form "key=value" or "key" (empty string
 * will be assigned as value in the later case)
 *
 * @param opt the store
 * @param optarg the string to parse to extract key and value. The
 * original string is copied so if it was allocated, it can be
 * safely freed when you don't need it.
 */
void options_store_add(OptionsStore *opt, const char *optarg/*, const char **name, const char **value*/)
{
    if (opt->options_len >= opt->options_size) {
        opt->options_size = 0 == opt->options_size ? 8 : opt->options_size << 1;
        opt->options = realloc(opt->options, opt->options_size * sizeof(*opt->options));
    }
    option_parse(optarg, &opt->options[opt->options_len]);
#if 0
    if (NULL != name) {
        *name = opt->options[opt->options_len].name;
    }
    if (NULL != value) {
        *value = opt->options[opt->options_len].value;
    }
#endif
    ++opt->options_len;
}

/**
 * Clear a key/pair store for reuse
 *
 * @param opt the store to clear
 */
void options_store_clear(OptionsStore *opt)
{
    size_t o;

    for (o = 0; o < opt->options_len; o++) {
        free(opt->options[o].name);
    }
    opt->options_len = 0;
}

/**
 * Free memory used by a store
 *
 * @param opt the store to destroy
 */
void options_store_free(OptionsStore *opt)
{
    size_t o;

    for (o = 0; o < opt->options_len; o++) {
        free((void *) opt->options[o].name); // quiet on const qualifier
    }
    if (NULL != opt->options) {
        free(opt->options);
    }
}
