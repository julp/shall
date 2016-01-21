#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

static int textlex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;

    YYTEXT = YYCURSOR;
    if (YYCURSOR < YYLIMIT) {
        YYCURSOR = YYLIMIT;
        TOKEN(IGNORABLE);
    }
    DONE();
}

LexerImplementation text_lexer = {
    "Text",
    "A \"dummy\" lexer that doesn't highlight anything",
    (const char * const []) { "txt", NULL },
    (const char * const []) { "*.txt", NULL },
    (const char * const []) { "text/plain", NULL },
    NULL,
    NULL,
    NULL,
    textlex,
    sizeof(LexerData),
    NULL,
    NULL
};
