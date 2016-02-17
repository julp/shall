/**
 * Spec/language reference:
 * - https://golang.org/ref/spec
 */

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
};

// typedef struct {
//     LexerData data;
// } GoLexerData;

static int yylex(YYLEX_ARGS)
{
//     GoLexerData *mydata;

    (void) ctxt;
    (void) options;
//     mydata = (GoLexerData *) data;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    } else {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

newline = [\u000A];
unicode_char = [^\u000A];
unicode_letter = [a-zA-Z]; // TODO: [\p{Lu}\p{Ll}\p{Lt}\p{Lm}\p{Lo}]
unicode_digit = [0-9]; // TODO: \p{Nd}

letter = unicode_letter | "_";
decimal_digit = [0-9];
octal_digit = [0-7];
hex_digit = [0-9a-fA-F];

white_space = [\u0020\u0009\u000D\u000A];

identifier = letter ( letter | unicode_digit )*;

exponent  = 'e' [-+] decimal_digit+;
float_lit = decimal_digit+ "." decimal_digit* exponent? | decimal_digit+ exponent | "." decimal_digit+ exponent?;

little_u_value = "\\u" hex_digit{4};
big_u_value = "\\U" hex_digit{8};
escaped_char = "\\" [abfnrtv\\'"];
unicode_value = unicode_char | little_u_value | big_u_value | escaped_char;
octal_byte_value = "\\" octal_digit{3};
hex_byte_value = "\\x" hex_digit{2};
byte_value = octal_byte_value | hex_byte_value;
rune_lit = "'" ( unicode_value | byte_value ) "'";

raw_string_lit = "`" (unicode_char | newline)* "`";
interpreted_string_lit = '"' (unicode_value | byte_value)* '"';
string_lit = raw_string_lit | interpreted_string_lit;

imaginary_lit = (decimal_digit+ | float_lit) "i";

<INITIAL> "//" unicode_char* {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "/*" {
    YYCTYPE *end;

    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "*/", STR_LEN("*/"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + STR_LEN("*/");
    }
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> "break" | "default" | "func" | "select" | "case" | "defer" | "go" | "else" | "goto" | "package" | "switch" | "const" | "fallthrough" | "if" | "range" | "type" | "continue" | "for" | "import" | "return" | "var" {
    TOKEN(KEYWORD);
}

<INITIAL> "struct" | "interface" | "map" | "chan" {
    TOKEN(KEYWORD_TYPE);
}

<INITIAL> [-+/*^!=|:%]"="? | [(){}[\].,;] | [-+|&]{2} | ";"{3} | [<>]{1,2}"="? | "<-" | "&" "^"? "="? {
    TOKEN(OPERATOR); // or delimiters, and other special tokens
}

<INITIAL> [1-9] decimal_digit* {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> "0" octal_digit* {
    TOKEN(NUMBER_OCTAL);
}

<INITIAL> '0x' hex_digit+ {
    TOKEN(NUMBER_HEXADECIMAL);
}

<INITIAL> float_lit {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> imaginary_lit {
    TOKEN(NUMBER_IMAGINARY);
}

<INITIAL> rune_lit {
    TOKEN(STRING_SINGLE); // TODO: char
}

<INITIAL> string_lit {
    TOKEN(STRING_SINGLE);
}

<INITIAL> "true" | "false" | "iota" | "nil" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL> "bool" | "byte" | "rune" | "u"? "int" ("8" | "16" | "32" | "64" | "ptr")? | "error" | "float" ("32" | "64") | "complex" ("64" | "128") | "string" {
    TOKEN(KEYWORD_TYPE);
}

<INITIAL> identifier {
    TOKEN(NAME);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
}

LexerImplementation go_lexer = {
    "Go",
    "For the Google Go programming language (golang.org)",
    NULL, // aliases
    (const char * const []) { "*.go", NULL },
    (const char * const []) { "text/x-gosrc", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    yylex,
    NULL, // finalize
    sizeof(/*Go*/LexerData),
    NULL, // options
    NULL // dependencies
};
