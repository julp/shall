#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"

enum {
    STATE(INITIAL),
    STATE(IN_DIRECTIVE),
    STATE(IN_SINGLE_STRING),
    STATE(IN_DOUBLE_STRING),
};

/**
 * TODO:
 * - gérer les LITERAL_DURATION et LITERAL_SIZE (cf http://nginx.org/en/docs/syntax.html)
 * - une variable ne sera pas interpolée dans une expression régulière
 * - pour l'interpolation, lever toute ambiguité notamment, le nom d'une variable peut être entourée d'accolades
 **/
static int nginxlex(YYLEX_ARGS) {
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

LNUM = [0-9]+;
SPACE = [ \n\r\t];

<INITIAL> SPACE+ {
    return IGNORABLE;
}

<IN_DIRECTIVE> "\\"[\n] {
    return IGNORABLE;
}

<IN_DIRECTIVE> [\n] {
    BEGIN(INITIAL);
    return IGNORABLE;
}

<IN_DIRECTIVE> [ \n\r\t]+ {
    return IGNORABLE;
}

<INITIAL> [#] {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                //YYLINENO++;
                break;
            default:
                continue;
        }
        break;
    }
    return COMMENT_SINGLE;
}

<INITIAL> [;{}] {
    return PUNCTUATION;
}

<IN_DIRECTIVE> [;{] {
    BEGIN(INITIAL);
    return PUNCTUATION;
}

<INITIAL> [^$#;{} \r\n\t][^;{} \n\r\t]* {
    BEGIN(IN_DIRECTIVE);
    return KEYWORD_BUILTIN;
}

<IN_DIRECTIVE> "$"[^;{}() \n\r\t]+ {
    return NAME_VARIABLE;
}

<IN_DIRECTIVE> [^$#;{}() \n\r\t]+ {
    return STRING_SINGLE;
}

<IN_DIRECTIVE> LNUM {
    return NUMBER_INTEGER;
}

<INITIAL,IN_DIRECTIVE> '"' {
    BEGIN(IN_DOUBLE_STRING);
    return STRING_SINGLE;
}

<INITIAL,IN_DIRECTIVE> '\'' {
    BEGIN(IN_SINGLE_STRING);
    return STRING_SINGLE;
}

<IN_DOUBLE_STRING> '\\'[\\"] {
    return ESCAPED_CHAR;
}

<IN_SINGLE_STRING> '\\'[\\'] {
    return ESCAPED_CHAR;
}

<*> [^] {
    return IGNORABLE;
}
*/
}

LexerImplementation nginx_lexer = {
    "Nginx",
    0,
    "Lexer for Nginx configuration files",
    (const char * const []) { "nginxconf", NULL },
    (const char * const []) { "nginx.conf", NULL },
    (const char * const []) { "text/x-nginx-conf", NULL },
    NULL,
    NULL,
    NULL,
    nginxlex,
    sizeof(LexerData),
    NULL
};
