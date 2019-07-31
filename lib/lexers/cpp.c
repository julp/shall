#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

#if 0
pragma
include
define
endif
if
ifn?def
else
elsif
error

if 0 => commentaire (idem pour le else de if 1 ?)

le \ en fin de ligne continue la macro
#endif

static int cpplex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;

    if (YYCURSOR < YYLIMIT) {
        bool escaped_eol;

        YYTEXT = YYCURSOR;
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
        DONE_AFTER_TOKEN(STRING_SINGLE);
    }
    DONE();
}

LexerImplementation cpp_lexer = {
    "CPP",
    "C preprocessor",
    NULL, // aliases
    NULL, // pattern
    NULL, // mimetypes
    NULL, // interpreters (shebang)
    NULL, // analyse
    NULL, // init
    cpplex,
    NULL, // finalize
    sizeof(LexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
