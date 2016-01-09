/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2015 Zend Technologies Ltd. (http://www.zend.com) |
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
   |          Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "tokens.h"
#include "lexer.h"

extern const LexerImplementation annotations_lexer;

typedef struct {
    LexerData data;
    int short_tags ALIGNED(sizeof(OptionValue));
    int asp_tags ALIGNED(sizeof(OptionValue));
    int version ALIGNED(sizeof(OptionValue));
    OptionValue secondary ALIGNED(sizeof(OptionValue));
    int in_namespace;
    char *doclabel; // (?:now|here)doc label // TODO: may leak
    size_t doclabel_len;
} PHPLexerData;

static int phpanalyse(const char *src, size_t src_len)
{
    // TODO: "<?php" is case insentive
    if (src_len >= STR_LEN("<?XXX") && (0 == memcmp(src, "<?php", STR_LEN("<?php")) || (0 == memcmp(src, "<?", STR_LEN("<?")) && 0 != memcmp(src, "<?xml", STR_LEN("<?xml"))))) {
        return 999;
    }

    return 0;
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
    STATE(ST_END_NOWDOC),
    STATE(ST_END_HEREDOC),
    STATE(ST_VAR_OFFSET),
    STATE(ST_LOOKING_FOR_VARNAME),
    STATE(ST_LOOKING_FOR_PROPERTY)
};

static int default_token_type[] = {
    IGNORABLE, // INITIAL
    IGNORABLE, // ST_IN_SCRIPTING
    COMMENT_MULTILINE, // ST_COMMENT_MULTI
    STRING_BACKTICK, // ST_BACKQUOTE
    STRING_SINGLE, // ST_SINGLE_QUOTES
    STRING_DOUBLE, // ST_DOUBLE_QUOTES
    STRING_SINGLE, // ST_NOWDOC
    STRING_DOUBLE, // ST_HEREDOC
    STRING_SINGLE, // ST_END_NOWDOC
    STRING_DOUBLE, // ST_END_HEREDOC
    IGNORABLE, // ST_VAR_OFFSET
    IGNORABLE, // ST_LOOKING_FOR_VARNAME
    IGNORABLE  // ST_LOOKING_FOR_PROPERTY
};

// http://lxr.php.net/xref/phpng/Zend/zend_language_scanner.l

#define DECL_OP(s) \
    { OPERATOR, s, STR_LEN(s) },
#define DECL_KW(s) \
    { KEYWORD, s, STR_LEN(s) },
#define DECL_KW_NS(s) \
    { KEYWORD_NAMESPACE, s, STR_LEN(s) },
#define DECL_KW_DECL(s) \
    { KEYWORD_DECLARATION, s, STR_LEN(s) },
#define DECL_KW_CONSTANT(s) \
    { KEYWORD_CONSTANT, s, STR_LEN(s) },
#define DECL_NAME_B(s) \
    { NAME_BUILTIN, s, STR_LEN(s) },
#define DECL_NAME_BP(s) \
    { NAME_BUILTIN_PSEUDO, s, STR_LEN(s) },

// https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html#Designated-Inits
static int case_insentive[_TOKEN_COUNT] = {
    [ OPERATOR ] = 1,
    [ KEYWORD ] = 1,
    [ KEYWORD_CONSTANT ] = 1,
    [ KEYWORD_DECLARATION ] = 1,
    [ KEYWORD_NAMESPACE ] = 1,
    [ NAME_BUILTIN ] = 1,
    [ NAME_BUILTIN_PSEUDO ] = 1,
};

// stdClass, __PHP_Incomplete_Class

static struct {
    int type;
    const char *name;
    size_t name_len;
} keywords[] = {
    //
    DECL_OP("and")
    DECL_OP("or")
    DECL_OP("xor")
    //
    DECL_KW_CONSTANT("NULL")
    DECL_KW_CONSTANT("TRUE")
    DECL_KW_CONSTANT("FALSE")
    // http://php.net/manual/fr/reserved.constants.php
    // ...
    //
    DECL_NAME_BP("this")
    DECL_NAME_BP("self")
    DECL_NAME_BP("parent")
    DECL_NAME_BP("__CLASS__")
    DECL_NAME_BP("__COMPILER_HALT_OFFSET__")
    DECL_NAME_BP("__DIR__")
    DECL_NAME_BP("__FILE__")
    DECL_NAME_BP("__FUNCTION__")
    DECL_NAME_BP("__LINE__")
    DECL_NAME_BP("__METHOD__")
    DECL_NAME_BP("__NAMESPACE__")
    DECL_NAME_BP("__TRAIT__")
    //
    DECL_NAME_B("__halt_compiler")
    DECL_NAME_B("echo")
    DECL_NAME_B("empty")
    DECL_NAME_B("eval")
    DECL_NAME_B("exit")
    DECL_NAME_B("die")
    DECL_NAME_B("include")
    DECL_NAME_B("include_once")
    DECL_NAME_B("isset")
    DECL_NAME_B("list")
    DECL_NAME_B("print")
    DECL_NAME_B("require")
    DECL_NAME_B("require_once")
    DECL_NAME_B("unset")
    //
//     DECL_KW_NS("namespace")
//     DECL_KW("use") // is used for namespaces and by closures
    //
    DECL_KW_DECL("var")
    DECL_KW("abstract")
    DECL_KW("final")
    DECL_KW("private")
    DECL_KW("protected")
    DECL_KW("public")
    DECL_KW("static")
    DECL_KW("extends")
    DECL_KW("implements")
    //
    DECL_KW("callable")
    DECL_KW("insteadof")
    DECL_KW("new")
    DECL_KW("clone")
    DECL_KW("class")
    DECL_KW("trait")
    DECL_KW("interface")
    DECL_KW("function")
    DECL_KW("global")
    DECL_KW("const")
    DECL_KW("return")
    DECL_KW("yield")
    DECL_KW("try")
    DECL_KW("catch")
    DECL_KW("finally")
    DECL_KW("throw")
    DECL_KW("if")
    DECL_KW("else")
    DECL_KW("elseif")
    DECL_KW("endif")
    DECL_KW("while")
    DECL_KW("endwhile")
    DECL_KW("do")
    DECL_KW("for")
    DECL_KW("endfor")
    DECL_KW("foreach")
    DECL_KW("endforeach")
    DECL_KW("declare")
    DECL_KW("enddeclare")
    DECL_KW("instanceof")
    DECL_KW("as")
    DECL_KW("switch")
    DECL_KW("endswitch")
    DECL_KW("case")
    DECL_KW("default")
    DECL_KW("break")
    DECL_KW("continue")
    DECL_KW("goto")
};

#if 0 /* UNUSED YET */
static named_element_t functions[] = {
    NE("urlencode"),
    NE("array_key_exists"),
};

static named_element_t classes[] = {
    NE("Directory"),
    NE("stdClass"),
    NE("__PHP_Incomplete_Class"),
};
#endif

/**
 * NOTE:
 * - ' = case insensitive (ASCII letters only)
 * - " = case sensitive
 * (for re2c, by default, without --case-inverted or --case-insensitive)
 **/

/**
 * TODO:
 * - :: et ->
 * - array reconnue comme une fonction ?
 * - tout ce qui est interpolé
 * - dimension de tableau (machin[bidule])
 **/

#define IS_SPACE(c) \
    (' ' == c || '\n' == c || '\r' == c || '\t' == c)

#define IS_LABEL_START(c) \
    (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_' || (c) >= 0x7F)

static int phplex(YYLEX_ARGS) {
    PHPLexerData *mydata;

    mydata = (PHPLexerData *) data;
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
LABEL =  [a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*;
NAMESPACED_LABEL = LABEL? ("\\" LABEL)+;
WHITESPACE = [ \n\r\t]+;
TABS_AND_SPACES = [ \t]*;
TOKENS = [;:,.\[\]()|^&+-/*=%!~$<>?@];
ANY_CHAR = [^];
NEWLINE = ("\r"|"\n"|"\r\n");

<INITIAL>'<?php' ([ \t]|NEWLINE) {
    //yy->yyleng = STR_LEN("<?php");
    yyless(STR_LEN("<?php"));
    BEGIN(ST_IN_SCRIPTING);
    TOKEN(NAME_TAG);
}

<INITIAL>"<?=" {
    // NOTE: <?= does not depend on short_open_tag since PHP 5.4.0
    BEGIN(ST_IN_SCRIPTING);
    TOKEN(NAME_TAG);
}

<INITIAL>"<?" {
    if (mydata->short_tags) {
        BEGIN(ST_IN_SCRIPTING);
        TOKEN(NAME_TAG);
    } else {
        goto not_php; // if short_open_tag is off, give it to sublexer (if any)
    }
}

<INITIAL>"<%" {
    if (mydata->version < 7 && mydata->asp_tags) {
        BEGIN(ST_IN_SCRIPTING);
        TOKEN(NAME_TAG);
    } else {
        goto not_php; // if asp_tags is off, give it to sublexer (if any)
    }
}

<INITIAL>'<script' WHITESPACE+ 'language' WHITESPACE* "=" WHITESPACE* ('php'|'"php"'|'\'php\'') WHITESPACE* ">" {
    if (mydata->version < 7) {
        BEGIN(ST_IN_SCRIPTING);
        TOKEN(NAME_TAG);
    } else {
        goto not_php;
    }
}

<ST_IN_SCRIPTING> "=>" {
    TOKEN(OPERATOR);
}

<ST_IN_SCRIPTING>"->" {
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    TOKEN(OPERATOR);
}

<ST_IN_SCRIPTING>'yield' WHITESPACE 'from' {
    if (mydata->version < 7) {
        yyless(STR_LEN("yield"));
    }
    TOKEN(KEYWORD);
}

<ST_LOOKING_FOR_PROPERTY>"->" {
    TOKEN(OPERATOR);
}

<ST_LOOKING_FOR_PROPERTY>LABEL {
    POP_STATE();
    TOKEN(NAME_VARIABLE_INSTANCE);
}

<ST_LOOKING_FOR_PROPERTY>ANY_CHAR {
    yyless(0);
    POP_STATE();
    goto restart;
}

<ST_IN_SCRIPTING>"??" | "<=>" {
    if (mydata->version >= 7) {
        //yyless(STR_LEN("?"));
        //yyless(STR_LEN("<="));
        yyless(YYLENG - 1);
    }
    TOKEN(OPERATOR);
}

<ST_IN_SCRIPTING> "++" | "--" | [!=]"==" | "<>" | [-+*/%.<>+&|^!=]"=" | ">>=" | "<<=" | "**=" | "<<" | ">>" | "**" | [-+.*/%=^&|!~<>?:@] {
    TOKEN(OPERATOR);
}

<ST_IN_SCRIPTING>"#" | "//" {
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
                if (!mydata->asp_tags) {
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
    TOKEN(COMMENT_SINGLE);
}

/*
<ST_IN_SCRIPTING>"(" TABS_AND_SPACES ('int' | 'integer' | 'bool' | 'boolean' | 'string' | 'binary' | 'real' | 'float' | 'double' | 'array' | 'object' | 'unset') TABS_AND_SPACES ")" {
    TOKEN(OPERATOR);
}
*/

<ST_IN_SCRIPTING>"{" {
    PUSH_STATE(ST_IN_SCRIPTING);
    TOKEN(PUNCTUATION);
}

<ST_IN_SCRIPTING>"}" {
//     if (darray_length(&mydata->state_stack)) {
        POP_STATE();
//     }
    if (STATE(ST_IN_SCRIPTING) == YYSTATE) {
        TOKEN(PUNCTUATION);
    } else {
        TOKEN(SEQUENCE_INTERPOLATED);
    }
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"${" {
    PUSH_STATE(ST_LOOKING_FOR_VARNAME);
    TOKEN(SEQUENCE_INTERPOLATED);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"{$" {
    PUSH_STATE(ST_IN_SCRIPTING);
    yyless(1);
    TOKEN(SEQUENCE_INTERPOLATED); // token is shorten to '{'
}

<ST_LOOKING_FOR_VARNAME>LABEL [[}] {
    yyless(YYLENG - 1);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    TOKEN(NAME_VARIABLE);
}

<ST_LOOKING_FOR_VARNAME>ANY_CHAR {
    yyless(0);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    goto restart;
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$" LABEL "->" [a-zA-Z_\x7f-\xff] {
    yyless(YYLENG - 3);
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    TOKEN(NAME_VARIABLE);
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$" LABEL "[" {
    yyless(YYLENG - 1);
    PUSH_STATE(ST_VAR_OFFSET);
    TOKEN(NAME_VARIABLE);
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE,ST_VAR_OFFSET>"$" LABEL {
    TOKEN(NAME_VARIABLE);
}

<ST_VAR_OFFSET>"]" {
    POP_STATE();
    TOKEN(PUNCTUATION);
}

<ST_VAR_OFFSET>[0] | [1-9][0-9]* {
    TOKEN(NUMBER_DECIMAL);
}

<ST_VAR_OFFSET>LNUM | HNUM | BNUM {
    /* Offset must be treated as a string */
    TOKEN(STRING_SINGLE);
}

<ST_VAR_OFFSET>"[" {
    TOKEN(PUNCTUATION);
}

<ST_VAR_OFFSET>LABEL {
    TOKEN(NAME_VARIABLE); // TODO: it's a constant
}

<ST_IN_SCRIPTING>[,;()] {
    TOKEN(PUNCTUATION);
}

<ST_IN_SCRIPTING,ST_LOOKING_FOR_PROPERTY>WHITESPACE+ {
    TOKEN(IGNORABLE);
}

<ST_IN_SCRIPTING>"/*" | "/**" WHITESPACE {
#if 0
    BEGIN(ST_COMMENT_MULTI);
    TOKEN(COMMENT_MULTILINE);
#else
    YYCTYPE *end;

    if (YYCURSOR > YYLIMIT) {
        return DONE;
    }
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "*/", STR_LEN("*/"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + STR_LEN("*/") + 1;
    }
    DELEGATE_UNTIL(COMMENT_MULTILINE);
#endif
}

<ST_COMMENT_MULTI>"*/" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<ST_IN_SCRIPTING> 'new' {
    data->next_label = CLASS;
    TOKEN(KEYWORD);
}

/*
<ST_IN_SCRIPTING> 'class' | 'interface' {
    data->next_label = CLASS;
    TOKEN(KEYWORD);
}

<ST_IN_SCRIPTING> 'function' {
    data->next_label = FUNCTION;
    TOKEN(KEYWORD);
}
*/

/*
<ST_IN_SCRIPTING> LABEL WHITESPACE* '(' {
    size_t i;

    // TODO: rempiler les ( + espaces
    // TODO: marquer si on a vu -> ou :: (on ne chercherait plus une fonction mais une méthode)
    for (i = 0; i < ARRAY_SIZE(functions); i++) {
        if (0 == ascii_strcasecmp_l(functions[i].name, functions[i].name_len, YYTEXT, YYLENG)) {
            TOKEN(NAME_FUNCTION);
        }
    }

    TOKEN(IGNORABLE);
//     TOKEN(NAME_FUNCTION);
}
*/

// TODO: handle '{' ... '}'
<ST_IN_SCRIPTING> 'namespace' {
    data->next_label = NAMESPACE;
    mydata->in_namespace = 1;
    TOKEN(KEYWORD_NAMESPACE);
}

// TODO: at BOL "use" is related to namespace else to closure
<ST_IN_SCRIPTING> 'use' {
    data->next_label = NAMESPACE;
    TOKEN(KEYWORD_NAMESPACE);
}

<ST_IN_SCRIPTING> LABEL | NAMESPACED_LABEL {
    int type;

    type = data->next_label;
    data->next_label = 0;
    switch (type) {
        case NAMESPACE:
        {
            TOKEN(NAME_NAMESPACE);
        }
        case FUNCTION:
        {
            TOKEN(NAME_FUNCTION);
        }
        case CLASS:
        {
#if 0
            // TODO: prendre en compte le namespace
            for (i = 0; i < ARRAY_SIZE(classes); i++) {
                if (0 == ascii_strcasecmp_l(classes[i].name, classes[i].name_len, YYTEXT, YYLENG)) {
                    // link it
                }
            }
#endif
            TOKEN(NAME_CLASS);
        }
        default:
        {
            size_t i;
            int token;

            token = IGNORABLE;
            for (i = 0; i < ARRAY_SIZE(keywords); i++) {
                if (0 == ascii_strcasecmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
                    if (case_insentive[keywords[i].type] || 0 == strcmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
                        token = keywords[i].type;
                    }
                    break;
                }
            }
#if 0
            // il faudrait pouvoir regarder en avant pour distinguer fonction / constante / classe
            // par contre, si on marque le passage d'opérateurs objet (:: et ->) on devrait savoir que c'est un nom de méthode
            for (i = 0; i < ARRAY_SIZE(functions); i++) {
                if (0 == ascii_strcasecmp_l(functions[i].name, functions[i].name_len, YYTEXT, YYLENG)) {
                    TOKEN(NAME_FUNCTION);
                }
            }
#endif
            TOKEN(token);
        }
    }
}

<ST_IN_SCRIPTING>LNUM {
    if ('0' == *YYTEXT) {
        TOKEN(NUMBER_OCTAL);
    } else {
        TOKEN(NUMBER_DECIMAL);
    }
}

<ST_IN_SCRIPTING>HNUM {
    TOKEN(NUMBER_HEXADECIMAL);
}

<ST_IN_SCRIPTING>BNUM {
    TOKEN(NUMBER_BINARY);
}

<ST_IN_SCRIPTING>DNUM | EXPONENT_DNUM {
    TOKEN(NUMBER_FLOAT);
}

<ST_IN_SCRIPTING>'</script' WHITESPACE* ">" NEWLINE? {
    if (mydata->version < 7) {
        BEGIN(INITIAL);
        TOKEN(NAME_TAG);
    } else {
        TOKEN(IGNORABLE);
    }
}

<ST_IN_SCRIPTING>"?>" NEWLINE? {
    BEGIN(INITIAL);
    TOKEN(NAME_TAG);
}

<ST_IN_SCRIPTING>"%>" NEWLINE? {
    if (mydata->version < 7 && mydata->asp_tags) {
        yyless(STR_LEN("%>"));
        BEGIN(INITIAL);
        TOKEN(NAME_TAG);
    } else {
        TOKEN(IGNORABLE);
    }
}

<ST_IN_SCRIPTING>"`" {
    BEGIN(ST_BACKQUOTE);
    TOKEN(STRING_BACKTICK);
}

<ST_IN_SCRIPTING>"b"? "'" {
    BEGIN(ST_SINGLE_QUOTES);
    TOKEN(STRING_SINGLE);
}

<ST_IN_SCRIPTING>"b"? "\"" {
    BEGIN(ST_DOUBLE_QUOTES);
    TOKEN(STRING_DOUBLE);
}

// TODO: split "b"? "<<<" [ \t]* (["']?) LABEL \1 NEWLINE into separate tokens ?
<ST_IN_SCRIPTING>"b"? "<<<" TABS_AND_SPACES (LABEL | (["] LABEL ["]) | ((['] LABEL [']))) NEWLINE {
    YYCTYPE *p;
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
    // optimisation for empty string, avoid an useless pass into <ST_HEREDOC,ST_NOWDOC>ANY_CHAR ?
    if (mydata->doclabel_len < SIZE_T(YYLIMIT - YYCURSOR) && !memcmp(YYCURSOR, p, mydata->doclabel_len)) {
        YYCTYPE *end = YYCURSOR + mydata->doclabel_len;

        if (*end == ';') {
            end++;
        }

        if ('\n' == *end || '\r' == *end) {
            if (STATE(ST_NOWDOC) == YYSTATE) {
                BEGIN(ST_END_NOWDOC);
            } else {
                BEGIN(ST_END_HEREDOC);
            }
        }
    }

    TOKEN(default_token_type[YYSTATE]);
}

<ST_HEREDOC,ST_NOWDOC>ANY_CHAR {
//     YYCTYPE c;

    if (YYCURSOR > YYLIMIT) {
        return 0;
    }
    YYCURSOR--;
    while (
#if 0
        // workaround for continue
        (STATE(ST_HEREDOC) == YYSTATE || STATE(ST_NOWDOC) == YYSTATE) &&
#endif
        YYCURSOR < YYLIMIT) {
        switch (/*c = */*YYCURSOR++) {
            case '\r':
                if (*YYCURSOR == '\n') {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                /* Check for ending label on the next line */
                if (IS_LABEL_START(*YYCURSOR) && mydata->doclabel_len < SIZE_T(YYLIMIT - YYCURSOR) && !memcmp(YYCURSOR, mydata->doclabel, mydata->doclabel_len)) {
                    YYCTYPE *end = YYCURSOR + mydata->doclabel_len;

                    if (*end == ';') {
                        end++;
                    }

                    if ('\n' == *end || '\r' == *end) {
                        if (STATE(ST_NOWDOC) == YYSTATE) {
                            BEGIN(ST_END_NOWDOC);
                        } else {
                            BEGIN(ST_END_HEREDOC);
                        }
//                         goto nowdoc_scan_done;
                        TOKEN(default_token_type[YYSTATE]); // conflict with continue; and main loop
                        break;
                    }
                }
                if (STATE(ST_HEREDOC) == YYSTATE) {
                    continue;
                }
                /* fall through */
            case '$':
                if (STATE(ST_HEREDOC) == YYSTATE && (IS_LABEL_START(*YYCURSOR) || '{' == *YYCURSOR)) {
                    break;
                }
                continue;
            case '{':
                if (STATE(ST_HEREDOC) == YYSTATE && *YYCURSOR == '$') {
                    break;
                }
                continue;
            case '\\':
                if (STATE(ST_HEREDOC) == YYSTATE && YYCURSOR < YYLIMIT && '\n' != *YYCURSOR && '\r' != *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            default:
                continue;
        }
        if (STATE(ST_HEREDOC) == YYSTATE) {
            YYCURSOR--;
            break;
        }
    }

// nowdoc_scan_done:
    TOKEN(default_token_type[YYSTATE]);
}

<ST_END_HEREDOC,ST_END_NOWDOC>ANY_CHAR {
    int old_state;

    old_state = YYSTATE;
    YYCURSOR += mydata->doclabel_len - 1;
    free(mydata->doclabel);
    mydata->doclabel = NULL;
    mydata->doclabel_len = 0;

    BEGIN(ST_IN_SCRIPTING);
    TOKEN(default_token_type[old_state]);
}

<ST_DOUBLE_QUOTES>"\\u{" [0-9a-fA-F]+ "}" {
    if (mydata->version >= 7) {
        TOKEN(ESCAPED_CHAR);
    } else {
        TOKEN(STRING_DOUBLE);
    }
}

<ST_DOUBLE_QUOTES>("\\0"[0-9]{2}) | ("\\" 'x' [0-9A-Fa-f]{2}) | ("\\"[$"efrntv\\]) {
    TOKEN(ESCAPED_CHAR);
}

<ST_DOUBLE_QUOTES>"\"" {
    BEGIN(ST_IN_SCRIPTING);
    TOKEN(STRING_DOUBLE);
}

<ST_IN_SCRIPTING> "$" LABEL {
    TOKEN(NAME_VARIABLE);
}

<ST_BACKQUOTE>"\\" [\\`] {
    TOKEN(ESCAPED_CHAR);
}

<ST_BACKQUOTE>"`" {
    BEGIN(ST_IN_SCRIPTING);
    TOKEN(STRING_BACKTICK);
}

<ST_SINGLE_QUOTES>"\\" [\\'] {
    TOKEN(ESCAPED_CHAR);
}

<ST_SINGLE_QUOTES>"'" {
    BEGIN(ST_IN_SCRIPTING);
    TOKEN(STRING_SINGLE);
}

<INITIAL>ANY_CHAR {
#define STRNCASECMP(s) \
    ascii_strncasecmp_l(s, STR_LEN(s), (char *) ptr, YYLIMIT - ptr, STR_LEN(s))
not_php:
    if (YYCURSOR > YYLIMIT) {
        return DONE;
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
                    if ((mydata->version < 7 && mydata->short_tags) || 0 == STRNCASECMP("php") || ('=' == *(YYCURSOR + 1))) { /* Assume [ \t\n\r] follows "php" */
                        break;
                    }
                    continue;
                case '%':
                    if (mydata->version < 7 && mydata->asp_tags) {
                        break;
                    }
                    continue;
                case 's':
                case 'S':
                    if (mydata->version < 7) {
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

<*>ANY_CHAR { // should be the last "rule"
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation php_lexer = {
    "PHP",
    0,
    "For PHP source code",
    (const char * const []) { "php3", "php4", "php5", NULL },
    (const char * const []) { "*.php", "*.php[345]", "*.inc", NULL },
    (const char * const []) { "text/x-php", "application/x-httpd-php", NULL },
    (const char * const []) { "php", "php-cli", "php5*", NULL },
    NULL,
    phpanalyse,
    phplex,
    sizeof(PHPLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { "start_inline", OPT_TYPE_BOOL,  offsetof(LexerData, state),         OPT_DEF_BOOL(0), "if true the lexer starts highlighting with php code (ie no starting `<?php`/`<?`/`<script language=\"php\">` is required at top)" },
        { "version",      OPT_TYPE_INT,   offsetof(PHPLexerData, version),    OPT_DEF_INT(5),  "TODO" },
        { "asp_tags",     OPT_TYPE_BOOL,  offsetof(PHPLexerData, asp_tags),   OPT_DEF_BOOL(0), "support, or not, `<%`/`%>` tags to begin/end PHP code ([asp_tags](http://php.net/asp_tags)) (only if version < 7)" },
        { "short_tags",   OPT_TYPE_BOOL,  offsetof(PHPLexerData, short_tags), OPT_DEF_BOOL(1), "support, or not, `<?` tags to begin PHP code ([short_open_tag](http://php.net/short_open_tag))" },
        { "secondary",    OPT_TYPE_LEXER, offsetof(PHPLexerData, secondary),  OPT_DEF_LEXER,   "Lexer to highlight content outside of PHP tags (if none, these parts will not be highlighted)" },
        END_OF_LEXER_OPTIONS
    },
    NULL
};
