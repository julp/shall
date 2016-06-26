#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

typedef struct {
    LexerData data;
    YYCTYPE delim;
} MyLexerData;

typedef struct {
    int ansi_quotes ALIGNED(sizeof(OptionValue));
    int uppercase_keywords ALIGNED(sizeof(OptionValue));
    int no_backslash_escapes ALIGNED(sizeof(OptionValue));
} MyLexerOption;

enum {
    STATE(INITIAL),
    STATE(IN_STRING),
    STATE(IN_COMMENT),
    STATE(IN_IDENTIFIER),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_STRING) ] = STRING,
    [ STATE(IN_COMMENT) ] = COMMENT_MULTILINE,
    [ STATE(IN_IDENTIFIER) ] = NAME,
};

#define RESERVED(s) NE(s)
static const named_element_t keywords[] = {
    RESERVED("ACCESSIBLE"),
    RESERVED("ADD"),
    RESERVED("ALL"),
    RESERVED("ALTER"),
    RESERVED("ANALYZE"),
    RESERVED("AND"),
    RESERVED("AS"),
    RESERVED("ASC"),
    RESERVED("ASENSITIVE"),
    RESERVED("BEFORE"),
    RESERVED("BETWEEN"),
    RESERVED("BIGINT"),
    RESERVED("BINARY"),
    RESERVED("BLOB"),
    RESERVED("BOTH"),
    RESERVED("BY"),
    RESERVED("CALL"),
    RESERVED("CASCADE"),
    RESERVED("CASE"),
    RESERVED("CHANGE"),
    RESERVED("CHAR"),
    RESERVED("CHAR"),
    RESERVED("CHECK"),
    RESERVED("COLLATE"),
    RESERVED("COLUMN"),
    RESERVED("CONDITION"),
    RESERVED("CONSTRAINT"),
    RESERVED("CONTINUE"),
    RESERVED("CONVERT"),
    RESERVED("CREATE"),
    RESERVED("CROSS"),
    RESERVED("CURDATE"),
    RESERVED("CURTIME"),
    RESERVED("NOW"),
    RESERVED("CURRENT_USER"),
    RESERVED("CURSOR"),
    RESERVED("DATABASE"),
    RESERVED("DATABASES"),
    RESERVED("DAY_HOUR"),
    RESERVED("DAY_MICROSECOND"),
    RESERVED("DAY_MINUTE"),
    RESERVED("DAY_SECOND"),
    RESERVED("DECIMAL"),
    RESERVED("DECIMAL"),
    RESERVED("DECLARE"),
    RESERVED("DEFAULT"),
    RESERVED("DELAYED"),
    RESERVED("DELETE"),
    RESERVED("DESC"),
    RESERVED("DESCRIBE"),
    RESERVED("DETERMINISTIC"),
    RESERVED("DISTINCT"),
    RESERVED("DISTINCT"),
    RESERVED("DIV"),
    RESERVED("DOUBLE"),
    RESERVED("DROP"),
    RESERVED("DUAL"),
    RESERVED("EACH"),
    RESERVED("ELSE"),
    RESERVED("ELSEIF"),
    RESERVED("ENCLOSED"),
    RESERVED("ESCAPED"),
    RESERVED("EXISTS"),
    RESERVED("EXIT"),
    RESERVED("DESCRIBE"),
    RESERVED("FALSE"),
    RESERVED("FETCH"),
    RESERVED("FLOAT"),
    RESERVED("FLOAT"),
    RESERVED("DOUBLE"),
    RESERVED("FOR"),
    RESERVED("FORCE"),
    RESERVED("FOREIGN"),
    RESERVED("FROM"),
    RESERVED("FULLTEXT"),
    RESERVED("GET"),
    RESERVED("GRANT"),
    RESERVED("GROUP"),
    RESERVED("HAVING"),
    RESERVED("HIGH_PRIORITY"),
    RESERVED("HOUR_MICROSECOND"),
    RESERVED("HOUR_MINUTE"),
    RESERVED("HOUR_SECOND"),
    RESERVED("IF"),
    RESERVED("IGNORE"),
    RESERVED("IN"),
    RESERVED("INDEX"),
    RESERVED("INFILE"),
    RESERVED("INNER"),
    RESERVED("INOUT"),
    RESERVED("INSENSITIVE"),
    RESERVED("INSERT"),
    RESERVED("INT"),
    RESERVED("TINYINT"),
    RESERVED("SMALLINT"),
    RESERVED("MEDIUMINT"),
    RESERVED("INT"),
    RESERVED("BIGINT"),
    RESERVED("INT"),
    RESERVED("INTERVAL"),
    RESERVED("INTO"),
    RESERVED("IO_AFTER_GTIDS"),
    RESERVED("IO_BEFORE_GTIDS"),
    RESERVED("IS"),
    RESERVED("ITERATE"),
    RESERVED("JOIN"),
    RESERVED("KEY"),
    RESERVED("KEYS"),
    RESERVED("KILL"),
    RESERVED("LEADING"),
    RESERVED("LEAVE"),
    RESERVED("LEFT"),
    RESERVED("LIKE"),
    RESERVED("LIMIT"),
    RESERVED("LINEAR"),
    RESERVED("LINES"),
    RESERVED("LOAD"),
    RESERVED("NOW"),
    RESERVED("NOW"),
    RESERVED("LOCK"),
    RESERVED("LONG"),
    RESERVED("LONGBLOB"),
    RESERVED("LONGTEXT"),
    RESERVED("LOOP"),
    RESERVED("LOW_PRIORITY"),
    RESERVED("MASTER_BIND"),
    RESERVED("MASTER_SSL_VERIFY_SERVER_CERT"),
    RESERVED("MATCH"),
    RESERVED("MAX_VALUE"),
    RESERVED("MEDIUMBLOB"),
    RESERVED("MEDIUMINT"),
    RESERVED("MEDIUMTEXT"),
    RESERVED("MEDIUMINT"),
    RESERVED("MINUTE_MICROSECOND"),
    RESERVED("MINUTE_SECOND"),
    RESERVED("MOD"),
    RESERVED("MODIFIES"),
    RESERVED("NATURAL"),
    RESERVED("NOT"),
    RESERVED("NO_WRITE_TO_BINLOG"),
    RESERVED("NULL"),
    RESERVED("NUMERIC"),
    RESERVED("ON"),
    RESERVED("OPTIMIZE"),
    RESERVED("OPTION"),
    RESERVED("OPTIONALLY"),
    RESERVED("OR"),
    RESERVED("ORDER"),
    RESERVED("OUT"),
    RESERVED("OUTER"),
    RESERVED("OUTFILE"),
    RESERVED("PARTITION"),
    RESERVED("PRECISION"),
    RESERVED("PRIMARY"),
    RESERVED("PROCEDURE"),
    RESERVED("PURGE"),
    RESERVED("RANGE"),
    RESERVED("READ"),
    RESERVED("READ_WRITE"),
    RESERVED("READS"),
    RESERVED("REAL"),
    RESERVED("REFERENCES"),
    RESERVED("REGEXP"),
    RESERVED("RELEASE"),
    RESERVED("RENAME"),
    RESERVED("REPLACE"),
    RESERVED("REPEAT"),
    RESERVED("REQUIRE"),
    RESERVED("RESIGNAL"),
    RESERVED("RESTRICT"),
    RESERVED("RETURN"),
    RESERVED("REVOKE"),
    RESERVED("RIGHT"),
    RESERVED("REGEXP"),
    RESERVED("DATABASE"),
    RESERVED("DATABASES"),
    RESERVED("SECOND_MICROSECOND"),
    RESERVED("SELECT"),
    RESERVED("SENSITIVE"),
    RESERVED("SEPARATOR"),
    RESERVED("SET"),
    RESERVED("SHOW"),
    RESERVED("SIGNAL"),
    RESERVED("SMALLINT"),
    RESERVED("SPATIAL"),
    RESERVED("SPECIFIC"),
    RESERVED("SQL"),
    RESERVED("SQLEXCEPTION"),
    RESERVED("SQLSTATE"),
    RESERVED("SQLWARNING"),
    RESERVED("SQL_BIG_RESULT"),
    RESERVED("SQL_CALC_FOUND_ROWS"),
    RESERVED("SQL_SMALL_RESULT"),
    RESERVED("SSL"),
    RESERVED("STARTING"),
    RESERVED("STRAIGHT_JOIN"),
    RESERVED("TABLE"),
    RESERVED("TERMINATED"),
    RESERVED("THEN"),
    RESERVED("TINYBLOB"),
    RESERVED("TINYINT"),
    RESERVED("TINYTEXT"),
    RESERVED("TO"),
    RESERVED("TRAILING"),
    RESERVED("TRIGGER"),
    RESERVED("TRUE"),
    RESERVED("UNDO"),
    RESERVED("UNION"),
    RESERVED("UNIQUE"),
    RESERVED("UNLOCK"),
    RESERVED("UNSIGNED"),
    RESERVED("UPDATE"),
    RESERVED("USAGE"),
    RESERVED("USE"),
    RESERVED("USING"),
    RESERVED("UTC_DATE"),
    RESERVED("UTC_TIME"),
    RESERVED("UTC_TIMESTAMP"),
    RESERVED("VALUES"),
    RESERVED("VARBINARY"),
    RESERVED("VARCHAR"),
    RESERVED("VARCHAR"),
    RESERVED("VARYING"),
    RESERVED("WHEN"),
    RESERVED("WHERE"),
    RESERVED("WHILE"),
    RESERVED("WITH"),
    RESERVED("WRITE"),
    RESERVED("XOR"),
    RESERVED("YEAR_MONTH"),
    RESERVED("ZEROFILL"),
};

/**
 * NOTE:
 * sql/lex.h
 *
 * TODO:
 * - /#!VERSI ... #/
 **/
#if 0
#define my_isspace(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_SPC)
#define _MY_SPC 010     /* Spacing character */

#define _MY_CTR 040     /* Control character */
#define my_iscntrl(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_CTR)
#endif
static int mylex(YYLEX_ARGS)
{
    MyLexerData *mydata;
    const MyLexerOption *myoptions;

    (void) ctxt;
    mydata = (MyLexerData *) data;
    myoptions = (const MyLexerOption *) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;

/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

bdigit = [01];
digit = [0-9];
xdigit = [a-fA-F0-9];

space = [\x10-\x14];
control = [\x01-\x32];

// \N, synonym for NULL, is case sensitive (NULL is not)
null = 'null' | "\\N";
// true/false are also case insensitive
true = 'true';
false = 'false';

operator = [!:<>] "=" | "<=>" | "->" ">"? | "<>" | "&&" | "||" | ">>" | "<<" | [=&|~*^/<>%!+-] | "IS" | "X"? "OR" | "AND" | "MOD" | "NOT"; // TODO: case insensitive + keyword or not + (R)LIKE + REGEXP + SOUNDS LIKE ?

// SELECT CONCAT("'", GROUP_CONCAT(CHARACTER_SET_NAME ORDER BY CHARACTER_SET_NAME SEPARATOR "'\n | '"), "'") FROM INFORMATION_SCHEMA.CHARACTER_SETS;
charset = 'armscii8'
    | 'ascii'
    | 'big5'
    | 'binary'
    | 'cp1250'
    | 'cp1251'
    | 'cp1256'
    | 'cp1257'
    | 'cp850'
    | 'cp852'
    | 'cp866'
    | 'cp932'
    | 'dec8'
    | 'eucjpms'
    | 'euckr'
    | 'gb18030'
    | 'gb2312'
    | 'gbk'
    | 'geostd8'
    | 'greek'
    | 'hebrew'
    | 'hp8'
    | 'keybcs2'
    | 'koi8r'
    | 'koi8u'
    | 'latin1'
    | 'latin2'
    | 'latin5'
    | 'latin7'
    | 'macce'
    | 'macroman'
    | 'sjis'
    | 'swe7'
    | 'tis620'
    | 'ucs2'
    | 'ujis'
    | 'utf16'
    | 'utf16le'
    | 'utf32'
    | 'utf8' 'mb3'?
    | 'utf8mb4'
;

quoted_identifier = [\x01-\x7F\u0080-\uFFFF]+;
unquoted_identifier = [0-9a-zA-Z$_\u0080-\uFFFF]+;
identifier = unquoted_identifier; // TODO

// the “-- ” (double-dash) comment style requires the second dash to be followed by at least one whitespace or control character (such as a space, tab, newline, and so on). This syntax differs slightly from standard SQL comment syntax
<INITIAL> "--" (control | space) [^\n\r]* {
    TOKEN(COMMENT_SINGLE);
}

// C-comment style for extension
// The special comment format is very strict:
// '/' '*' '!', followed by exactly
// 1 digit (major), 2 digits (minor), then 2 digits (dot).
<INITIAL> "/*!" digit{5} {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

// C-comment style for hints
<INITIAL> "/*+" {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> "/*" {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT> "*/" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> null | true | false {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL> [+-]? (digit+ | digit* '.' digit+) ('e' [+-]? digit+)? {
    TOKEN(NUMBER);
}

<INITIAL> identifier {
    named_element_t *match, key = { (char *) YYTEXT, YYLENG };

    if (NULL == (match = bsearch(&key, keywords, ARRAY_SIZE(keywords), sizeof(keywords[0]), named_elements_casecmp))) {
        TOKEN(NAME);
    } else {
        if (myoptions->uppercase_keywords/* && KEYWORD == match->type*/) {
            TOKEN_OUTSRC(KEYWORD, (const YYCTYPE *) match->name, (const YYCTYPE *) match->name + match->name_len);
        } else {
            TOKEN(KEYWORD);
        }
    }
}

<INITIAL> ('n' | "_" charset)? "'" { // TODO: charset fonctionne aussi avec les " + il peut y avoir des espaces (et/ou commentaires) entre _charset et la quote
    mydata->delim = '\'';
    BEGIN(IN_STRING);
    TOKEN(STRING);
}

// binary strings as b'' may contain no digit but with 0b, one, at least, is needed
<INITIAL> 'b' "'" bdigit* "'" | '0b' bdigit+ {
    TOKEN(STRING_SINGLE);
}

// hexadecimal strings as x'' may contain no digit (and an even number of digits) but with 0x, one, at least, is needed (with an odd as even number of digits)
<INITIAL> 'x' "'" xdigit* "'" | '0x' xdigit+ {
    if ('x' == YYTEXT[0] && 0 == (YYLENG & 1)) {
        TOKEN(ERROR);
    } else {
        TOKEN(STRING_SINGLE);
    }
}

<INITIAL> operator {
    TOKEN(OPERATOR);
}

// TODO: @@variable? + ["'`]
// User variables are written as @var_name, where the variable name var_name consists of alphanumeric characters, “.”, “_”, and “$”. A user variable name can contain other characters if you quote it as a string or identifier (for example, @'my-var', @"my-var", or @`my-var`).
<INITIAL> "@" identifier {
    TOKEN(NAME_VARIABLE);
}

// parameter of a prepared statement
<INITIAL> "?" {
    TOKEN(NAME_VARIABLE);
}

<INITIAL> '"' {
    mydata->delim = '"';
    if (myoptions->ansi_quotes) {
        BEGIN(IN_IDENTIFIER);
        TOKEN(NAME);
    } else {
        BEGIN(IN_STRING);
        TOKEN(STRING);
    }
}

<IN_STRING> "\\" [0'"bnrtZ\\%_] {
    if (myoptions->no_backslash_escapes) {
        TOKEN(ESCAPED_CHAR);
    } else {
        TOKEN(STRING);
    }
}

<IN_STRING> '""' | "''" {
    if (*YYTEXT == mydata->delim) {
        TOKEN(ESCAPED_CHAR);
    } else {
        TOKEN(STRING);
    }
}

<IN_STRING> ['"] {
    if (*YYTEXT == mydata->delim) {
        BEGIN(INITIAL);
    }
    TOKEN(STRING);
}

<IN_IDENTIFIER> "``" {
    if ('`' == mydata->delim) {
        TOKEN(ESCAPED_CHAR);
    } else {
        TOKEN(NAME);
    }
}

<IN_IDENTIFIER> '""' {
    if (myoptions->ansi_quotes && '"' == mydata->delim) {
        TOKEN(ESCAPED_CHAR);
    } else {
        TOKEN(NAME);
    }
}

<IN_IDENTIFIER> [`"] {
    if (*YYTEXT == mydata->delim) {
        BEGIN(INITIAL);
    }
    TOKEN(NAME);
}

<*> [^] { // should be the last "rule"
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation mysql_lexer = {
    "MySQL",
    "Lexer for the MySQL dialect of SQL",
    (const char * const []) { "mariadb", NULL },
    NULL, // "*.sql" but it may conflict with future mysql & co?
    (const char * const []) { "text/x-mysql", NULL },
    NULL, // interpreters
    NULL, // analyze
    NULL, // init
    mylex,
    NULL,
    sizeof(MyLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("uppercase_keywords"),   OPT_TYPE_BOOL, offsetof(MyLexerOption, uppercase_keywords),   OPT_DEF_BOOL(0), "When true, MySQL keywords are uppercased" },
        { S("ansi_quotes"),          OPT_TYPE_BOOL, offsetof(MyLexerOption, ansi_quotes),          OPT_DEF_BOOL(0), "When true, double-quoted strings are identifiers instead of string literals." },
        { S("no_backslash_escapes"), OPT_TYPE_BOOL, offsetof(MyLexerOption, no_backslash_escapes), OPT_DEF_BOOL(0), "When true, disable the use of the backslash character as an escape character within strings, backslash becomes an ordinary character like any other." },
        END_OF_OPTIONS
    },
    NULL // dependencies
};