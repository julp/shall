#pragma once

#ifndef DOXYGEN
# define OPT_STRVAL_FREE(opt, def) \
    do { \
        if (NULL != OPT_STRVAL(opt) && OPT_STRVAL(opt) != OPT_STRVAL(def)) { \
            free((char *) OPT_STRVAL(opt)); \
        } \
    } while (0);
#endif /* !DOXYGEN */

#include "types.h"

void option_copy(OptionType, OptionValue *, OptionValue, OptionValue);
int option_parse_as_string(OptionValue *, int, const char *, size_t, int);
