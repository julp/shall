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

/*
static char *find_starting_var(GlobalState *yy, char *start, char *end)
{
    for ( ; start < end - 1; start++) {
        if ('$' == start[0] && '{' == start[1]) {
            return start;
        }
    }

    return NULL;
}

static char *find_ending_var(GlobalState *yy, char *start, char *end)
{
    return memchr(start, '}', end - start);
}
*/

/**
 * TODO:
 * - ${ VAR }
 * - le > des tags à remettre en TAG_NAME
 * - expressions ? (SetEnvIfExpr, If, ElseIf, RewriteCond expr, Require expr, SSLRequire)
 **/
static int apachelex(YYLEX_ARGS) {
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
SPACE = [ \f\n\r\t\v];
EOS = [\000];

<INITIAL> SPACE+ {
    PUSH_TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> "\\"[\n] {
    PUSH_TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [\n] {
    BEGIN(INITIAL);
    PUSH_TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [ \f\r\t\v]+ {
    PUSH_TOKEN(IGNORABLE);
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
    PUSH_TOKEN(COMMENT_SINGLE);
}

<INITIAL> [^ #\f\n\r\t\v][^ \f\n\r\t\v]* {
    BEGIN(IN_DIRECTIVE);
    if ('<' == YYTEXT[0]) {
        PUSH_TOKEN(NAME_TAG);
    } else {
        PUSH_TOKEN(KEYWORD_BUILTIN);
    }
}

<IN_DIRECTIVE> [^ \f\n\r\t\v\000]+ {
    PUSH_TOKEN(STRING_SINGLE);
}

<IN_DIRECTIVE> LNUM {
    PUSH_TOKEN(NUMBER_DECIMAL);
}

<INITIAL,IN_DIRECTIVE> '"' {
    BEGIN(IN_DOUBLE_STRING);
    PUSH_TOKEN(STRING_SINGLE);
}

<INITIAL,IN_DIRECTIVE> '\'' {
    BEGIN(IN_SINGLE_STRING);
    PUSH_TOKEN(STRING_SINGLE);
}

<IN_DOUBLE_STRING> '\\'[\\"] {
    PUSH_TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_STRING> '\\'[\\'] {
    PUSH_TOKEN(ESCAPED_CHAR);
}

<*> EOS {
    return 0;
}

/*
<INITIAL,IN_DIRECTIVE,IN_DOUBLE_STRING,IN_SINGLE_STRING> '${' [^:]+ '}' {
    return ;
}
*/
*/
}

LexerImplementation apache_lexer = {
    "Apache",
    0,
    "Lexer for configuration files following the Apache configuration file format (including .htaccess)",
    (const char * const []) { "apacheconf", NULL },
    (const char * const []) { "httpd.conf", "apache.conf", "apache2.conf", ".htaccess", NULL },
    (const char * const []) { "text/x-apacheconf", NULL },
    NULL,
    NULL,
    NULL,
    apachelex,
    sizeof(LexerData),
    NULL
};
