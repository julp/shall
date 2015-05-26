#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"

enum {
    STATE(INITIAL),
    STATE(IN_COMMENT),
    STATE(IN_SINGLE_STRING),
    STATE(IN_DOUBLE_STRING)
};

#if 0
static int canalyse(const char *src, size_t src_len)
{
    // TODO: look for \s*#\s*include\s*<stdlib.h>

    return 0;
}
#endif

/**
 * TODO
 * - macro/preprocessor: \s* # \s* (if|ifn?def|else|...|define|error) + handle \ to continue on next line
 * - escaped sequences in strings
 * - C99 types? (conditionnal? - on option)
 **/
static int clex(YYLEX_ARGS) {
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

D = [0-9];
L = [a-zA-Z_];
H = [a-fA-F0-9];
E = [Ee][+-]?D+;
FS = [fFlL];
IS = [uUlL]*;

<INITIAL> "break" | "case" | "continue" | "default" | "do" | "else" | "extern" | "if" | "for" | "goto" | "return" | "sizeof" | "static" | "switch" | "typedef" | "while" {
    PUSH_TOKEN(KEYWORD);
}

<INITIAL> "NULL" {
    PUSH_TOKEN(NAME_BUILTIN);
}

<INITIAL> "auto" | "register" | "volatile" {
    PUSH_TOKEN(KEYWORD);
}

<INITIAL> "char" | "double" | "enum" | "float" | "int" | "long" | "short" | "struct" | "union" | "void" {
    PUSH_TOKEN(KEYWORD);
}

<INITIAL> "const" | "signed" | "unsigned" {
    PUSH_TOKEN(KEYWORD);
}

<INITIAL> L (L | D)* {
    PUSH_TOKEN(IGNORABLE);
}

<INITIAL> D+ E FS? {
    PUSH_TOKEN(NUMBER_FLOAT);
}

<INITIAL> D* "." D+ E? FS? {
    PUSH_TOKEN(NUMBER_FLOAT);
}

<INITIAL> D+ "." D* E? FS? {
    PUSH_TOKEN(NUMBER_FLOAT);
}

<INITIAL> "0" 'x' H+ IS? {
    PUSH_TOKEN(NUMBER_HEXADECIMAL);
}

<INITIAL> "0" D+ IS? {
    PUSH_TOKEN(NUMBER_OCTAL);
}

<INITIAL> D+ IS? {
    PUSH_TOKEN(NUMBER_DECIMAL);
}

<INITIAL> "L"? "'" {
    BEGIN(IN_SINGLE_STRING);
    PUSH_TOKEN(STRING_SINGLE);
}

<IN_SINGLE_STRING> "'" {
    BEGIN(INITIAL);
    PUSH_TOKEN(STRING_SINGLE);
}

<IN_SINGLE_STRING> [^] {
    PUSH_TOKEN(STRING_SINGLE);
}

<INITIAL> "L"? "\"" {
    BEGIN(IN_DOUBLE_STRING);
    PUSH_TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_STRING> "\"" {
    BEGIN(INITIAL);
    PUSH_TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_STRING> [^] {
    PUSH_TOKEN(STRING_DOUBLE);
}

<INITIAL> [[\]{}(),;.] {
    PUSH_TOKEN(PUNCTUATION);
}

<INITIAL> [?:=&|!~^%<>*/+-] {
    PUSH_TOKEN(OPERATOR);
}

<INITIAL> '//' {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                //CG(zend_lineno)++;
                break;
            default:
                continue;
        }
        break;
    }
    PUSH_TOKEN(COMMENT_SINGLE);
}

<INITIAL> "/*" {
    BEGIN(IN_COMMENT);
    PUSH_TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT> [^] {
    PUSH_TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT> "*/" {
    BEGIN(INITIAL);
    PUSH_TOKEN(COMMENT_MULTILINE);
}

<INITIAL> "..." | ">>" "="? | "<<" "="? | [!=<>|&~^*/%-]"=" | "--" | "++" | "->" {
    PUSH_TOKEN(OPERATOR);
}

<*> [^] {
    PUSH_TOKEN(IGNORABLE);
}
*/
}

LexerImplementation c_lexer = {
    "C",
    0,
    "For C source code with preprocessor directives",
    NULL,
    (const char * const []) { "*.[ch]", NULL },
    (const char * const []) { "text/x-chdr", "text/x-csrc", NULL },
    NULL,
    NULL,
    NULL,
    clex,
    sizeof(LexerData),
    NULL
};
