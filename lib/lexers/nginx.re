#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

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
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
SPACE = [ \n\r\t];

<INITIAL> SPACE+ {
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> "\\"[\n] {
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [\n] {
    BEGIN(INITIAL);
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [ \n\r\t]+ {
    TOKEN(IGNORABLE);
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
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> [;{}] {
    TOKEN(PUNCTUATION);
}

<IN_DIRECTIVE> [;{] {
    BEGIN(INITIAL);
    TOKEN(PUNCTUATION);
}

<INITIAL> [^$#;{} \r\n\t][^;{} \n\r\t]* {
    BEGIN(IN_DIRECTIVE);
    TOKEN(KEYWORD_BUILTIN);
}

<IN_DIRECTIVE> "$"[^;{}() \n\r\t]+ {
    TOKEN(NAME_VARIABLE);
}

<IN_DIRECTIVE> [^$#;{}() \n\r\t]+ {
    TOKEN(STRING_SINGLE);
}

<IN_DIRECTIVE> LNUM {
    TOKEN(NUMBER_INTEGER);
}

<INITIAL,IN_DIRECTIVE> '"' {
    BEGIN(IN_DOUBLE_STRING);
    TOKEN(STRING_SINGLE);
}

<INITIAL,IN_DIRECTIVE> '\'' {
    BEGIN(IN_SINGLE_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_DOUBLE_STRING> '\\'[\\"] {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_STRING> '\\'[\\'] {
    TOKEN(ESCAPED_CHAR);
}

<*> [^] {
    TOKEN(IGNORABLE);
}
*/
    }
    DONE();
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
    NULL,
    NULL
};
