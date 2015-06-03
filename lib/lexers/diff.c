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
    (void) data;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;

        while (YYCURSOR < YYLIMIT && '\n' != *YYCURSOR) {
            ++YYCURSOR;
        }
        if ('\n' != *YYCURSOR) {
            DONE;
        } else {
            ++YYCURSOR; // skip '\n' for next call
            switch (*YYTEXT) {
                case '+':
                    PUSH_TOKEN(GENERIC_INSERTED);
                case '-':
                    PUSH_TOKEN(GENERIC_DELETED);
                case '!':
                    PUSH_TOKEN(GENERIC_STRONG);
                case '@':
                    PUSH_TOKEN(GENERIC_SUBHEADING);
                case '=':
                    PUSH_TOKEN(GENERIC_HEADING);
                case 'i':
                case 'I':
                    if (YYLENG >= STR_LEN("index") && 0 == memcmp(YYTEXT + 1, "ndex", STR_LEN("ndex"))) {
                        PUSH_TOKEN(GENERIC_HEADING);
                    }
                    break;
                case 'd':
                    if (YYLENG >= STR_LEN("diff") && 0 == memcmp(YYTEXT + 1, "iff", STR_LEN("iff"))) {
                        PUSH_TOKEN(GENERIC_HEADING);
                    }
                    break;
            }
        }
        PUSH_TOKEN(IGNORABLE);
    }
    DONE;
}

LexerImplementation diff_lexer = {
    "Diff",
    0,
    "Lexer for unified or context-style diffs or patches",
    (const char * const []) { "udiff", NULL },
    (const char * const []) { "*.diff", "*.patch", NULL },
    (const char * const []) { "text/x-diff", "text/x-patch", NULL },
    NULL,
    NULL,
    diffanalyse,
    difflex,
    sizeof(LexerData),
    NULL,
    NULL
};
