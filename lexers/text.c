#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"

/**
 * We just consider the current character (byte) as this lexer can be used
 * by an other one.
 *
 * A better approach may be to let the higher lexer to predefine the beginning
 * and end of its unprocessable token so this lexer, in this case, can simply
 * return IGNORABLE for it. And, if there isn't any parent lexer, just set
 * YYCURSOR to YYLIMIT and return IGNORABLE.
 **/
static int textlex(YYLEX_ARGS) {
    (void) data;
    YYTEXT = YYCURSOR;
    if (YYCURSOR >= YYLIMIT) {
        return 0;
    } else {
        ++YYCURSOR;
        return IGNORABLE;
    }
}

LexerImplementation text_lexer = {
    "Text",
    0,
    "A \"dummy\" lexer that doesn't highlight anything",
    (const char * const []) { NULL },
    (const char * const []) { "*.txt", NULL },
    (const char * const []) { "text/plain", NULL },
    NULL,
    NULL,
    NULL,
    textlex,
    sizeof(LexerData),
    NULL
};
