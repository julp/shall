#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "utils.h"

enum {
    STATE(INITIAL),
    STATE(IN_RUBY),
    STATE(IN_COMMENT),
    STATE(IN_ERB_COMMENT),
    STATE(IN_PERCENT_STRING)
};

#define STR_FUNC_ESCAPE 0x01
#define STR_FUNC_EXPAND 0x02
#define STR_FUNC_REGEXP 0x04
#define STR_FUNC_QWORDS 0x08
#define STR_FUNC_SYMBOL 0x10
#define STR_FUNC_INDENT 0x20

enum string_type {
    str_squote/* = (0)*/,
    str_dquote/* = (STR_FUNC_EXPAND)*/,
    str_xquote/* = (STR_FUNC_EXPAND)*/,
    str_regexp/* = (STR_FUNC_REGEXP|STR_FUNC_ESCAPE|STR_FUNC_EXPAND)*/,
    str_sword/*  = (STR_FUNC_QWORDS)*/,
    str_dword/*  = (STR_FUNC_QWORDS|STR_FUNC_EXPAND)*/,
    str_ssym/*   = (STR_FUNC_SYMBOL)*/,
    str_dsym/*   = (STR_FUNC_SYMBOL|STR_FUNC_EXPAND)*/
};

static struct {
    int type;
    unsigned int flags;
} string_map[] = {
    [ str_squote ] = { STRING_SINGLE,   0 },
    [ str_dquote ] = { STRING_DOUBLE,   STR_FUNC_EXPAND },
    [ str_xquote ] = { STRING_BACKTICK, STR_FUNC_EXPAND },
    [ str_regexp ] = { STRING_REGEX,    STR_FUNC_REGEXP | STR_FUNC_ESCAPE | STR_FUNC_EXPAND },
    [ str_sword ]  = { STRING_SINGLE,   STR_FUNC_QWORDS },
    [ str_dword ]  = { STRING_DOUBLE,   STR_FUNC_QWORDS | STR_FUNC_EXPAND },
    [ str_ssym ]   = { STRING_INTERNED, STR_FUNC_SYMBOL },
    [ str_dsym ]   = { STRING_INTERNED, STR_FUNC_SYMBOL | STR_FUNC_EXPAND },
};

typedef struct {
    LexerData data;
    bool erb;
    int quote_char;
    enum string_type string_type;
} RubyLexerData;

typedef struct {
    OptionValue secondary ALIGNED(sizeof(OptionValue));
} RubyLexerOption;

static void rubyinit(const OptionValue *UNUSED(options), LexerData *data, void *UNUSED(ctxt))
{
    BEGIN(IN_RUBY);
}

static void erbinit(const OptionValue *options, LexerData *data, void *ctxt)
{
    Lexer *secondary;
    RubyLexerData *mydata;
    RubyLexerOption *myoptions;

    BEGIN(INITIAL);
    mydata = (RubyLexerData *) data;
    myoptions = (RubyLexerOption *) options;
    mydata->erb = true;
    secondary = LEXER_UNWRAP(myoptions->secondary);
    if (NULL != secondary) {
        append_lexer(ctxt, secondary);
    }
}

#if 0
/*
    Séquences d'échappement (chaînes) : \u{...}, \uXXXX, \C-?, \M-?

    suffixes i (complexes) et r (rationnels) ?

    commentaires magiques : coding, encoding, warn_indent + warn_past_scope pour ripper

    % : par défaut c'est Q (équivalent à double quotes) si ce qui suit n'est pas ALNUM (%<c #{i} d> équivaut à "c #{i} d")
    (caractères autorisés pour début/fin de %X)
    Q = double quotes
    q = single quotes
    W = double words (interpolation des #{ ... })
    w = single words (pas d'interpolation)
    I = double words (interpolation des #{ ... } : exemple %I<foo #{i}> avec i = 'bar', ça donne [:foo, :bar])
    i = single words (pas d'interpolation)
    x = exécution/`
    r = regexp
    s = symbole

    un identifiant peut se terminer par [!?]?=?

    =begin/=end en début de ligne

    si on recontre __END__ on a fini (en faire un paramètre ? ça dépend si la suite est du ruby ou pas ...)

https://raw.githubusercontent.com/ruby/ruby/trunk/parse.y
*/
#endif
static int rubylex(YYLEX_ARGS)
{
    RubyLexerData *mydata;

    (void) ctxt;
    (void) options;
    mydata = (RubyLexerData *) data;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

// awkward, an identifier in MRI can be composed of any codepoint > 127
// is_identchar is defined as rb_enc_isalnum((unsigned char)(*(p)),(enc)) || (*(p)) == '_' || !ISASCII(*(p));
IDENTIFIER = [a-zA-Z0-9_\u0080-\U0010FFFF]+;
// as defined by ISSPACE/rb_isspace
SPACE = [\t\n\v\f\r ];

<IN_RUBY> '#' .* {
    TOKEN(COMMENT_SINGLE);
}

<IN_RUBY> "0" 'b' [01]+ ("_" [01]+)* {
    TOKEN(NUMBER_BINARY);
}

<IN_RUBY> ("0" 'o' | "0" "_"?) [0-7]+ ("_" [0-7]+)* {
    TOKEN(NUMBER_OCTAL);
}

<IN_RUBY> ("0" 'd' [0-9]+ | [1-9] [0-9]*) ("_" [0-9]+)* {
    TOKEN(NUMBER_DECIMAL);
}

<IN_RUBY> "0" 'x' [0-9a-fA-F]+ ("_" [0-9a-fA-F]+)* {
    TOKEN(NUMBER_HEXADECIMAL);
}

<IN_RUBY> "$-" [0adFiIlpvw] {
    TOKEN(NAME_VARIABLE_GLOBAL);
}

<IN_RUBY> "$" [0-9_~*$?!@/\\;,.=:<>"&`'+-] {
    TOKEN(NAME_VARIABLE_GLOBAL);
}

<IN_RUBY> "@"{1,2} IDENTIFIER {
    if ('@' == YYTEXT[1]) {
        TOKEN(NAME_VARIABLE_CLASS);
    } else {
        TOKEN(NAME_VARIABLE_INSTANCE);
    }
}

<IN_RUBY> "::" {
    TOKEN(OPERATOR); // TODO
}

<IN_RUBY> "=>" {
    TOKEN(OPERATOR); // TODO
}

<IN_RUBY> "->" {
    TOKEN(OPERATOR); // TODO
}

<IN_RUBY> "**" | "<=>" | [<>!=]"=" | "&&" | "||" | [=!]"~" | ".." | "..." | ">>" | "<<" {
    // ** n'est pas l'opérateur de puissance mais de "splat" sur les arguments d'une méthode
    TOKEN(OPERATOR);
}

<IN_RUBY> [?:=!~&|^<>*+/-] {
    TOKEN(OPERATOR);
}

<IN_RUBY> "self" {
    TOKEN(NAME_BUILTIN_PSEUDO);
}

<IN_RUBY> "BEGIN" | "END" | "alias" | "begin" | "break" | "case" | "class" | "def" | "defined?" | "do" | "else" | "elsif" | "end" | "ensure" | "for" | "if" | "in" | "module" | "next" | "redo" | "rescue" | "retry" | "return" | "super" | "then" | "undef" | "unless" | "until" | "when" | "while" | "yield" {
    TOKEN(KEYWORD);
}

<IN_RUBY> "__ENCODING__" | "__FILE__" | "__LINE__" {
    TOKEN(NAME_BUILTIN_PSEUDO);
}

<IN_RUBY> "false" | "true" | "nil" | "FALSE" | "TRUE" | "NIL" {
    TOKEN(KEYWORD_BUILTIN);
}

<IN_RUBY> "require" {
    TOKEN(NAME_BUILTIN_PSEUDO);
}

<IN_RUBY> "and" | "or" | "not" {
    TOKEN(OPERATOR);
}

<IN_RUBY> "%" [qQwWiIxrs]? [^a-zA-Z0-9] {
    // TODO: x et r
    // TODO: options de regexp après le délimiteur de fin (eg %r{o}iu)
    // TODO: wWiI accepte des espaces après ? (mais on s'en fout nous ?)
    YYCTYPE q;

    q = YYTEXT[2];
    switch (YYTEXT[1]) {
        case 'i':
        case 'w':
            mydata->string_type = str_sword;
            break;
        case 'I':
        case 'W':
            mydata->string_type = str_dword;
            break;
        case 's':
            mydata->string_type = str_ssym;
            break;
        case 'q':
            mydata->string_type = str_squote;
            break;
        case '>':
            if(mydata->erb) {
                yyless(STR_LEN("%>"));
                BEGIN(INITIAL);
                TOKEN(NAME_TAG);
            }
        default:
            q = YYTEXT[1];
            /* no break */
        case 'Q':
            mydata->string_type = str_dquote;
            break;
    }
    switch (q) {
        case '[':
            mydata->quote_char = ']';
            break;
        case '(':
            mydata->quote_char = ')';
            break;
        case '<':
            mydata->quote_char = '>';
            break;
        case '{':
            mydata->quote_char = '}';
            break;
        default:
            mydata->quote_char = q;
            break;
    }
    BEGIN(IN_PERCENT_STRING);
    TOKEN(string_map[mydata->string_type].type);
}

<IN_RUBY> [/] {
    // /.*/[mixounse]*
    BEGIN(IN_PERCENT_STRING);
    mydata->quote_char = *YYTEXT;
    mydata->string_type = str_regexp;
    TOKEN(string_map[mydata->string_type].type);
}

<IN_RUBY> ["] {
    mydata->quote_char = '"';
    mydata->string_type = str_dquote;
    BEGIN(IN_PERCENT_STRING);
    TOKEN(string_map[mydata->string_type].type);
}

<IN_RUBY> ['] {
    mydata->quote_char = '\'';
    mydata->string_type = str_squote;
    BEGIN(IN_PERCENT_STRING);
    TOKEN(string_map[mydata->string_type].type);
}

<IN_PERCENT_STRING> [^] {
    // TODO: s'assurer que le quote_char n'est pas échappé
    if (mydata->quote_char == *YYTEXT) {
        BEGIN(IN_RUBY);
    }
    TOKEN(string_map[mydata->string_type].type);
}

<IN_RUBY> [;.{}[\]] {
    TOKEN(PUNCTUATION);
}

<IN_RUBY> "=begin" SPACE {
    if (!IS_BOL) {
        yyless(0);
    } else {
//         yyless(YYLENG - 1);
        BEGIN(IN_COMMENT);
        TOKEN(COMMENT_MULTILINE);
    }
}

<IN_COMMENT> "=end" SPACE {
    if (!IS_BOL) {
        yyless(0);
    } else {
//         yyless(YYLENG - 1);
        BEGIN(IN_RUBY);
        TOKEN(COMMENT_MULTILINE);
    }
}

<IN_COMMENT> [^] {
    TOKEN(COMMENT_MULTILINE);
}

<IN_RUBY> [^] {
    TOKEN(IGNORABLE);
}

// TODO: pas plus d'un ':' consécutif sinon c'est :: pour la résolution de portée
<IN_RUBY> IDENTIFIER ':' {
    if (':' == *YYCURSOR) {
        yyless((YYCURSOR - YYTEXT) - 1);
        if (*YYTEXT >= 'A' && *YYTEXT <= 'Z') {
            TOKEN(NAME_CLASS);
        } else {
            TOKEN(NAME_VARIABLE);
        }
    } else {
        TOKEN(STRING_INTERNED);
    }
}

<IN_RUBY> ':' IDENTIFIER {
    TOKEN(STRING_INTERNED);
}

// un identifiant peut encore se terminer par '=' ?
<IN_RUBY> IDENTIFIER [!?]? {
    // TODO:
    // - si la première lettre est une majuscule, c'est une constante
    // - si finit par '!' ou '?' c'est un id de méthode ?
    if (*YYTEXT >= 'A' && *YYTEXT <= 'Z') {
        TOKEN(NAME_VARIABLE_CLASS);
    } else {
        TOKEN(NAME_FUNCTION);
    }
}

<INITIAL> "<%#" {
    BEGIN(IN_ERB_COMMENT);
    TOKEN(COMMENT_SINGLE);/*NAME_TAG*/
}

<IN_ERB_COMMENT> "%>" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_SINGLE);/*NAME_TAG*/
}

<IN_ERB_COMMENT> [^] {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "<%" "="? {
    BEGIN(IN_RUBY);
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
*/
    }
    DONE();
}

LexerImplementation ruby_lexer = {
    "Ruby",
    "For Ruby source code",
    NULL,
    (const char * const []) { "*.rb", NULL },
    (const char * const []) { "text/x-ruby", "application/x-ruby", NULL },
    (const char * const []) { "ruby", "ruby[12]*", NULL },
    NULL, // analyze
    rubyinit,
    rubylex,
    NULL, // finalize
    sizeof(RubyLexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};

LexerImplementation erb_lexer = {
    "ERB",
    "For ERB (Ruby) templates. Use the \"secondary\" option to delegate tokenization of parts which are outside of ERB tags.",
    NULL,
    (const char * const []) { "*.erb", NULL },
    (const char * const []) { "application/x-ruby-templating", NULL },
    NULL, // interpreters
    NULL, // analyze
    erbinit,
    rubylex,
    NULL, // finalize
    sizeof(RubyLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("secondary"), OPT_TYPE_LEXER, offsetof(RubyLexerOption, secondary), OPT_DEF_LEXER, "Lexer to highlight content outside of ERB tags (if none, these parts will not be highlighted)" },
        END_OF_OPTIONS
    },
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
