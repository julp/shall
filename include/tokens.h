#ifndef TOKENS_H

# define TOKENS_H

enum {
#define TOKEN(constant, description, cssclass) \
    constant,
#include "keywords.h"
#undef TOKEN
    _TOKEN_COUNT
};

typedef struct {
    int value;
    const char *name;
    const char *description;
    const char *cssclass;
} Token;

/*SHALL_API */const Token tokens[_TOKEN_COUNT];

#endif /* !TOKENS_H */
