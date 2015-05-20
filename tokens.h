#ifndef TOKENS_H

# define TOKENS_H

enum {
#define TOKEN(constant, description, cssclass) \
    constant,
#include "keywords.h"
#undef TOKEN
    _LAST_TOKEN
};

#endif /* !TOKENS_H */
