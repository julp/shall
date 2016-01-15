#include "cpp.h"
#include "tokens.h"

/*SHALL_API */const Token tokens[_TOKEN_COUNT] = {
#define TOKEN(constant, description, cssclass) \
    [ constant ] = { constant, #constant, STR_LEN(#constant), description, cssclass },
#include "keywords.h"
#undef TOKEN
};

#if 0
SHALL_API void token_each(const Token *token, void *data)
{
    //
}
#endif
