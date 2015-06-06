#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

static int textlex(YYLEX_ARGS) {
    (void) data;
    YYTEXT = YYCURSOR;
    if (YYCURSOR < YYLIMIT) {
        YYCURSOR = YYLIMIT;
        cb(EVENT_TOKEN, cb_data, IGNORABLE);
    }
    DONE;
}

LexerImplementation text_lexer = {
    "Text",
    0,
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
