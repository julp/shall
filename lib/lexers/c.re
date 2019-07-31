#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "utils.h"

extern const LexerImplementation annotations_lexer, cpp_lexer;

enum {
    STATE(INITIAL),
    STATE(IN_COMMENT),
    STATE(IN_SINGLE_STRING),
    STATE(IN_DOUBLE_STRING)
};

static int default_token_type[] = {
    IGNORABLE,         // INITIAL
    COMMENT_MULTILINE, // IN_COMMENT
    STRING_SINGLE,     // IN_SINGLE_STRING
    STRING_DOUBLE,     // IN_DOUBLE_STRING
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
static int clex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;

    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

D = [0-9];
L = [a-zA-Z_];
H = [a-fA-F0-9];
E = [Ee][+-]?D+;
FS = [fFlL];
IS = [uUlL]*;

TABS_AND_SPACES = [ \t]*;
NEWLINE = ("\r"|"\n"|"\r\n");

<INITIAL>TABS_AND_SPACES "#" {
    if (!IS_BOL) {
        yyless(0);
    } else {
#if 0
        //--YYCURSOR;
        prepend_lexer_implementation(ctxt, &cpp_lexer);
        DELEGATE_FULL(IGNORABLE);
#else
        bool escaped_eol;

        escaped_eol = true;
        while (escaped_eol && YYCURSOR < YYLIMIT) {
            while (YYCURSOR < YYLIMIT && !IS_NL(*YYCURSOR)) {
                ++YYCURSOR;
            }
            if (IS_NL(*YYCURSOR)) {
                escaped_eol = '\\' == YYCURSOR[-1];
                HANDLE_CR_LF;
                ++YYCURSOR; // skip newline ([\r\n]) for next call
            } else {
                escaped_eol = false;
            }
        }
        TOKEN(COMMENT_SINGLE);
#endif
    }
}

<INITIAL> "break" | "case" | "continue" | "default" | "do" | "else" | "extern" | "if" | "for" | "goto" | "return" | "sizeof" | "static" | "switch" | "typedef" | "while" {
    TOKEN(KEYWORD);
}

<INITIAL> "NULL" {
    TOKEN(NAME_BUILTIN);
}

<INITIAL> "auto" | "register" | "volatile" {
    TOKEN(KEYWORD);
}

<INITIAL> "char" | "double" | "enum" | "float" | "int" | "long" | "short" | "struct" | "union" | "void" {
    TOKEN(KEYWORD);
}

<INITIAL> "const" | "signed" | "unsigned" {
    TOKEN(KEYWORD);
}

<INITIAL> L (L | D)* {
    TOKEN(IGNORABLE);
}

<INITIAL> D+ E FS? {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> D* "." D+ E? FS? {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> D+ "." D* E? FS? {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> "0" 'x' H+ IS? {
    TOKEN(NUMBER_HEXADECIMAL);
}

<INITIAL> "0" D+ IS? {
    TOKEN(NUMBER_OCTAL);
}

<INITIAL> D+ IS? {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> "L"? "'" {
    BEGIN(IN_SINGLE_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_SINGLE_STRING> "'" {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<INITIAL> "L"? "\"" {
    BEGIN(IN_DOUBLE_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_STRING> "\"" {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> [[\]{}(),;.] {
    TOKEN(PUNCTUATION);
}

<INITIAL> [?:=&|!~^%<>*/+-] {
    TOKEN(OPERATOR);
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
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "/*" {
#if 1
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
#else
    YYCTYPE *end;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    }
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "*/", STR_LEN("*/"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + 1;
    }
    DELEGATE_UNTIL(COMMENT_MULTILINE);
#endif
}

<IN_COMMENT> "*/" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> "..." | ">>" "="? | "<<" "="? | [!=<>|&~^*/%-]"=" | "--" | "++" | "->" {
    TOKEN(OPERATOR);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation c_lexer = {
    "C",
    "For C source code with preprocessor directives",
    NULL, // aliases
    (const char * const []) { "*.[ch]", NULL },
    (const char * const []) { "text/x-chdr", "text/x-csrc", NULL },
    NULL, // interpreters (shebang)
    NULL, // analyse
    NULL, // init
    clex, // yylex
    NULL, // finalize
    sizeof(LexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
