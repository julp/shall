/**
 * Spec/language reference:
 * - http://twig.sensiolabs.org/doc/templates.html
 * - http://twig.sensiolabs.org/documentation
 */

#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

enum {
    TWIG_STATEMENT,
    TWIG_EXPRESSION,
};

typedef struct {
    LexerData data;
    int current_twig_block;
} TwigLexerData;

typedef struct {
    OptionValue secondary ALIGNED(sizeof(OptionValue));
} TwigLexerOption;

static named_element_t statement_keywords[] = {
    NE("as"),
    NE("autoescape"),
    NE("block"),
    NE("do"),
    NE("else"),
    NE("elseif"),
    NE("embed"),
    NE("endautoescape"),
    NE("endblock"),
    NE("endembed"),
    NE("endfilter"),
    NE("endfor"),
    NE("endif"),
    NE("endmacro"),
    NE("endsandbox"),
    NE("endset"),
    NE("endspaceless"),
    NE("endverbatim"),
    NE("extends"),
    NE("filter"),
    NE("flush"),
    NE("for"),
    NE("from"),
    NE("if"),
    NE("import"),
    NE("include"),
    NE("macro"),
    NE("sandbox"),
    NE("set"),
    NE("spaceless"),
    NE("use"),
    NE("verbatim"),
    NE("with"),
};

#if 0 /* UNUSED */
static named_element_t global_variables[] = {
    NE("_charset"),
    NE("_context"),
    NE("_self"),
};
#endif

enum {
    STATE(INITIAL),
    STATE(IN_TWIG),
    STATE(IN_DOUBLE_QUOTED_STRING),
    STATE(IN_SINGLE_QUOTED_STRING),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_TWIG) ] = IGNORABLE,
    [ STATE(IN_DOUBLE_QUOTED_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_SINGLE_QUOTED_STRING) ] = STRING_SINGLE,
};

static void twiginit(const OptionValue *options, LexerData *UNUSED(data), void *ctxt)
{
    Lexer *secondary;
    const TwigLexerOption *myoptions;

    myoptions = (const TwigLexerOption *) options;
    secondary = LEXER_UNWRAP(myoptions->secondary);
    if (NULL != secondary) {
        append_lexer(ctxt, secondary);
    }
}

static int yylex(YYLEX_ARGS)
{
    TwigLexerData *mydata;
    const TwigLexerOption *myoptions;

    (void) ctxt;
    mydata = (TwigLexerData *) data;
    myoptions = (const TwigLexerOption *) options;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    } else {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

<INITIAL> "{{" "-"? {
    mydata->current_twig_block = TWIG_EXPRESSION;
    BEGIN(IN_TWIG);
    //PUSH_STATE(IN_TWIG);
    TOKEN(NAME_TAG);
}

<INITIAL> "{%" "-"? {
    mydata->current_twig_block = TWIG_STATEMENT;
    BEGIN(IN_TWIG);
    //PUSH_STATE(IN_TWIG);
    TOKEN(NAME_TAG);
}

<INITIAL> "{#" {
    YYCTYPE *end;

    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "#}", STR_LEN("#}"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + STR_LEN("#}");
    }
    TOKEN(COMMENT_MULTILINE);
}

<IN_TWIG> "-"? "}}" {
    if (TWIG_EXPRESSION == mydata->current_twig_block) {
        BEGIN(INITIAL);
        //POP_STATE();
        TOKEN(NAME_TAG);
    } else {
        yyless(1);
        if ('-' == *YYTEXT) {
            TOKEN(OPERATOR);
        } else {
            TOKEN(PUNCTUATION);
        }
    }
}

<IN_TWIG> "-"? "%}" {
    if (TWIG_STATEMENT == mydata->current_twig_block) {
        BEGIN(INITIAL);
        //POP_STATE();
        TOKEN(NAME_TAG);
    } else {
        yyless(1);
        if ('-' == *YYTEXT) {
            TOKEN(OPERATOR);
        } else {
            TOKEN(PUNCTUATION);
        }
    }
}

<IN_TWIG> "{" {
    PUSH_STATE(IN_TWIG);
    TOKEN(PUNCTUATION);
}

<IN_TWIG> "}" {
    POP_STATE();
    TOKEN(PUNCTUATION);
}

<IN_TWIG> [0-9]+ "." [0-9]+ {
    TOKEN(NUMBER_FLOAT);
}

<IN_TWIG> [0-9]+ {
    TOKEN(NUMBER_DECIMAL);
}

<IN_TWIG> "'" {
    BEGIN(IN_SINGLE_QUOTED_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_TWIG> '"' {
    BEGIN(IN_DOUBLE_QUOTED_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_QUOTED_STRING> "#{" {
    PUSH_STATE(IN_TWIG);
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_DOUBLE_QUOTED_STRING> "}" {
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_DOUBLE_QUOTED_STRING> '\\"' {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_QUOTED_STRING> "\\'" {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_QUOTED_STRING> "'" {
    BEGIN(IN_TWIG);
    TOKEN(STRING_SINGLE);
}

<IN_DOUBLE_QUOTED_STRING> '"' {
    BEGIN(IN_TWIG);
    TOKEN(STRING_DOUBLE);
}

<IN_TWIG> [.()[\]] {
    TOKEN(PUNCTUATION);
}

<IN_TWIG> "true" | "false" | "null" | "none" {
    TOKEN(KEYWORD_CONSTANT);
}

<IN_TWIG> "is" | "not" | "in" | "and" | "or" | ("b-" ("and" | "or" | "xor")) | "matches" | ("starts" | "ends") [ \t]+ "with" | [.?/*]{2} | [-+%|?:~] | [!=<>]? "=" {
    TOKEN(OPERATOR);
}

<IN_TWIG> [a-zA-Z_][a-zA-Z_0-9]* {
    named_element_t key = { (char *) YYTEXT, YYLENG };

    if (TWIG_STATEMENT == mydata->current_twig_block && NULL != bsearch(&key, statement_keywords, ARRAY_SIZE(statement_keywords), sizeof(statement_keywords[0]), named_elements_cmp)) {
        TOKEN(KEYWORD);
    } else {
        TOKEN(IGNORABLE);
    }
}

<INITIAL> [^] {
    while (1) {
        YYCTYPE *ptr;

        if (NULL == (ptr = memchr(YYCURSOR, '{', YYLIMIT - YYCURSOR))) {
            YYCURSOR = YYLIMIT;
            DONE();
        } else {
            YYCURSOR = ptr;
            if (YYCURSOR >= YYLIMIT) {
                DONE();
            } else if ('{' == YYCURSOR[1] || '%' == YYCURSOR[1] || '#' == YYCURSOR[1]) {
                break;
            }
            ++YYCURSOR;
        }
    }
    DELEGATE_UNTIL(IGNORABLE);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
}

LexerImplementation twig_lexer = {
    "Twig",
    "Twig template engine",
    NULL, // aliases
    (const char * const []) { "*.twig", NULL },
    (const char * const []) { "application/x-twig", "text/html+twig", NULL },
    NULL, // interpreters
    NULL, // analyse
    twiginit,
    yylex,
    NULL, // finalize
    sizeof(TwigLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("secondary"), OPT_TYPE_LEXER, offsetof(TwigLexerOption, secondary), OPT_DEF_LEXER, "Lexer to highlight content outside of Twig blocks (if none, these parts will not be highlighted)" },
        END_OF_OPTIONS
    },
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
