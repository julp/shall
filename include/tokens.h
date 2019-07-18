#pragma once

#include <stddef.h> /* size_t */

enum {
#define TOKEN(constant, parent, description, cssclass) \
    constant,
#include "keywords.h"
#undef TOKEN
    _TOKEN_COUNT
};

typedef struct {
    int value;
    int parent;
    const char *name;
    size_t name_len;
    const char *description;
    const char *cssclass;
} Token;

extern /*SHALL_API*/ const Token tokens[_TOKEN_COUNT];
