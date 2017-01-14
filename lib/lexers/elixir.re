/**
 * Spec/language reference:
 * - https://raw.githubusercontent.com/elixir-lang/elixir/v1.4.0/lib/elixir/src/elixir.hrl
 * - https://raw.githubusercontent.com/elixir-lang/elixir/v1.4.0/lib/elixir/src/elixir_parser.yrl
 */

#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
    STATE(IN_ELIXIR),
    STATE(IN_EEX_COMMENT),
    STATE(IN_SIGIL),
    STATE(IN_SINGLE_STRING),
    STATE(IN_HEREDOC_STRING),
    STATE(IN_SINGLE_CHARLIST),
    STATE(IN_HEREDOC_CHARLIST),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_SIGIL) ] = IGNORABLE,
    [ STATE(IN_SINGLE_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_HEREDOC_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_SINGLE_CHARLIST) ] = STRING_SINGLE,
    [ STATE(IN_HEREDOC_CHARLIST) ] = STRING_SINGLE,
};

typedef struct {
    LexerData data;
    bool eex;
    YYCTYPE quote_char;
} ElixirLexerData;

typedef struct {
    OptionValue secondary ALIGNED(sizeof(OptionValue));
} ElixirLexerOption;

static void elixirinit(const OptionValue *UNUSED(options), LexerData *data, void *UNUSED(ctxt))
{
    BEGIN(IN_ELIXIR);
}

static void eexinit(const OptionValue *options, LexerData *data, void *ctxt)
{
    Lexer *secondary;
    ElixirLexerData *mydata;
    ElixirLexerOption *myoptions;

    BEGIN(INITIAL);
    mydata = (ElixirLexerData *) data;
    myoptions = (ElixirLexerOption *) options;
    mydata->eex = true;
    secondary = LEXER_UNWRAP(myoptions->secondary);
    if (NULL != secondary) {
        append_lexer(ctxt, secondary);
    }
}

#if 0
TODO:
* def(p), defmacro, defstruct, defmodule, ...
* modules attributes ("@" identifier)
* spec/types ?
* magic constants
* interpolation (#{...})
* bit strings (<< ... >>)
* recognize base modules? (Kernel, Enum, List)
#endif
static int yylex(YYLEX_ARGS)
{
    ElixirLexerData *mydata;

    (void) ctxt;
    (void) options;
    mydata = (ElixirLexerData *) data;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    } else {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

alpha = [a-zA-Z];
sigilseparator = [/<'"\[{(|];
identifier_start = [a-zA-Z_];
atom = identifier_start ([0-9@] | identifier_start)*;
identifier = identifier_start ([0-9] | identifier_start)*;

// TODO: handle modifier(s?) after the sigil character terminator
<IN_ELIXIR> "~" alpha sigilseparator {
    YYCTYPE terminator;

    switch (YYTEXT[2]) {
        case '<':
            terminator = '>';
            break;
        case '[':
            terminator = ']';
            break;
        case '{':
            terminator = '}';
            break;
        case '(':
            terminator = ')';
            break;
        default:
            terminator = YYTEXT[2];
            break;
    }
#if 1
    BEGIN(IN_SIGIL);
    mydata->quote_char = terminator;
    TOKEN(KEYWORD);
#else
    YYCTYPE *ptr;

    if (NULL == (ptr = memchr(YYCURSOR, terminator, YYLIMIT - YYCURSOR))) {
        YYCURSOR = YYLIMIT;
        DONE();
    } else {
        YYCURSOR = ptr;
        TOKEN(STRING);
        ++YYCURSOR;
    }
#endif
}

<IN_SIGIL> [^] {
    if (mydata->quote_char == *YYTEXT) {
        BEGIN(IN_ELIXIR);
        TOKEN(KEYWORD);
    } else {
        TOKEN(IGNORABLE); // TODO
    }
}

<IN_ELIXIR> "0" 'b' [01]+ ("_" [01]+)* {
    TOKEN(NUMBER_BINARY);
}

<IN_ELIXIR> [0-9]+ {
    TOKEN(NUMBER_DECIMAL);
}

<IN_ELIXIR> [0-9]+ "." [0-9]+ ('e' [-+]? [0-9]+)? {
    TOKEN(NUMBER_FLOAT);
}

// inexact? First digit have to be [0;3]
<IN_ELIXIR> "0" 'o' | [0-7]+ ("_" [0-7]+)* {
    TOKEN(NUMBER_OCTAL);
}

<IN_ELIXIR> "0" 'x' [0-9a-fA-F]+ ("_" [0-9a-fA-F]+)* {
    TOKEN(NUMBER_HEXADECIMAL);
}

<IN_ELIXIR> "and" | "in" | "or" | "not" | "when" {
    TOKEN(OPERATOR);
}

<IN_ELIXIR> [!=]"="{1,2} | "|"{2,3} | "&"{2,3} | "^"{2,3} | [<>]"="? | [-+] {
    TOKEN(OPERATOR);
}

<IN_ELIXIR> "#" .* {
    TOKEN(COMMENT_SINGLE);
}

<IN_ELIXIR> ":" (atom | '"' [^"]+ '"' | "'" [^']+ "'") {
    TOKEN(STRING_INTERNED);
}

<IN_SINGLE_STRING,IN_SINGLE_CHARLIST> "\\" [abdefnrstv] {
    TOKEN(ESCAPED_CHAR);
}

<IN_ELIXIR> '"""' {
    BEGIN(IN_HEREDOC_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_HEREDOC_STRING> '"""' {
    BEGIN(IN_ELIXIR);
    TOKEN(STRING_DOUBLE);
}

<IN_ELIXIR> "'''" {
    BEGIN(IN_HEREDOC_CHARLIST);
    TOKEN(STRING_SINGLE);
}

<IN_HEREDOC_CHARLIST> "'''" {
    BEGIN(IN_ELIXIR);
    TOKEN(STRING_SINGLE);
}

<IN_ELIXIR> '"' {
    BEGIN(IN_SINGLE_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_SINGLE_STRING> '\\"' {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_STRING> '"' {
    BEGIN(IN_ELIXIR);
    TOKEN(STRING_DOUBLE);
}

<IN_ELIXIR> "'" {
    BEGIN(IN_SINGLE_CHARLIST);
    TOKEN(STRING_SINGLE);
}


<IN_SINGLE_CHARLIST> "\\'" {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_CHARLIST> "'" {
    BEGIN(IN_ELIXIR);
    TOKEN(STRING_SINGLE);
}

<IN_ELIXIR> [(){}[\],:] | "->" | "=>" | "%{" | "|>" {
    TOKEN(PUNCTUATION);
}

<IN_ELIXIR> "fn" | "do" | "end" | "after" | "else" | "rescue" | "if" | "unless" | "case" | "cond" | "for" | "with" | "use" | "import" | "alias" | "require" {
    TOKEN(KEYWORD);
}

// in fact these are atoms
<IN_ELIXIR> "nil" | "true" | "false" {
    TOKEN(KEYWORD_BUILTIN);
}

// def, defmodule, ... are macros
// TODO: magic constants like __MODULE__?
<IN_ELIXIR> identifier {
    TOKEN(IGNORABLE);
}

<IN_ELIXIR> "%>" {
    if (mydata->eex) {
        yyless(STR_LEN("%>"));
        BEGIN(INITIAL);
        TOKEN(NAME_TAG);
    }
}

<IN_ELIXIR> [^] {
    TOKEN(IGNORABLE);
}

<INITIAL> "<%#" {
    BEGIN(IN_EEX_COMMENT);
    TOKEN(COMMENT_SINGLE);/*NAME_TAG*/
}

<IN_EEX_COMMENT> "%>" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_SINGLE);/*NAME_TAG*/
}

<IN_EEX_COMMENT> [^] {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "<%" "="? {
    BEGIN(IN_ELIXIR);
    TOKEN(NAME_TAG);
}

<INITIAL> [^] {
    YYCTYPE *end;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    }
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "<%", STR_LEN("<%"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end;
    }
    DELEGATE_UNTIL(IGNORABLE);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
}

LexerImplementation elixir_lexer = {
    "Elixir",
    "For the Elixir programming language (elixir-lang.org)",
    NULL, // aliases
    (const char * const []) { "*.ex", "*.exs", NULL },
    (const char * const []) { "text/x-elixir", NULL },
    (const char * const []) { "elixir", "iex", NULL },
    NULL, // analyse
    elixirinit,
    yylex,
    NULL, // finalize
    sizeof(ElixirLexerData),
    NULL, // options
    NULL // dependencies
};

LexerImplementation eex_lexer = {
    "EEX",
    "For EEX (Elixir) templates. Use the \"secondary\" option to delegate tokenization of parts which are outside of EEX tags.",
    NULL,
    (const char * const []) { "*.eex", NULL },
    (const char * const []) { "application/x-elixir-templating", NULL },
    NULL, // interpreters
    NULL, // analyze
    eexinit,
    yylex,
    NULL, // finalize
    sizeof(ElixirLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("secondary"), OPT_TYPE_LEXER, offsetof(ElixirLexerOption, secondary), OPT_DEF_LEXER, "Lexer to highlight content outside of EEX tags (if none, these parts will not be highlighted)" },
        END_OF_OPTIONS
    },
    NULL // dependencies
};
