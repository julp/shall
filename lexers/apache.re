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
    return IGNORABLE;
}

<IN_DIRECTIVE> "\\"[\n] {
    return IGNORABLE;
}

<IN_DIRECTIVE> [\n] {
    BEGIN(INITIAL);
    return IGNORABLE;
}

<IN_DIRECTIVE> [ \f\r\t\v]+ {
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

<INITIAL> [^ #\f\n\r\t\v][^ \f\n\r\t\v]* {
    BEGIN(IN_DIRECTIVE);
    if ('<' == YYTEXT[0]) {
        return NAME_TAG;
    } else {
        return KEYWORD_BUILTIN;
    }
}

<IN_DIRECTIVE> [^ \f\n\r\t\v\000]+ {
    return STRING_SINGLE;
}

<IN_DIRECTIVE> LNUM {
    return NUMBER_DECIMAL;
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
