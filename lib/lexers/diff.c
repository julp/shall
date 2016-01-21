#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "lexer.h"

static int diffanalyse(const char *src, size_t src_len)
{
    if (src_len >= STR_LEN("Index: ") && 0 == memcmp(src, "Index: ", STR_LEN("Index: "))) {
        return 999;
    }
    if (src_len >= STR_LEN("diff ") && 0 == memcmp(src, "diff ", STR_LEN("diff "))) {
        return 999;
    }
    if (src_len >= STR_LEN("--- ") && 0 == memcmp(src, "--- ", STR_LEN("--- "))) {
        return 600;
    }

    return 0;
}

static int difflex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;

    if (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;

        while (YYCURSOR < YYLIMIT && !IS_NL(*YYCURSOR)) {
            ++YYCURSOR;
        }
        HANDLE_CR_LF;
        if (IS_NL(*YYCURSOR)) {
            ++YYCURSOR; // skip newline ([\r\n]) for next call
            switch (*YYTEXT) {
                case '+':
                    TOKEN(GENERIC_INSERTED);
                case '-':
                    TOKEN(GENERIC_DELETED);
                case '!':
                    TOKEN(GENERIC_STRONG);
                case '@':
                    TOKEN(GENERIC_SUBHEADING);
                case '=':
                    TOKEN(GENERIC_HEADING);
                case 'i':
                case 'I':
                    if (((size_t) YYLENG) >= STR_LEN("index") && 0 == memcmp(YYTEXT + 1, "ndex", STR_LEN("ndex"))) {
                        TOKEN(GENERIC_HEADING);
                    }
                    break;
                case 'd':
                    if (((size_t) YYLENG) >= STR_LEN("diff") && 0 == memcmp(YYTEXT + 1, "iff", STR_LEN("iff"))) {
                        TOKEN(GENERIC_HEADING);
                    }
                    break;
            }
        }
        TOKEN(IGNORABLE);
    }
    DONE();
}

LexerImplementation diff_lexer = {
    "Diff",
    "Lexer for unified or context-style diffs or patches",
    (const char * const []) { "udiff", NULL },
    (const char * const []) { "*.diff", "*.patch", NULL },
    (const char * const []) { "text/x-diff", "text/x-patch", NULL },
    NULL, // interpreters
    diffanalyse,
    NULL, // init
    difflex,
    NULL, // finalyze
    sizeof(LexerData),
    NULL, // options
    NULL // dependencies
};
