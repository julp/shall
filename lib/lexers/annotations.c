#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

#define IS_SPACE(c) \
    (' ' == (c) || '\t' == (c))

static int annotationslex(YYLEX_ARGS)
{
    (void) data;
    (void) options;
    YYTEXT = YYCURSOR;
    YYCURSOR += STR_LEN("/*"); // should be safe as this should be the prealable condition to the call of annotations_lexer
    while (YYCURSOR < YYLIMIT) {
        while (YYCURSOR < YYLIMIT && IS_SPACE(*YYCURSOR)) {
            ++YYCURSOR;
        }
        while (YYCURSOR < YYLIMIT && '*' == *YYCURSOR) {
            ++YYCURSOR;
        }
        while (YYCURSOR < YYLIMIT && IS_SPACE(*YYCURSOR)) {
            ++YYCURSOR;
        }
        if ('@' == *YYCURSOR) {
            TOKEN(COMMENT_MULTILINE);
            YYTEXT = YYCURSOR;
            ++YYCURSOR;
            while (YYCURSOR < YYLIMIT && !IS_SPACE(*YYCURSOR)) {
                ++YYCURSOR;
            }
            TOKEN(KEYWORD);
            YYTEXT = YYCURSOR;
        } else {
            // handle XXX, NOTE, TODO, BUG?
        }
        while (YYCURSOR < YYLIMIT && /*(*/'\n' != *YYCURSOR /* || '\r' != *YYCURSOR) */) {
            if ('*' == *YYCURSOR && YYCURSOR < YYLIMIT && '/' == YYCURSOR[1]) {
                YYCURSOR += STR_LEN("*/");
                goto end;
            }
            ++YYCURSOR;
        }
        NEWLINE;
        ++YYCURSOR; // '\n'
    }
end:
    TOKEN(COMMENT_MULTILINE);
    DONE;
}

LexerImplementation annotations_lexer = {
    "Annotations", // unused
    "", // unused
    NULL, // aliases
    NULL, // pattern
    NULL, // mimetypes
    NULL, // interpreters (shebang)
    NULL, // analyze
    NULL, // init
    annotationslex,
    NULL, // finalize
    sizeof(LexerData),
    NULL, // options
    NULL // dependencies
};
