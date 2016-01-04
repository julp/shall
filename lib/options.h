#ifndef OPTIONS_H

# define OPTIONS_H

# ifndef DOXYGEN
#  define OPT_STRVAL_FREE(opt, def) \
    do { \
        if (NULL != OPT_STRVAL(opt) && OPT_STRVAL(opt) != OPT_STRVAL(def)) { \
            free((char *) OPT_STRVAL(opt)); \
        } \
    } while (0);
# endif /* !DOXYGEN */

# include "types.h"

void opt_copy(OptionType, OptionValue *, OptionValue, OptionValue);
int parse_option_as_string(OptionValue *, int, const char *, size_t, int);

#endif /* !OPTIONS_H */
