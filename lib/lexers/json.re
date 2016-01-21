#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
    STATE(IN_STRING)
};

/**
 * NOTE:
 * - ' = case insensitive (ASCII letters only)
 * - " = case sensitive
 * (for re2c, by default, without --case-inverted or --case-insensitive)
 **/
static int jsonlex(YYLEX_ARGS) {
    (void) ctxt;
    (void) data;
    (void) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
DNUM = ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*);
EXPONENT_DNUM = ((LNUM|DNUM)[eE][+-]?LNUM);

<INITIAL> "true" | "false" | "null" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL> LNUM {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> DNUM | EXPONENT_DNUM {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> '"' {
    BEGIN(IN_STRING);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> [{}[\],:] {
    TOKEN(PUNCTUATION);
}

<IN_STRING> "\\" ( [\\"/bfnrt] | "u" [a-zA-Z0-9]{4} ) {
    TOKEN(ESCAPED_CHAR);
}

<IN_STRING> '"' {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<IN_STRING> [^] {
    TOKEN(STRING_DOUBLE);
}

<INITIAL> [^] {
    TOKEN(IGNORABLE);
}
*/
    }
    DONE();
}

LexerImplementation json_lexer = {
    "JSON",
    "For JSON data structures",
    NULL,
    (const char * const []) { "*.json", NULL },
    (const char * const []) { "application/json", NULL },
    NULL,
    NULL,
    NULL,
    jsonlex,
    sizeof(LexerData),
    NULL,
    NULL
};
