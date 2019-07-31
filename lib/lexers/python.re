/**
 * Spec/language reference:
 * - https://docs.python.org/3/reference/index.html
 * - https://docs.python.org/3/reference/lexical_analysis.html
 */

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
    STATE(SHORT_SINGLE_QUOTES),
    STATE(LONG_SINGLE_QUOTES),
    STATE(SHORT_DOUBLE_QUOTES),
    STATE(LONG_DOUBLE_QUOTES),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(SHORT_SINGLE_QUOTES) ] = STRING_DOUBLE,
    [ STATE(SHORT_DOUBLE_QUOTES) ] = STRING_DOUBLE,
    [ STATE(LONG_SINGLE_QUOTES) ] = STRING_DOUBLE,
    [ STATE(LONG_DOUBLE_QUOTES) ] = STRING_DOUBLE,
};

enum {
    BYTE_STRING = (1<<0),
    RAW_STRING = (1<<1),
    UNICODE_STRING = (1<<2),
};

typedef struct {
    LexerData data;
    int string_type;
} PythonLexerData;

static int yylex(YYLEX_ARGS)
{
    PythonLexerData *mydata;

    (void) ctxt;
    (void) options;
    mydata = (PythonLexerData *) data;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    } else {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

DIGIT = [0-9];
XDIGIT = [0-9a-fA-F];

decimalinteger = [1-9] DIGIT* | "0"+;
octinteger = "0" 'o' [0-7]+;
hexinteger = "0" 'x' XDIGIT+;
bininteger = "0" 'b' [01]+;

pointfloat = DIGIT* "." DIGIT+ | DIGIT+ ".";
floatnumber = pointfloat | (DIGIT+ | pointfloat) 'e' [+-]? DIGIT+;
imagnumber = (floatnumber | DIGIT+) 'j';

<INITIAL> "False" | "True" | "None" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL> "is" | "in" | "or" | "and" | "not" {
    TOKEN(OPERATOR);
}

<INITIAL> "finally" | "try" | "except" | "raise" {
    TOKEN(KEYWORD);
}

<INITIAL> "for" | "while" | "elif" | "if" | "else" {
    TOKEN(KEYWORD);
}

<INITIAL> "return" | "continue" | "pass" | "break" {
    TOKEN(KEYWORD);
}

<INITIAL> "class" | "lambda" | "def" | "from" | "nonlocal" | "del" | "global" | "with" | "as" | "yield" | "assert" | "import" {
    TOKEN(KEYWORD);
}

// TODO: support Unicode
<INITIAL> [a-zA-Z_] [a-zA-Z_0-9]* {
    TOKEN(NAME_VARIABLE);
}

<INITIAL> ('b' 'r'? | 'r' 'b'? | 'u')? ("'" "''"? | '"' '""'?) {
    int type;
    const YYCTYPE *quote;

    type = STRING_DOUBLE;
    mydata->string_type = 0;
    for (quote = YYTEXT; '"' != *quote && '\'' != *quote; quote++) {
        switch (*quote) {
            case 'b':
            case 'B':
                mydata->string_type |= BYTE_STRING;
                break;
            case 'r':
            case 'R':
                type = STRING_SINGLE;
                mydata->string_type |= RAW_STRING;
                break;
            case 'u':
            case 'U':
                mydata->string_type |= UNICODE_STRING;
                break;
        }
    }
    if (YYCURSOR - quote >= 3 && *quote == quote[1] && *quote == quote[2]) {
        YYSETCONDITION('"' == *quote ? STATE(LONG_DOUBLE_QUOTES) : STATE(LONG_SINGLE_QUOTES));
    } else {
        YYSETCONDITION('"' == *quote ? STATE(SHORT_DOUBLE_QUOTES) : STATE(SHORT_SINGLE_QUOTES));
    }
    TOKEN(type);
}

<INITIAL> [-+@%&|^~] | [<>!=]"=" | [*<>/]{1,2} {
    TOKEN(OPERATOR);
}

<INITIAL> [.,:;{}()[\]] {
    TOKEN(PUNCTUATION);
}

<INITIAL> "#" {
    while (YYCURSOR < YYLIMIT && !IS_NL(*YYCURSOR)) {
        ++YYCURSOR;
    }
    HANDLE_CR_LF;
    TOKEN(COMMENT_SINGLE);
}

<SHORT_SINGLE_QUOTES,SHORT_DOUBLE_QUOTES,LONG_SINGLE_QUOTES,LONG_DOUBLE_QUOTES> [\\] (['"abfnrtv] | "o" [0-7]{1,3} | "x" XDIGIT{2}) {
    // TODO: ' vs " suivant le délimiteur
    if (HAS_FLAG(mydata->string_type, RAW_STRING)) {
        TOKEN(STRING_SINGLE)
    } else {
        TOKEN(SEQUENCE_ESCAPED);
    }
}

// \x7D = '}', re2c fails on it
<SHORT_SINGLE_QUOTES,SHORT_DOUBLE_QUOTES,LONG_SINGLE_QUOTES,LONG_DOUBLE_QUOTES> [\\] ("N{" [^}]* "}" | "u" XDIGIT{4} | "U" XDIGIT{8}) {
    int type;

    if (HAS_FLAG(mydata->string_type, RAW_STRING)) {
        if (HAS_FLAG(mydata->string_type, BYTE_STRING)) {
            type = STRING_DOUBLE;
        } else {
            type = STRING_SINGLE;
        }
    } else {
        type = SEQUENCE_ESCAPED;
    }
    TOKEN(type);
}

<SHORT_SINGLE_QUOTES> "'" {
    BEGIN(INITIAL);
    TOKEN(HAS_FLAG(mydata->string_type, RAW_STRING) ? STRING_DOUBLE : STRING_SINGLE);
}

<SHORT_DOUBLE_QUOTES> '"' {
    BEGIN(INITIAL);
    TOKEN(HAS_FLAG(mydata->string_type, RAW_STRING) ? STRING_DOUBLE : STRING_SINGLE);
}

<LONG_SINGLE_QUOTES> "'''" {
    BEGIN(INITIAL);
    TOKEN(HAS_FLAG(mydata->string_type, RAW_STRING) ? STRING_DOUBLE : STRING_SINGLE);
}

<LONG_DOUBLE_QUOTES> '"""' {
    BEGIN(INITIAL);
    TOKEN(HAS_FLAG(mydata->string_type, RAW_STRING) ? STRING_DOUBLE : STRING_SINGLE);
}

<SHORT_SINGLE_QUOTES,SHORT_DOUBLE_QUOTES> [\n] {
    TOKEN(ERROR);
}

<SHORT_SINGLE_QUOTES,SHORT_DOUBLE_QUOTES,LONG_SINGLE_QUOTES,LONG_DOUBLE_QUOTES> [^] {
    TOKEN(HAS_FLAG(mydata->string_type, RAW_STRING) ? STRING_DOUBLE : STRING_SINGLE);
}

<INITIAL> decimalinteger {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> octinteger {
    TOKEN(NUMBER_OCTAL);
}

<INITIAL> hexinteger {
    TOKEN(NUMBER_HEXADECIMAL);
}

<INITIAL> bininteger {
    TOKEN(NUMBER_BINARY);
}

<INITIAL> imagnumber {
    TOKEN(NUMBER_IMAGINARY);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
}

LexerImplementation python_lexer = {
    "Python",
    "For the Python programming language (python.org)",
    NULL, // aliases
    (const char * const []) { "*.py", NULL },
    (const char * const []) { "text/x-python", "application/x-python", NULL },
    (const char * const []) { "python", "python[23]", NULL },
    NULL, // analyse
    NULL, // init
    yylex,
    NULL, // finalize
    sizeof(PythonLexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
