#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

typedef struct {
    OptionValue secondary ALIGNED(sizeof(OptionValue));
} DiffLexerOption;

static void diffinit(const OptionValue *options, LexerData *UNUSED(data), void *ctxt)
{
    Lexer *secondary;
    const DiffLexerOption *myoptions;

    myoptions = (const DiffLexerOption *) options;
    secondary = LEXER_UNWRAP(myoptions->secondary);
    if (NULL != secondary) {
        append_lexer(ctxt, secondary);
    }
}

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
        DELEGATE_UNTIL(IGNORABLE);
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
    diffinit, // init
    difflex,
    NULL, // finalize
    sizeof(LexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("secondary"), OPT_TYPE_LEXER, offsetof(DiffLexerOption, secondary), OPT_DEF_LEXER, "Lexer to highlight non diff lines (if none, these parts will not be highlighted)" },
        END_OF_OPTIONS
    },
    NULL // dependencies
};
