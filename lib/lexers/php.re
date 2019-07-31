/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2018 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   |          Nuno Lopes <nlopess@php.net>                                |
   |          Scott MacVicar <scottmac@php.net>                           |
   | Flex version authors:                                                |
   |          Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   +----------------------------------------------------------------------+
*/

/**
 * Spec/language reference:
 * - https://github.com/php/php-langspec/blob/master/spec/00-specification-for-php.md
 *
 * This file is based on PHP 7.3.8
 *
 * https://raw.githubusercontent.com/php/php-src/PHP-7.3.8/Zend/zend_language_scanner.l
 *
 * Don't forget to update this reference!
 */

#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"
#include "php.h"

typedef struct {
    LexerData data;
    bool in_namespace;
    bool in_doc_comment;
    char *doclabel; // (?:now|here)doc label
    size_t doclabel_len;
} PHPLexerData;

typedef struct {
    int version ALIGNED(sizeof(OptionValue));
    int start_inline ALIGNED(sizeof(OptionValue));
    int asp_tags ALIGNED(sizeof(OptionValue));
    int short_open_tag ALIGNED(sizeof(OptionValue));
    OptionValue secondary ALIGNED(sizeof(OptionValue));
} PHPLexerOption;

static int phpanalyse(const char *src, size_t src_len)
{
    int score;

    score = 0;
#if 1
    // TODO: look anywhere into the string?
    if (src_len >= STR_LEN("<?XXX") && (0 == ascii_memcasecmp(src, "<?php", STR_LEN("<?php")) || (0 == memcmp(src, "<?", STR_LEN("<?")) && 0 != memcmp(src, "<?xml", STR_LEN("<?xml"))))) {
        score = 999;
    }
#else
    char *found;
    void *kmp_ctxt;

    kmp_ctxt = kmp_init("<?php", STR_LEN("<?php"), KMP_INSENSITIVE);
    if (NULL != (found = kmp_search_first(src, src_len, kmp_ctxt))) {
        score = 999;
    }
    kmp_finalize(kmp_ctxt);
#endif

    return score;
}

enum {
    STATE(INITIAL),         // should be 1st as &data.state is interfaced as the (boolean) start_inline option
    STATE(ST_IN_SCRIPTING), // should be 2nd for the same reason
    STATE(ST_COMMENT_MULTI),
    STATE(ST_BACKQUOTE),
    STATE(ST_SINGLE_QUOTES),
    STATE(ST_DOUBLE_QUOTES),
    STATE(ST_NOWDOC),
    STATE(ST_HEREDOC),
    STATE(ST_VAR_OFFSET),
    STATE(ST_LOOKING_FOR_VARNAME),
    STATE(ST_LOOKING_FOR_PROPERTY)
};

static void phpinit(const OptionValue *options, LexerData *data, void *ctxt)
{
    Lexer *secondary;
    const PHPLexerOption *myoptions;

    myoptions = (const PHPLexerOption *) options;
    if (myoptions->start_inline) {
        BEGIN(ST_IN_SCRIPTING);
    }
    secondary = LEXER_UNWRAP(myoptions->secondary);
    if (NULL != secondary) {
        append_lexer(ctxt, secondary);
    }
}

static void phpfinalize(LexerData *data)
{
    PHPLexerData *mydata;

    mydata = (PHPLexerData *) data;
    if (NULL != mydata->doclabel) {
        free(mydata->doclabel);
    }
}

static int default_token_type[] = {
    [ STATE(INITIAL) ]                 = IGNORABLE,
    [ STATE(ST_IN_SCRIPTING) ]         = IGNORABLE,
    [ STATE(ST_COMMENT_MULTI) ]        = COMMENT_MULTILINE,
    [ STATE(ST_BACKQUOTE) ]            = STRING_BACKTICK,
    [ STATE(ST_SINGLE_QUOTES) ]        = STRING_SINGLE,
    [ STATE(ST_DOUBLE_QUOTES) ]        = STRING_DOUBLE,
    [ STATE(ST_NOWDOC) ]               = STRING_SINGLE,
    [ STATE(ST_HEREDOC) ]              = STRING_DOUBLE,
    [ STATE(ST_VAR_OFFSET) ]           = IGNORABLE,
    [ STATE(ST_LOOKING_FOR_VARNAME) ]  = IGNORABLE,
    [ STATE(ST_LOOKING_FOR_PROPERTY) ] = IGNORABLE,
};

static int default_token_value[] = {
    [ STATE(INITIAL) ]                 = T_IGNORE,
    [ STATE(ST_IN_SCRIPTING) ]         = -1,
    [ STATE(ST_COMMENT_MULTI) ]        = T_IGNORE,
    [ STATE(ST_VAR_OFFSET) ]           = -1,
    [ STATE(ST_SINGLE_QUOTES) ]        = T_IGNORE,
    [ STATE(ST_NOWDOC) ]               = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_DOUBLE_QUOTES) ]        = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_HEREDOC) ]              = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_BACKQUOTE) ]            = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_LOOKING_FOR_VARNAME) ]  = -1,
    [ STATE(ST_LOOKING_FOR_PROPERTY) ] = -1,
};

/**
 * NOTE:
 * - ' = case insensitive (ASCII letters only)
 * - " = case sensitive
 * (for re2c, by default, without --case-inverted or --case-insensitive)
 **/

#define IS_SPACE(c) \
    (' ' == c || '\n' == c || '\r' == c || '\t' == c)

#define IS_LABEL_START(c) \
    (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_' || (c) >= 0x7F)

static int phplex(YYLEX_ARGS)
{
    PHPLexerData *mydata;
    const PHPLexerOption *myoptions;

    (void) ctxt;
    mydata = (PHPLexerData *) data;
    myoptions = (const PHPLexerOption *) options;
    while (YYCURSOR < YYLIMIT) {
restart:
        YYTEXT = YYCURSOR;
// yymore_restart:
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
DNUM = ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*);
EXPONENT_DNUM = ((LNUM|DNUM)[eE][+-]?LNUM);
HNUM = "0" 'x' [0-9a-fA-F]+;
BNUM = "0" 'b' [01]+;
LABEL =  [a-zA-Z_\x7F-\xFF][a-zA-Z0-9_\x7F-\xFF]*;
WHITESPACE = [ \n\r\t]+;
TABS_AND_SPACES = [ \t]*;
TOKENS = [;:,.\[\]()|^&+-/*=%!~$<>?@];
ANY_CHAR = [^];
NEWLINE = ("\r"|"\n"|"\r\n");

<INITIAL> '<?php' ([ \t]|NEWLINE) {
    yyless(STR_LEN("<?php"));
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_TAG, T_IGNORE);
}

<INITIAL> "<?=" {
    // NOTE: <?= does not depend on short_open_tag since PHP 5.4.0
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_TAG, T_IGNORE);
}

<INITIAL> "<?" {
    if (myoptions->short_open_tag) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_IGNORE);
    } else {
        goto not_php; // if short_open_tag is off, give it to sublexer (if any)
    }
}

<INITIAL> "<%" {
    if (myoptions->version < 7 && myoptions->asp_tags) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_IGNORE);
    } else {
        goto not_php; // if asp_tags is off, give it to sublexer (if any)
    }
}

<INITIAL> '<script' WHITESPACE+ 'language' WHITESPACE* "=" WHITESPACE* ('php'|'"php"'|'\'php\'') WHITESPACE* ">" {
    if (myoptions->version < 7) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_IGNORE);
    } else {
        goto not_php;
    }
}

<ST_IN_SCRIPTING> '</script' WHITESPACE* ">" NEWLINE? {
    if (myoptions->version < 7) {
        BEGIN(INITIAL);
        VALUED_TOKEN(NAME_TAG, T_IGNORE);
    } else {
        VALUED_TOKEN(IGNORABLE, T_IGNORE);
    }
}

<ST_IN_SCRIPTING> "?>" NEWLINE? {
    BEGIN(INITIAL);
    VALUED_TOKEN(NAME_TAG, ';');
}

<ST_IN_SCRIPTING> "%>" NEWLINE? {
    if (myoptions->version < 7 && myoptions->asp_tags) {
        yyless(STR_LEN("%>"));
        BEGIN(INITIAL);
        VALUED_TOKEN(NAME_TAG, T_IGNORE);
    } else {
        VALUED_TOKEN(IGNORABLE, T_IGNORE);
    }
}

<ST_IN_SCRIPTING> "`" {
    BEGIN(ST_BACKQUOTE);
    VALUED_TOKEN(STRING_BACKTICK, *YYTEXT);
}

<ST_IN_SCRIPTING> "b"? "'" {
    BEGIN(ST_SINGLE_QUOTES);
    VALUED_TOKEN(STRING_SINGLE, T_CONSTANT_ENCAPSED_STRING);
}

<ST_IN_SCRIPTING> "b"? "\"" {
    BEGIN(ST_DOUBLE_QUOTES);
    VALUED_TOKEN(STRING_DOUBLE, '"');
}

// TODO: split "b"? "<<<" [ \t]* (["']?) LABEL \1 NEWLINE into separate tokens ?
<ST_IN_SCRIPTING>"b"? "<<<" TABS_AND_SPACES (LABEL | (["] LABEL ["]) | ((['] LABEL [']))) NEWLINE {
    const YYCTYPE *p;
    int bprefix, quoted;

    quoted = 0;
    bprefix = '<' != *YYTEXT;
//     if (NULL != mydata->doclabel) {
//         free(mydata->doclabel);
//     }
    for (p = YYTEXT + bprefix + STR_LEN("<<<"); '\t' == *p || ' ' == *p; p++)
        ;
    switch (*p) {
        case '\'':
            ++p;
            quoted = 1;
            BEGIN(ST_NOWDOC);
            break;
        case '"':
            ++p;
            quoted = 1;
            /* no break */
        default:
            BEGIN(ST_HEREDOC);
            break;
    }
    mydata->doclabel_len = YYCURSOR - p - quoted - STR_LEN("\n") - ('\r' == YYCURSOR[-2]);
    mydata->doclabel = strndup((char *) p, mydata->doclabel_len);

    VALUED_TOKEN(default_token_type[YYSTATE], T_START_HEREDOC);
}

<ST_HEREDOC,ST_NOWDOC> NEWLINE TABS_AND_SPACES LABEL [^a-zA-Z0-9_\x80-\xFF] {
    int old_state, token_value;
    // beginning and end of the LABEL
    // - s is on the first character of the LABEL
    // - e is on the comma (';'), after the last character of the LABEL
    const YYCTYPE *s, *e;

    s = YYTEXT;
    e = YYCURSOR;
    old_state = YYSTATE;
    token_value = T_ENCAPSED_AND_WHITESPACE;
    // skip newline before label
    if ('\r' == *s) {
        ++s;
    }
    if ('\n' == *s) {
        ++s;
    }
    while (' ' == *s || '\t' == *s) {
        ++s;
    }
    --e; // YYCURSOR is on the next character after the last NEWLINE, so "decrement" it first
    if (mydata->doclabel_len == SIZE_T(e - s) && 0 == memcmp(s, mydata->doclabel, mydata->doclabel_len)) {
        yyless((e - YYTEXT));
        free(mydata->doclabel);
        mydata->doclabel = NULL;
        mydata->doclabel_len = 0;
        token_value = T_END_HEREDOC;

        BEGIN(ST_IN_SCRIPTING);
    }
    VALUED_TOKEN(default_token_type[old_state], token_value);
}

<ST_IN_SCRIPTING> "__" ('CLASS' | 'TRAIT' | 'FUNCTION' | 'METHOD' | 'LINE' | 'FILE' | 'DIR' | 'NAMESPACE') "__" {
    VALUED_TOKEN(OPERATOR, T_INTERNAL_CONSTANT);
}

<ST_IN_SCRIPTING> "::" {
    VALUED_TOKEN(OPERATOR, T_PAAMAYIM_NEKUDOTAYIM);
}

<ST_IN_SCRIPTING> "\\" {
    VALUED_TOKEN(NAME, T_NS_SEPARATOR);
}

<ST_IN_SCRIPTING> "=>" {
    VALUED_TOKEN(OPERATOR, T_DOUBLE_ARROW);
}

<ST_IN_SCRIPTING> "->" {
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    VALUED_TOKEN(OPERATOR, T_OBJECT_OPERATOR);
}

<ST_LOOKING_FOR_PROPERTY> "->" {
    VALUED_TOKEN(OPERATOR, T_OBJECT_OPERATOR);
}

<ST_LOOKING_FOR_PROPERTY> LABEL {
    POP_STATE();
    VALUED_TOKEN(NAME_VARIABLE_INSTANCE, T_STRING);
}

<ST_IN_SCRIPTING,ST_LOOKING_FOR_PROPERTY> WHITESPACE+ {
    VALUED_TOKEN(IGNORABLE, T_IGNORE);
}

<ST_LOOKING_FOR_PROPERTY> ANY_CHAR {
    yyless(0);
    POP_STATE();
    goto restart;
}

<ST_IN_SCRIPTING> "++" {
    VALUED_TOKEN(OPERATOR, T_INC);
}

<ST_IN_SCRIPTING> "..." {
    VALUED_TOKEN(OPERATOR, T_ELLIPSIS);
}

<ST_IN_SCRIPTING> "--" {
    VALUED_TOKEN(OPERATOR, T_DEC);
}

<ST_IN_SCRIPTING> "(" TABS_AND_SPACES ('int' | 'integer' | 'bool' | 'boolean' | 'string' | 'binary' | 'real' | 'float' | 'double' | 'array' | 'object' | 'unset') TABS_AND_SPACES ")" {
    VALUED_TOKEN(OPERATOR, T_CAST);
}

<ST_IN_SCRIPTING> ([&|./*%+^-] | "**" | "<<" | ">>") "=" {
    VALUED_TOKEN(OPERATOR, T_EQUAL_OP);
}

<ST_IN_SCRIPTING> 'abstract' {
    VALUED_TOKEN(KEYWORD, T_ABSTRACT);
}

<ST_IN_SCRIPTING> 'array' {
    VALUED_TOKEN(KEYWORD, T_ARRAY);
}

<ST_IN_SCRIPTING> 'as' {
    VALUED_TOKEN(KEYWORD, T_AS);
}

<ST_IN_SCRIPTING> 'break' {
    VALUED_TOKEN(KEYWORD, T_BREAK);
}

<ST_IN_SCRIPTING> 'callable' {
    VALUED_TOKEN(KEYWORD_TYPE, T_CALLABLE);
}

<ST_IN_SCRIPTING> 'case' {
    VALUED_TOKEN(KEYWORD, T_CASE);
}

<ST_IN_SCRIPTING> 'catch' {
    VALUED_TOKEN(KEYWORD, T_CATCH);
}

<ST_IN_SCRIPTING> 'class' {
    VALUED_TOKEN(KEYWORD, T_CLASS);
}

<ST_IN_SCRIPTING> 'clone' {
    VALUED_TOKEN(OPERATOR, T_CLONE);
}

<ST_IN_SCRIPTING> 'const' {
    VALUED_TOKEN(KEYWORD, T_CONST);
}

<ST_IN_SCRIPTING> 'continue' {
    VALUED_TOKEN(KEYWORD, T_CONTINUE);
}

<ST_IN_SCRIPTING> 'declare' {
    VALUED_TOKEN(KEYWORD, T_DECLARE);
}

<ST_IN_SCRIPTING> 'default' {
    VALUED_TOKEN(KEYWORD, T_DEFAULT);
}

<ST_IN_SCRIPTING> 'do' {
    VALUED_TOKEN(KEYWORD, T_DO);
}

<ST_IN_SCRIPTING> 'echo' {
    VALUED_TOKEN(KEYWORD, T_ECHO);
}

<ST_IN_SCRIPTING> 'elseif' {
    VALUED_TOKEN(KEYWORD, T_ELSEIF);
}

<ST_IN_SCRIPTING> 'else' {
    VALUED_TOKEN(KEYWORD, T_ELSE);
}

<ST_IN_SCRIPTING> 'empty' | 'eval' {
    VALUED_TOKEN(KEYWORD, T_FUNCTION_LIKE_KEYWORD);
}

<ST_IN_SCRIPTING> 'enddeclare' {
    VALUED_TOKEN(KEYWORD, T_ENDDECLARE);
}

<ST_IN_SCRIPTING> 'endforeach' {
    VALUED_TOKEN(KEYWORD, T_ENDFOREACH);
}

<ST_IN_SCRIPTING> 'endfor' {
    VALUED_TOKEN(KEYWORD, T_ENDFOR);
}

<ST_IN_SCRIPTING> 'endif' {
    VALUED_TOKEN(KEYWORD, T_ENDIF);
}

<ST_IN_SCRIPTING> 'endswitch' {
    VALUED_TOKEN(KEYWORD, T_ENDSWITCH);
}

<ST_IN_SCRIPTING> 'endwhile' {
    VALUED_TOKEN(KEYWORD, T_ENDWHILE);
}

<ST_IN_SCRIPTING> 'exit' | 'die' {
    VALUED_TOKEN(KEYWORD, T_EXIT);
}

<ST_IN_SCRIPTING> 'extends' {
    VALUED_TOKEN(KEYWORD, T_EXTENDS);
}

<ST_IN_SCRIPTING> 'finally' {
    VALUED_TOKEN(KEYWORD, T_FINALLY);
}

<ST_IN_SCRIPTING> 'final' {
    VALUED_TOKEN(KEYWORD, T_FINAL);
}

<ST_IN_SCRIPTING> 'foreach' {
    VALUED_TOKEN(KEYWORD, T_FOREACH);
}

<ST_IN_SCRIPTING> 'for' {
    VALUED_TOKEN(KEYWORD, T_FOR);
}

<ST_IN_SCRIPTING> 'function' {
    VALUED_TOKEN(KEYWORD, T_FUNCTION);
}

<ST_IN_SCRIPTING> 'global' {
    VALUED_TOKEN(KEYWORD, T_GLOBAL);
}

<ST_IN_SCRIPTING> 'goto' {
    VALUED_TOKEN(KEYWORD, T_GOTO);
}

<ST_IN_SCRIPTING> '__halt_compiler' {
    VALUED_TOKEN(KEYWORD, T_HALT_COMPILER);
}

<ST_IN_SCRIPTING> 'if' {
    VALUED_TOKEN(KEYWORD, T_IF);
}

<ST_IN_SCRIPTING> 'implements' {
    VALUED_TOKEN(KEYWORD, T_IMPLEMENTS);
}

<ST_IN_SCRIPTING> ('include' | 'require') ('_once')? {
    VALUED_TOKEN(KEYWORD, T_INCLUDE_REQUIRE);
}

<ST_IN_SCRIPTING> 'instanceof' {
    VALUED_TOKEN(KEYWORD, T_INSTANCEOF);
}

<ST_IN_SCRIPTING> 'insteadof' {
    VALUED_TOKEN(KEYWORD, T_INSTEADOF);
}

<ST_IN_SCRIPTING> 'interface' {
    VALUED_TOKEN(KEYWORD, T_INTERFACE);
}

<ST_IN_SCRIPTING> 'isset' {
    VALUED_TOKEN(KEYWORD, T_ISSET);
}

<ST_IN_SCRIPTING> 'list' {
    VALUED_TOKEN(KEYWORD, T_LIST);
}

<ST_IN_SCRIPTING> 'new' {
    VALUED_TOKEN(KEYWORD, T_NEW);
}

<ST_IN_SCRIPTING> 'namespace' {
    VALUED_TOKEN(KEYWORD, T_NAMESPACE);
}

<ST_IN_SCRIPTING> 'public' | 'protected' | 'private' {
    VALUED_TOKEN(KEYWORD, T_VISIBILITY);
}

<ST_IN_SCRIPTING> 'print' {
    VALUED_TOKEN(KEYWORD, T_PRINT);
}

<ST_IN_SCRIPTING> 'return' {
    VALUED_TOKEN(KEYWORD, T_RETURN);
}

<ST_IN_SCRIPTING> 'static' {
    VALUED_TOKEN(KEYWORD, T_STATIC);
}

<ST_IN_SCRIPTING> 'switch' {
    VALUED_TOKEN(KEYWORD, T_SWITCH);
}

<ST_IN_SCRIPTING> [?:] {
    VALUED_TOKEN(OPERATOR, *YYTEXT);
}

<ST_IN_SCRIPTING> 'xor' | 'or' | 'and' {
    VALUED_TOKEN(OPERATOR, T_LOGICAL_BINARY_OP);
}

<ST_IN_SCRIPTING> "&&" | "||" | "**" | "<=>" | "??" | "<<" | ">>" | [<>]"=" | [!=]"="{1,2} | [<>/%|&.+*^-] {
    VALUED_TOKEN(OPERATOR, T_BINARY_OP);
}

<ST_IN_SCRIPTING> 'throw' {
    VALUED_TOKEN(KEYWORD, T_THROW);
}

<ST_IN_SCRIPTING> 'trait' {
    VALUED_TOKEN(KEYWORD, T_TRAIT);
}

<ST_IN_SCRIPTING> 'try' {
    VALUED_TOKEN(KEYWORD, T_TRY);
}

<ST_IN_SCRIPTING> 'unset' {
    VALUED_TOKEN(KEYWORD, T_UNSET);
}

<ST_IN_SCRIPTING> 'use' {
    VALUED_TOKEN(KEYWORD, T_USE);
}

<ST_IN_SCRIPTING> 'var' {
    VALUED_TOKEN(KEYWORD, T_VAR);
}

<ST_IN_SCRIPTING> 'while' {
    VALUED_TOKEN(KEYWORD, T_WHILE);
}

<ST_IN_SCRIPTING> 'yield' WHITESPACE 'from' [^a-zA-Z0-9_\x80-\xFF] {
    yyless(STR_LEN("yield from"));
    VALUED_TOKEN(KEYWORD, T_YIELD_FROM);
}

<ST_IN_SCRIPTING> 'yield' {
    VALUED_TOKEN(KEYWORD, T_YIELD);
}

<ST_IN_SCRIPTING> "#" | "//" {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                break;
            case '%':
                if (!myoptions->asp_tags) {
                    continue;
                }
                /* fall through */
            case '?':
                if ('>' == *YYCURSOR) {
                    YYCURSOR--;
                    break;
                }
                /* fall through */
            default:
                continue;
        }
        break;
    }
    VALUED_TOKEN(COMMENT_SINGLE, T_IGNORE);
}

<ST_IN_SCRIPTING> LABEL {
    VALUED_TOKEN(NAME, T_STRING);
}

<ST_IN_SCRIPTING> LNUM {
    if ('0' == *YYTEXT) {
        VALUED_TOKEN(NUMBER_OCTAL, T_LNUMBER);
    } else {
        VALUED_TOKEN(NUMBER_DECIMAL, T_LNUMBER);
    }
}

<ST_IN_SCRIPTING> HNUM {
    VALUED_TOKEN(NUMBER_HEXADECIMAL, T_LNUMBER);
}

<ST_IN_SCRIPTING> BNUM {
    VALUED_TOKEN(NUMBER_BINARY, T_LNUMBER);
}

<ST_IN_SCRIPTING> DNUM | EXPONENT_DNUM {
    VALUED_TOKEN(NUMBER_FLOAT, T_DNUMBER);
}

<ST_IN_SCRIPTING> TOKENS {
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_IN_SCRIPTING> "{" {
    PUSH_STATE(ST_IN_SCRIPTING);
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_IN_SCRIPTING> "}" {
//     if (darray_length(&myoptions->state_stack)) {
        POP_STATE();
//     }
    if (STATE(ST_IN_SCRIPTING) == YYSTATE) {
        VALUED_TOKEN(PUNCTUATION, *YYTEXT);
    } else {
        VALUED_TOKEN(SEQUENCE_INTERPOLATED, *YYTEXT);
    }
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "\\u{" [0-9a-fA-F]+ "}" {
    if (myoptions->version >= 7) {
        if (!check_codepoint(YYTEXT, YYLIMIT, &YYCURSOR, "\\u{", STR_LEN("\\u{"), "}", STR_LEN("}"), 1, (size_t) -1, 16 | 32)) {
            TOKEN(ERROR);
        }
        VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
    } else {
        VALUED_TOKEN(default_token_type[YYSTATE], T_ENCAPSED_AND_WHITESPACE);
    }
}

<ST_DOUBLE_QUOTES> "\\\"" {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_BACKQUOTE> "\\`" {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> ("\\0"[0-9]{2}) | ("\\" 'x' [0-9A-Fa-f]{2}) | ("\\"[$efrntv\\]) {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_DOUBLE_QUOTES> "\"" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_DOUBLE, *YYTEXT);
}

<ST_IN_SCRIPTING>"/**" WHITESPACE {
#if 1
    mydata->in_doc_comment = true;
    BEGIN(ST_COMMENT_MULTI);
    VALUED_TOKEN(COMMENT_DOCUMENTATION, T_IGNORE);
#else
    // TODO
    YYCTYPE *end;

    if (YYCURSOR > YYLIMIT) {
        DONE();
    }
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "*/", STR_LEN("*/"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + STR_LEN("*/") + 1;
    }
    DELEGATE_UNTIL(COMMENT_MULTILINE);
#endif
}

<ST_IN_SCRIPTING> "/*" {
    mydata->in_doc_comment = false;
    BEGIN(ST_COMMENT_MULTI);
    VALUED_TOKEN(COMMENT_MULTILINE, T_IGNORE);
}

<ST_COMMENT_MULTI> "*/" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(mydata->in_doc_comment ? COMMENT_DOCUMENTATION : COMMENT_MULTILINE, T_IGNORE);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "${" {
    PUSH_STATE(ST_LOOKING_FOR_VARNAME);
    VALUED_TOKEN(SEQUENCE_INTERPOLATED, T_DOLLAR_OPEN_CURLY_BRACES);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "{$" {
    PUSH_STATE(ST_IN_SCRIPTING);
    yyless(1);
    VALUED_TOKEN(SEQUENCE_INTERPOLATED, T_CURLY_OPEN); // token is shorten to '{'
}

<ST_LOOKING_FOR_VARNAME> LABEL [[}] {
    yyless(YYLENG - 1);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_VARIABLE, T_STRING_VARNAME);
}

<ST_LOOKING_FOR_VARNAME> ANY_CHAR {
    yyless(0);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    goto restart;
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE> "$" LABEL "->" [a-zA-Z_\x7F-\xFF] {
    yyless(YYLENG - 3);
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE> "$" LABEL "[" {
    yyless(YYLENG - 1);
    PUSH_STATE(ST_VAR_OFFSET);
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE,ST_VAR_OFFSET> "$" LABEL {
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_VAR_OFFSET> "]" {
    POP_STATE();
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_VAR_OFFSET> [0] | [1-9][0-9]* {
    VALUED_TOKEN(NUMBER_DECIMAL, T_NUM_STRING);
}

<ST_VAR_OFFSET> LNUM | HNUM | BNUM {
    /* Offset must be treated as a string */
    VALUED_TOKEN(STRING_SINGLE, T_NUM_STRING);
}

<ST_VAR_OFFSET> "[" {
    VALUED_TOKEN(PUNCTUATION, *YYTEXT); // ???
}

<ST_VAR_OFFSET> LABEL {
    VALUED_TOKEN(NAME_CONSTANT, T_STRING);
}

<ST_VAR_OFFSET>[ \n\r\t\\'#] {
    /* Invalid rule to return a more explicit parse error with proper line number */
    yyless(0);
    POP_STATE();
    VALUED_TOKEN(IGNORABLE, T_ENCAPSED_AND_WHITESPACE);
}

<ST_BACKQUOTE> "\\" [\\`] {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_BACKQUOTE> "`" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_BACKTICK, *YYTEXT);
}

<ST_SINGLE_QUOTES> "\\" [\\'] {
    VALUED_TOKEN(ESCAPED_CHAR, T_IGNORE);
}

<ST_SINGLE_QUOTES> "'" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_SINGLE, T_IGNORE);
}

<INITIAL> ANY_CHAR {
#define STRNCASECMP(s) \
    ascii_strncasecmp_l(s, STR_LEN(s), (char *) ptr, YYLIMIT - ptr, STR_LEN(s))
not_php:
    if (YYCURSOR > YYLIMIT) {
        DONE();
    }
    while (1) {
        YYCTYPE *ptr;

        if (NULL == (ptr = memchr(YYCURSOR, '<', YYLIMIT - YYCURSOR))) {
            YYCURSOR = YYLIMIT;
            break;
        } else {
            YYCURSOR = ptr + 1;
        }
        if (YYCURSOR < YYLIMIT) {
            ptr += STR_LEN("<X");
            switch (*YYCURSOR) {
                case '?':
                    if ((myoptions->version < 7 && myoptions->short_open_tag) || 0 == STRNCASECMP("php") || ('=' == *(YYCURSOR + 1))) { /* Assume [ \t\n\r] follows "php" */
                        break;
                    }
                    continue;
                case '%':
                    if (myoptions->version < 7 && myoptions->asp_tags) {
                        break;
                    }
                    continue;
                case 's':
                case 'S':
                    if (myoptions->version < 7) {
                        // '<script' WHITESPACE+ 'language' WHITESPACE* "=" WHITESPACE* ('php'|'"php"'|'\'php\'') WHITESPACE* ">"
                        if (0 == STRNCASECMP("cript")) {
                            ptr += STR_LEN("cript");
                            if (ptr < YYLIMIT && IS_SPACE(*ptr)) {
                                ++ptr;
                                while (ptr < YYLIMIT && IS_SPACE(*ptr)) {
                                    ++ptr;
                                }
                                if (0 == STRNCASECMP("language")) {
                                    ptr += STR_LEN("language");
                                    while (ptr < YYLIMIT && IS_SPACE(*ptr)) {
                                        ++ptr;
                                    }
                                    if ('=' == *ptr) {
                                        YYCTYPE quote;

                                        ++ptr;
                                        while (ptr < YYLIMIT && IS_SPACE(*ptr)) {
                                            ++ptr;
                                        }
                                        quote = *ptr;
                                        if ('\'' == quote || '"' == quote) {
                                            ++ptr;
                                        } else {
                                            quote = 0;
                                        }
                                        if (STRNCASECMP("php")) {
                                            ptr += STR_LEN("php");
                                            if (0 == quote || *ptr == quote) {
                                                if (quote) {
                                                    ++ptr;
                                                }
                                                while (ptr < YYLIMIT && IS_SPACE(*ptr)) {
                                                    ++ptr;
                                                }
                                                if ('>' == *ptr) {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
//                     YYCURSOR--;
//                     yymore();
                    /* no break - implicit continue */
                default:
                    continue;
            }
            YYCURSOR--;
        }
        break;
    }
// debug("NON PHP = ---\n%.*s\n---", YYLENG, YYTEXT);
    DELEGATE_UNTIL(IGNORABLE);
}

<ST_COMMENT_MULTI> ANY_CHAR {
    VALUED_TOKEN(mydata->in_doc_comment ? COMMENT_DOCUMENTATION : COMMENT_MULTILINE, T_IGNORE);
}

<*> ANY_CHAR { // should be the last "rule"
    int type;

    if (-1 == (type = default_token_value[YYSTATE])) {
        type = *YYTEXT;
    }
    VALUED_TOKEN(default_token_type[YYSTATE], type);
}
*/
    }
    DONE();
}

LexerImplementation php_lexer = {
    "PHP",
    "For PHP source code",
    (const char * const []) { "php3", "php4", "php5", "php7", NULL },
    (const char * const []) { "*.php", "*.php[3457]", "*.inc", NULL },
    (const char * const []) { "text/x-php", "application/x-httpd-php", NULL },
    (const char * const []) { "php", "php-cli", "php[3457]*", NULL },
    phpanalyse,
    phpinit,
    phplex,
    phpfinalize,
    sizeof(PHPLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("version"),        OPT_TYPE_INT,   offsetof(PHPLexerOption, version),        OPT_DEF_INT(7),  "major versions of PHP brings some changes, use this parameter to parse PHP code as a newer or older version" },
        { S("asp_tags"),       OPT_TYPE_BOOL,  offsetof(PHPLexerOption, asp_tags),       OPT_DEF_BOOL(0), "support, or not, `<%`/`%>` tags to begin/end PHP code ([asp_tags](http://php.net/asp_tags)) (only if version < 7)" },
        { S("start_inline"),   OPT_TYPE_BOOL,  offsetof(PHPLexerOption, start_inline),   OPT_DEF_BOOL(0), "if true the lexer starts highlighting with php code (ie no starting `<?php`/`<?`/`<script language=\"php\">` is required at top)" },
        { S("short_open_tag"), OPT_TYPE_BOOL,  offsetof(PHPLexerOption, short_open_tag), OPT_DEF_BOOL(1), "support, or not, `<?` tags to begin PHP code ([short_open_tag](http://php.net/short_open_tag))" },
        { S("secondary"),      OPT_TYPE_LEXER, offsetof(PHPLexerOption, secondary),      OPT_DEF_LEXER,   "Lexer to highlight content outside of PHP tags (if none, these parts will not be highlighted)" },
        END_OF_OPTIONS
    },
    NULL, // dependencies
    phppush_parse,
    phppstate_new,
    phppstate_delete
};
