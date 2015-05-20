#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"

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
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

LNUM = [0-9]+;
DNUM = ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*);
EXPONENT_DNUM = ((LNUM|DNUM)[eE][+-]?LNUM);

<INITIAL> "true" | "false" | "null" {
    return KEYWORD_CONSTANT;
}

<INITIAL> LNUM {
    return NUMBER_DECIMAL;
}

<INITIAL> DNUM | EXPONENT_DNUM {
    return NUMBER_FLOAT;
}

<INITIAL> '"' {
    BEGIN(IN_STRING);
    return STRING_DOUBLE;
}

<INITIAL> [{}[\],:] {
    return PUNCTUATION;
}

<IN_STRING> "\\" ( [\\"/bfnrt] | "u" [a-zA-Z0-9]{4} ) {
    return ESCAPED_CHAR;
}

<IN_STRING> '"' {
    BEGIN(INITIAL);
    return STRING_DOUBLE;
}

<IN_STRING> [^] {
    return STRING_DOUBLE;
}

<INITIAL> [^] {
    return IGNORABLE;
}
*/
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
    NULL
};
