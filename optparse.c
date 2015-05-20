#include <stdlib.h>
#include <string.h>

#include "optparse.h"

void options_init(Options *opt)
{
    opt->options = NULL;
    opt->options_len = opt->options_size = 0;
}

void options_add(Options *opt, const char *optarg/*, const char **name, const char **value*/)
{
    char *p;

    if (opt->options_len >= opt->options_size) {
        opt->options_size = 0 == opt->options_size ? 8 : opt->options_size << 1;
        opt->options = realloc(opt->options, opt->options_size * sizeof(*opt->options));
    }
    opt->options[opt->options_len].name = strdup(optarg);
    if (NULL == (p = strchr(opt->options[opt->options_len].name, '='))) {
        opt->options[opt->options_len].value = "";
        opt->options[opt->options_len].name_len = strlen(optarg);
        opt->options[opt->options_len].value_len = 0;
    } else {
        *p = '\0';
        opt->options[opt->options_len].value = p + 1;
        opt->options[opt->options_len].value_len = strlen(opt->options[opt->options_len].value);
        opt->options[opt->options_len].name_len = p - opt->options[opt->options_len].name - 1;
    }
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

void options_free(Options *opt)
{
    size_t o;

    for (o = 0; o < opt->options_len; o++) {
        free((void *) opt->options[o].name); // quiet on const qualifier
    }
    if (NULL != opt->options) {
        free(opt->options);
    }
}
