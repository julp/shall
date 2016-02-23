/**
 * Spec/language reference:
 * - http://www.lua.org/manual/5.3/manual.html#3
 */

#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

typedef struct {
    LexerData data;
    int bracket_len; // total length of "[" "="* "["
} LuaLexerData;

enum {
    STATE(INITIAL),
    STATE(IN_DOUBLE_QUOTED_STRING),
    STATE(IN_SINGLE_QUOTED_STRING),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_DOUBLE_QUOTED_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_SINGLE_QUOTED_STRING) ] = STRING_DOUBLE,
};

static int yylex(YYLEX_ARGS)
{
    LuaLexerData *mydata;

    (void) ctxt;
    (void) options;
    mydata = (LuaLexerData *) data;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    } else {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

identifier = [a-zA-Z_][a-zA-Z_0-9]*;
digit = [0-9];
hexdigit = [0-9a-fA-F];
fractional_part = "." digit+;
decimal_exponent = 'e' [+-]? digit*;
binary_exponent = 'p' [+-]? digit*;
bracket_open = "[" "="* "[";
bracket_close = "]" "="* "]";

<INITIAL> "false" | "true" | "nil" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL> "and" | "not" | "or" {
    TOKEN(OPERATOR);
}

<INITIAL> "break" | "do" | "else" | "elseif" | "end" | "for" | "function" | "goto" | "if" | "in" | "local" | "repeat" | "return" | "then" | "until" | "while" {
    TOKEN(KEYWORD);
}

<INITIAL> identifier {
    TOKEN(NAME_VARIABLE);
}

<INITIAL> [-+/*%^#&~|(){}[\];,] | [~<>]"=" | [=:<>/]{1,2} | "."{1,3} {
    TOKEN(OPERATOR);
}

<INITIAL> "--"? bracket_open {
    bool comment;
    YYCTYPE *end;
    size_t bracket_len;
    char bracket_close[YYLENG];

    comment = '-' == *YYTEXT;
    bracket_len = YYLENG - (comment ? STR_LEN("--") : 0);
    memcpy(bracket_close, YYTEXT + (comment ? STR_LEN("--") : 0), bracket_len);
    bracket_close[0] = bracket_close[bracket_len - 1] = ']';
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, bracket_close, bracket_len, (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + bracket_len;
    }
    if (comment) {
        TOKEN(COMMENT_MULTILINE);
    } else {
        TOKEN(STRING_SINGLE);
    }
}

<INITIAL> "--" [^\n]* {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> digit+ {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> digit+ fractional_part? decimal_exponent? {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> '0x' hexdigit+ ("." hexdigit+)? binary_exponent? {
    TOKEN(NUMBER_HEXADECIMAL);
}

<INITIAL> '"' {
    BEGIN(IN_DOUBLE_QUOTED_STRING);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> "'" {
    BEGIN(IN_SINGLE_QUOTED_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_QUOTED_STRING,IN_SINGLE_QUOTED_STRING> "\\" ([abfnrtv\\z0] | "x" hexdigit{2} | [0-9]{1,3} | "u{" hexdigit+ "}") {
    TOKEN(ESCAPED_CHAR);
}

<IN_DOUBLE_QUOTED_STRING> '\\"' {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_QUOTED_STRING> "\\'" {
    TOKEN(ESCAPED_CHAR);
}

<IN_DOUBLE_QUOTED_STRING> '"' {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<IN_SINGLE_QUOTED_STRING> "'" {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
}

LexerImplementation lua_lexer = {
    "Lua",
    "For the Lua programming language (lua.org)",
    NULL, // aliases
    (const char * const []) { "*.lua", NULL },
    (const char * const []) { "text/x-lua", "application/x-lua", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    yylex,
    NULL, // finalize
    sizeof(LuaLexerData),
    NULL, // options
    NULL // dependencies
};
