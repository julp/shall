/**
 * @file lib/tokens.c
 * @brief functions relative to token types
 */

#include "cpp.h"
#include "tokens.h"

SHALL_API const Token tokens[_TOKEN_COUNT] = {
#define TOKEN(constant, parent, description, cssclass) \
    [ constant ] = { constant, parent, #constant, STR_LEN(#constant), description, cssclass },
#include "keywords.h"
#undef TOKEN
};

SHALL_API void token_each(void (*cb)(const Token *, void *), void *data)
{
    size_t i;

    for (i = 0; i < _TOKEN_COUNT; i++) {
        cb(&tokens[i], data);
    }
}

SHALL_API const Token *token_parent(const Token *token)
{
    return -1 == token->parent ? NULL : &tokens[token->parent];
}

SHALL_API const char *token_name(const Token *token)
{
    return token->name;
}

SHALL_API const char *token_description(const Token *token)
{
    return token->description;
}

#if 0
SHALL_API void tokens_to_iterator(Iterator *it)
{
    //
}
#endif
