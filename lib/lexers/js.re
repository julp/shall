#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
    STATE(IN_REGEXP),
    STATE(IN_COMMENT),
    STATE(IN_STRING_SINGLE),
    STATE(IN_STRING_DOUBLE),
};

static int default_token_type[] = {
    IGNORABLE, // INITIAL
    STRING_REGEX, // IN_REGEXP
    COMMENT_MULTILINE, // IN_COMMENT
    STRING_DOUBLE, // IN_STRING_SINGLE
    STRING_DOUBLE, // IN_STRING_DOUBLE
};

static int jslex(YYLEX_ARGS) {
    (void) ctxt;
    (void) data;
    (void) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

// http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf

WHITESPACE = [\t\x0b\x0c \u00A0\uFEFF]; // § 7.2 (TODO: any codepoint categorized as Zs)

// 1680          ; White_Space # Zs       OGHAM SPACE MARK
// 2000..200A    ; White_Space # Zs  [11] EN QUAD..HAIR SPACE
// 202F          ; White_Space # Zs       NARROW NO-BREAK SPACE
// 205F          ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE
// 3000          ; White_Space # Zs       IDEOGRAPHIC SPACE

LINE_TERMINATORS = "\r\n" | [\r\n\u2028\u2029]; // § 7.3

HEXDIGIT = [0-9a-fA-F];
UNICODE_ESCAPE_SEQUENCE = "\\u" HEXDIGIT{4};

IDENTIFIER_NAME = ([$_a-zA-Z] | UNICODE_ESCAPE_SEQUENCE)([$_a-zA-Z0-9] | UNICODE_ESCAPE_SEQUENCE)*; // § 7.6 (TODO: \p{Lu}\p{Ll}\p{Lt}\p{Lm}\p{Lo}\p{Nl}\p{Mn}\p{Mc}\p{Nd}\p{Nc} but not as first character

PUNCTUATORS = [{}()[\].;,]; // part of § 7.7

// § 7.8.3
EXPONENT_PART = ('e' [+-]? [0-9]+);
DECIMAL_LITERAL = ("0" | [1-9][0-9]*) ("." [0-9]*)? EXPONENT_PART? | "." [0-9]+ EXPONENT_PART?;
HEXADECIMAL_LITERAL = ('0x' HEXDIGIT+)+;

<INITIAL>"//" [^\r\n\u2028\u2029]* {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "/*" {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT> "*/" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

// part of § 7.7
<INITIAL> [!=] "==" | [-+*/%&|^!<>=] "="? | [~?:] | [-+&|]{2} | [<>]{2} "="? | ">"{3} "="? {
    TOKEN(OPERATOR);
}

// § 7.8.[12]
<INITIAL> "null" | "true" | "false" {
    TOKEN(KEYWORD_CONSTANT);
}

// reserved words § 7.6.1.1
<INITIAL> "break" | "do" | "instanceof" | "typeof" | "case" | "else" | "new" | "var" | "catch" | "finally" | "return" | "void" | "continue" | "for" | "switch" | "while" | "debugger" | "function" | "this" | "with" | "default" | "if" | "throw" | "delete" | "in" | "try" {
    TOKEN(KEYWORD);
}

// future reserved words § 7.6.1.2
<INITIAL> "class" | "enum" | "extends" | "super" | "const" | "export" | "import" {
    TOKEN(KEYWORD);
}

// future reserved words § 7.6.1.2 (same) but only in strict mode
<INITIAL> "implements" | "let" | "private" | "public" | "yield" | "interface" | "package" | "protected" | "static" {
    TOKEN(KEYWORD);
}

<INITIAL> IDENTIFIER_NAME {
    // TODO
    TOKEN(NAME_VARIABLE);
}

// § 7.8.4
<INITIAL> "'" {
    BEGIN(IN_STRING_SINGLE);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> '"' {
    BEGIN(IN_STRING_DOUBLE);
    TOKEN(STRING_DOUBLE);
}

// TODO: line continuation
<IN_STRING_SINGLE> "'" {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<IN_STRING_DOUBLE> '"' {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<IN_STRING_SINGLE,IN_STRING_DOUBLE> "\\" ([\\'"bfnrtv] | "x" HEXDIGIT{2} | "u" HEXDIGIT{4}) {
    TOKEN(ESCAPED_CHAR);
}

// § 7.8.5
<INITIAL> "/" {
    BEGIN(IN_REGEXP);
    TOKEN(STRING_REGEX);
}

<IN_REGEXP> "\\" . {
    TOKEN(ESCAPED_CHAR);
}

// flags are defined by § 15.10.4.1
<IN_REGEXP> "/" [gim]* {
    BEGIN(INITIAL);
    TOKEN(STRING_REGEX);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation js_lexer = {
    "Javascript",
    "TODO",
    (const char * const []) { "JS", NULL },
    (const char * const []) { "*.js", NULL },
    (const char * const []) { "text/javascript", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    jslex,
    NULL, // finalyze
    sizeof(LexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
