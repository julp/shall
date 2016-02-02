/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2016 Zend Technologies Ltd. (http://www.zend.com) |
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
    // TODO: look anywhere into the string?
    if (src_len >= STR_LEN("<?XXX") && (0 == ascii_memcasecmp(src, "<?php", STR_LEN("<?php")) || (0 == memcmp(src, "<?", STR_LEN("<?")) && 0 != memcmp(src, "<?xml", STR_LEN("<?xml"))))) {
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
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(ST_IN_SCRIPTING) ] = IGNORABLE,
    [ STATE(ST_COMMENT_MULTI) ] = COMMENT_MULTILINE,
    [ STATE(ST_BACKQUOTE) ] = STRING_BACKTICK,
    [ STATE(ST_SINGLE_QUOTES) ] = STRING_SINGLE,
    [ STATE(ST_DOUBLE_QUOTES) ] = STRING_DOUBLE,
    [ STATE(ST_NOWDOC) ] = STRING_SINGLE,
    [ STATE(ST_HEREDOC) ] = STRING_DOUBLE,
    [ STATE(ST_VAR_OFFSET) ] = IGNORABLE,
    [ STATE(ST_LOOKING_FOR_VARNAME) ] = IGNORABLE,
    [ STATE(ST_LOOKING_FOR_PROPERTY) ] = IGNORABLE,
};

// http://lxr.php.net/xref/phpng/Zend/zend_language_scanner.l

#define DECL_OP(s, t) \
    { OPERATOR, t, s, STR_LEN(s) },
#define DECL_KW(s, t) \
    { KEYWORD, t, s, STR_LEN(s) },
#define DECL_KW_NS(s, t) \
    { KEYWORD_NAMESPACE, t, s, STR_LEN(s) },
#define DECL_KW_DECL(s, t) \
    { KEYWORD_DECLARATION, t, s, STR_LEN(s) },
#define DECL_KW_CONSTANT(s, t) \
    { KEYWORD_CONSTANT, t, s, STR_LEN(s) },
#define DECL_NAME_B(s, t) \
    { NAME_BUILTIN, t, s, STR_LEN(s) },
#define DECL_NAME_BP(s, t) \
    { NAME_BUILTIN_PSEUDO, t, s, STR_LEN(s) },

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

// regrouper: les opérateurs (par groupe sur nb d'opérandes ?), casts, constantes "magiques", (include|require)(_once)?, etc
enum {
    END = 0, // end of file
    T_LNUMBER = 256, // integer number (T_LNUMBER)
    T_DNUMBER, // floating-point number (T_DNUMBER)
    T_STRING, // identifier (T_STRING)
    T_VARIABLE, // variable (T_VARIABLE)
    T_INLINE_HTML, // text outside PHP tags
    T_ENCAPSED_AND_WHITESPACE, // quoted-string and whitespace (T_ENCAPSED_AND_WHITESPACE)
    T_CONSTANT_ENCAPSED_STRING, // quoted-string (T_CONSTANT_ENCAPSED_STRING)
    T_STRING_VARNAME, // variable name (T_STRING_VARNAME)
    T_NUM_STRING, // number (T_NUM_STRING)
    T_INCLUDE, // include (T_INCLUDE)
    T_INCLUDE_ONCE, // include_once (T_INCLUDE_ONCE)
    T_EVAL, // eval (T_EVAL)
    T_REQUIRE, // require (T_REQUIRE)
    T_REQUIRE_ONCE, // require_once (T_REQUIRE_ONCE)
    T_LOGICAL_OR, // or (T_LOGICAL_OR)
    T_LOGICAL_XOR, // xor (T_LOGICAL_XOR)
    T_LOGICAL_AND, // and (T_LOGICAL_AND)
    T_PRINT, // print (T_PRINT)
    T_YIELD, // yield (T_YIELD)
    T_YIELD_FROM, // yield from (T_YIELD_FROM)
    T_PLUS_EQUAL, // += (T_PLUS_EQUAL)
    T_MINUS_EQUAL, // -= (T_MINUS_EQUAL)
    T_MUL_EQUAL, // *= (T_MUL_EQUAL)
    T_DIV_EQUAL, // /= (T_DIV_EQUAL)
    T_CONCAT_EQUAL, // .= (T_CONCAT_EQUAL)
    T_MOD_EQUAL, // %= (T_MOD_EQUAL)
    T_AND_EQUAL, // &= (T_AND_EQUAL)
    T_OR_EQUAL, // |= (T_OR_EQUAL)
    T_XOR_EQUAL, // ^= (T_XOR_EQUAL)
    T_SL_EQUAL, // <<= (T_SL_EQUAL)
    T_SR_EQUAL, // >>= (T_SR_EQUAL)
    T_BOOLEAN_OR, // || (T_BOOLEAN_OR)
    T_BOOLEAN_AND, // && (T_BOOLEAN_AND)
    T_IS_EQUAL, // == (T_IS_EQUAL)
    T_IS_NOT_EQUAL, // != (T_IS_NOT_EQUAL)
    T_IS_IDENTICAL, // === (T_IS_IDENTICAL)
    T_IS_NOT_IDENTICAL, // !== (T_IS_NOT_IDENTICAL)
    T_IS_SMALLER_OR_EQUAL, // <= (T_IS_SMALLER_OR_EQUAL)
    T_IS_GREATER_OR_EQUAL, // >= (T_IS_GREATER_OR_EQUAL)
    T_SPACESHIP, // <=> (T_SPACESHIP)
    T_SL, // << (T_SL)
    T_SR, // >> (T_SR)
    T_INSTANCEOF, // instanceof (T_INSTANCEOF)
    T_INC, // ++ (T_INC)
    T_DEC, // -- (T_DEC)
    T_INT_CAST, // (int) (T_INT_CAST)
    T_DOUBLE_CAST, // (double) (T_DOUBLE_CAST)
    T_STRING_CAST, // (string) (T_STRING_CAST)
    T_ARRAY_CAST, // (array) (T_ARRAY_CAST)
    T_OBJECT_CAST, // (object) (T_OBJECT_CAST)
    T_BOOL_CAST, // (bool) (T_BOOL_CAST)
    T_UNSET_CAST, // (unset) (T_UNSET_CAST)
    T_NEW, // new (T_NEW)
    T_CLONE, // clone (T_CLONE)
    T_EXIT, // exit (T_EXIT)
    T_IF, // if (T_IF)
    T_ELSEIF, // elseif (T_ELSEIF)
    T_ELSE, // else (T_ELSE)
    T_ENDIF, // endif (T_ENDIF)
    T_ECHO, // echo (T_ECHO)
    T_DO, // do (T_DO)
    T_WHILE, // while (T_WHILE)
    T_ENDWHILE, // endwhile (T_ENDWHILE)
    T_FOR, // for (T_FOR)
    T_ENDFOR, // endfor (T_ENDFOR)
    T_FOREACH, // foreach (T_FOREACH)
    T_ENDFOREACH, // endforeach (T_ENDFOREACH)
    T_DECLARE, // declare (T_DECLARE)
    T_ENDDECLARE, // enddeclare (T_ENDDECLARE)
    T_AS, // as (T_AS)
    T_SWITCH, // switch (T_SWITCH)
    T_ENDSWITCH, // endswitch (T_ENDSWITCH)
    T_CASE, // case (T_CASE)
    T_DEFAULT, // default (T_DEFAULT)
    T_BREAK, // break (T_BREAK)
    T_CONTINUE, // continue (T_CONTINUE)
    T_GOTO, // goto (T_GOTO)
    T_FUNCTION, // function (T_FUNCTION)
    T_CONST, // const (T_CONST)
    T_RETURN, // return (T_RETURN)
    T_TRY, // try (T_TRY)
    T_CATCH, // catch (T_CATCH)
    T_FINALLY, // finally (T_FINALLY)
    T_THROW, // throw (T_THROW)
    T_USE, // use (T_USE)
    T_INSTEADOF, // insteadof (T_INSTEADOF)
    T_GLOBAL, // global (T_GLOBAL)
    T_STATIC, // static (T_STATIC)
    T_ABSTRACT, // abstract (T_ABSTRACT)
    T_FINAL, // final (T_FINAL)
    T_PRIVATE, // private (T_PRIVATE)
    T_PROTECTED, // protected (T_PROTECTED)
    T_PUBLIC, // public (T_PUBLIC)
    T_VAR, // var (T_VAR)
    T_UNSET, // unset (T_UNSET)
    T_ISSET, // isset (T_ISSET)
    T_EMPTY, // empty (T_EMPTY)
    T_HALT_COMPILER, // __halt_compiler (T_HALT_COMPILER)
    T_CLASS, // class (T_CLASS)
    T_TRAIT, // trait (T_TRAIT)
    T_INTERFACE, // interface (T_INTERFACE)
    T_EXTENDS, // extends (T_EXTENDS)
    T_IMPLEMENTS, // implements (T_IMPLEMENTS)
    T_OBJECT_OPERATOR, // -> (T_OBJECT_OPERATOR)
    T_DOUBLE_ARROW, // => (T_DOUBLE_ARROW)
    T_LIST, // list (T_LIST)
    T_ARRAY, // array (T_ARRAY)
    T_CALLABLE, // callable (T_CALLABLE)
    T_LINE, // __LINE__ (T_LINE)
    T_FILE, // __FILE__ (T_FILE)
    T_DIR, // __DIR__ (T_DIR)
    T_CLASS_C, // __CLASS__ (T_CLASS_C)
    T_TRAIT_C, // __TRAIT__ (T_TRAIT_C)
    T_METHOD_C, // __METHOD__ (T_METHOD_C)
    T_FUNC_C, // __FUNCTION__ (T_FUNC_C)
    T_COMMENT, // comment (T_COMMENT)
    T_DOC_COMMENT, // doc comment (T_DOC_COMMENT)
    T_OPEN_TAG, // open tag (T_OPEN_TAG)
    T_OPEN_TAG_WITH_ECHO, // open tag with echo (T_OPEN_TAG_WITH_ECHO)
    T_CLOSE_TAG, // close tag (T_CLOSE_TAG)
    T_WHITESPACE, // whitespace (T_WHITESPACE)
    T_START_HEREDOC, // heredoc start (T_START_HEREDOC)
    T_END_HEREDOC, // heredoc end (T_END_HEREDOC)
    T_DOLLAR_OPEN_CURLY_BRACES, // ${ (T_DOLLAR_OPEN_CURLY_BRACES)
    T_CURLY_OPEN, // {$ (T_CURLY_OPEN)
    T_PAAMAYIM_NEKUDOTAYIM, // :: (T_PAAMAYIM_NEKUDOTAYIM)
    T_NAMESPACE, // namespace (T_NAMESPACE)
    T_NS_C, // __NAMESPACE__ (T_NS_C)
    T_NS_SEPARATOR, // \\ (T_NS_SEPARATOR)
    T_ELLIPSIS, // ... (T_ELLIPSIS)
    T_COALESCE, // ?? (T_COALESCE)
    T_POW, // ** (T_POW)
    T_POW_EQUAL, // **= (T_POW_EQUAL)
    T_ERROR, // token used to force a parse error from the lexer
};

static int default_token_value[] = {
    [ STATE(INITIAL) ] = T_INLINE_HTML,
    [ STATE(ST_IN_SCRIPTING) ] = -1,
    [ STATE(ST_COMMENT_MULTI) ] = T_COMMENT,
    [ STATE(ST_VAR_OFFSET) ] = -1,
    [ STATE(ST_SINGLE_QUOTES) ] = T_CONSTANT_ENCAPSED_STRING,
    [ STATE(ST_NOWDOC) ] = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_DOUBLE_QUOTES) ] = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_HEREDOC) ] = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_BACKQUOTE) ] = T_ENCAPSED_AND_WHITESPACE,
    [ STATE(ST_LOOKING_FOR_VARNAME) ] = -1,
    [ STATE(ST_LOOKING_FOR_PROPERTY) ] = -1,
};

static struct {
    int type;
    int token;
    const char *name;
    size_t name_len;
} keywords[] = {
    //
    DECL_OP("and", T_LOGICAL_AND)
    DECL_OP("or", T_LOGICAL_OR)
    DECL_OP("xor", T_LOGICAL_XOR)
    //
    DECL_KW_CONSTANT("NULL", T_STRING)
    DECL_KW_CONSTANT("TRUE", T_STRING)
    DECL_KW_CONSTANT("FALSE", T_STRING)
    // http://php.net/manual/fr/reserved.constants.php
    // ...
    //
    DECL_NAME_BP("this", T_STRING)
    DECL_NAME_BP("self", T_STRING)
    DECL_NAME_BP("parent", T_STRING)
    DECL_NAME_BP("__CLASS__", T_CLASS_C)
    DECL_NAME_BP("__COMPILER_HALT_OFFSET__", T_STRING)
    DECL_NAME_BP("__DIR__", T_DIR)
    DECL_NAME_BP("__FILE__", T_FILE)
    DECL_NAME_BP("__FUNCTION__", T_FUNC_C)
    DECL_NAME_BP("__LINE__", T_LINE)
    DECL_NAME_BP("__METHOD__", T_METHOD_C)
    DECL_NAME_BP("__NAMESPACE__", T_NS_C)
    DECL_NAME_BP("__TRAIT__", T_TRAIT_C)
    //
    DECL_NAME_B("__halt_compiler", T_HALT_COMPILER)
    DECL_NAME_B("echo", T_ECHO)
    DECL_NAME_B("empty", T_EMPTY)
    DECL_NAME_B("eval", T_EVAL)
    DECL_NAME_B("exit", T_EXIT)
    DECL_NAME_B("die", T_EXIT)
    DECL_NAME_B("include", T_INCLUDE)
    DECL_NAME_B("include_once", T_INCLUDE_ONCE)
    DECL_NAME_B("isset", T_ISSET)
    DECL_NAME_B("list", T_LIST)
    DECL_NAME_B("print", T_PRINT)
    DECL_NAME_B("require", T_REQUIRE)
    DECL_NAME_B("require_once", T_REQUIRE_ONCE)
    DECL_NAME_B("unset", T_UNSET)
    //
    DECL_KW_NS("namespace", T_NAMESPACE)
    DECL_KW("use", T_USE) // is used for namespaces and by closures
    //
    DECL_KW_DECL("var", T_VAR)
    DECL_KW("abstract", T_ABSTRACT)
    DECL_KW("final", T_FINAL)
    DECL_KW("private", T_PRIVATE)
    DECL_KW("protected", T_PROTECTED)
    DECL_KW("public", T_PUBLIC)
    DECL_KW("static", T_STATIC)
    DECL_KW("extends", T_EXTENDS)
    DECL_KW("implements", T_IMPLEMENTS)
    //
    DECL_KW("callable", T_CALLABLE)
    DECL_KW("insteadof", T_INSTEADOF)
    DECL_KW("new", T_NEW)
    DECL_KW("clone", T_CLONE)
    DECL_KW("class", T_CLASS)
    DECL_KW("trait", T_TRAIT)
    DECL_KW("interface", T_INTERFACE)
    DECL_KW("function", T_FUNCTION)
    DECL_KW("global", T_GLOBAL)
    DECL_KW("const", T_CONST)
    DECL_KW("return", T_RETURN)
    DECL_KW("yield", T_YIELD)
    DECL_KW("try", T_TRY)
    DECL_KW("catch", T_CATCH)
    DECL_KW("finally", T_FINALLY)
    DECL_KW("throw", T_THROW)
    DECL_KW("if", T_IF)
    DECL_KW("else", T_ELSE)
    DECL_KW("elseif", T_ELSEIF)
    DECL_KW("endif", T_ENDIF)
    DECL_KW("while", T_WHILE)
    DECL_KW("endwhile", T_ENDWHILE)
    DECL_KW("do", T_DO)
    DECL_KW("for", T_FOR)
    DECL_KW("endfor", T_ENDFOR)
    DECL_KW("foreach", T_FOREACH)
    DECL_KW("endforeach", T_ENDFOREACH)
    DECL_KW("declare", T_DECLARE)
    DECL_KW("enddeclare", T_ENDDECLARE)
    DECL_KW("instanceof", T_INSTANCEOF)
    DECL_KW("as", T_AS)
    DECL_KW("switch", T_SWITCH)
    DECL_KW("endswitch", T_ENDSWITCH)
    DECL_KW("case", T_CASE)
    DECL_KW("default", T_DEFAULT)
    DECL_KW("break", T_BREAK)
    DECL_KW("continue", T_CONTINUE)
    DECL_KW("goto", T_GOTO)
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
LABEL =  [a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*;
NAMESPACED_LABEL = LABEL? ("\\" LABEL)+;
WHITESPACE = [ \n\r\t]+;
TABS_AND_SPACES = [ \t]*;
TOKENS = [;:,.\[\]()|^&+-/*=%!~$<>?@];
ANY_CHAR = [^];
NEWLINE = ("\r"|"\n"|"\r\n");

<INITIAL>'<?php' ([ \t]|NEWLINE) {
    yyless(STR_LEN("<?php"));
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_TAG, T_OPEN_TAG);
}

<INITIAL>"<?=" {
    // NOTE: <?= does not depend on short_open_tag since PHP 5.4.0
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_TAG, T_OPEN_TAG_WITH_ECHO);
}

<INITIAL>"<?" {
    if (myoptions->short_open_tag) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_OPEN_TAG);
    } else {
        goto not_php; // if short_open_tag is off, give it to sublexer (if any)
    }
}

<INITIAL>"<%" {
    if (myoptions->version < 7 && myoptions->asp_tags) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_OPEN_TAG);
    } else {
        goto not_php; // if asp_tags is off, give it to sublexer (if any)
    }
}

<INITIAL>'<script' WHITESPACE+ 'language' WHITESPACE* "=" WHITESPACE* ('php'|'"php"'|'\'php\'') WHITESPACE* ">" {
    if (myoptions->version < 7) {
        BEGIN(ST_IN_SCRIPTING);
        VALUED_TOKEN(NAME_TAG, T_OPEN_TAG);
    } else {
        goto not_php;
    }
}

<ST_IN_SCRIPTING> "=>" {
    VALUED_TOKEN(OPERATOR, T_DOUBLE_ARROW);
}

<ST_IN_SCRIPTING>"->" {
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    VALUED_TOKEN(OPERATOR, T_OBJECT_OPERATOR);
}

<ST_IN_SCRIPTING>'yield' WHITESPACE 'from' {
    if (myoptions->version < 7) {
        yyless(STR_LEN("yield"));
    }
    VALUED_TOKEN(KEYWORD, T_YIELD_FROM);
}

<ST_LOOKING_FOR_PROPERTY>"->" {
    VALUED_TOKEN(OPERATOR, T_OBJECT_OPERATOR);
}

<ST_LOOKING_FOR_PROPERTY>LABEL {
    POP_STATE();
    VALUED_TOKEN(NAME_VARIABLE_INSTANCE, T_STRING);
}

<ST_LOOKING_FOR_PROPERTY>ANY_CHAR {
    yyless(0);
    POP_STATE();
    goto restart;
}

<ST_IN_SCRIPTING>"??" | "<=>" {
    if (myoptions->version >= 7) {
        //yyless(STR_LEN("?"));
        //yyless(STR_LEN("<="));
        yyless(YYLENG - 1);
    }
    VALUED_TOKEN(OPERATOR, T_SPACESHIP);
}

<ST_IN_SCRIPTING>"\\" {
    VALUED_TOKEN(IGNORABLE, T_NS_SEPARATOR);
}

<ST_IN_SCRIPTING> "++" | "--" | [!=]"==" | "<>" | [-+*/%.<>+&|^!=]"=" | ">>=" | "<<=" | "**=" | "<<" | ">>" | "**" | [-+.*/%=^&|!~<>?:@] {
    VALUED_TOKEN(OPERATOR, T_INC);
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
    VALUED_TOKEN(COMMENT_SINGLE, T_COMMENT);
}

<ST_IN_SCRIPTING>"(" TABS_AND_SPACES ('int' | 'integer' | 'bool' | 'boolean' | 'string' | 'binary' | 'real' | 'float' | 'double' | 'array' | 'object' | 'unset') TABS_AND_SPACES ")" {
    VALUED_TOKEN(OPERATOR, T_INT_CAST);
}

<ST_IN_SCRIPTING>"{" {
    PUSH_STATE(ST_IN_SCRIPTING);
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_IN_SCRIPTING>"}" {
//     if (darray_length(&myoptions->state_stack)) {
        POP_STATE();
//     }
    if (STATE(ST_IN_SCRIPTING) == YYSTATE) {
        VALUED_TOKEN(PUNCTUATION, *YYTEXT);
    } else {
        VALUED_TOKEN(SEQUENCE_INTERPOLATED, *YYTEXT);
    }
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"${" {
    PUSH_STATE(ST_LOOKING_FOR_VARNAME);
    VALUED_TOKEN(SEQUENCE_INTERPOLATED, T_DOLLAR_OPEN_CURLY_BRACES);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"{$" {
    PUSH_STATE(ST_IN_SCRIPTING);
    yyless(1);
    VALUED_TOKEN(SEQUENCE_INTERPOLATED, T_CURLY_OPEN); // token is shorten to '{'
}

<ST_LOOKING_FOR_VARNAME>LABEL [[}] {
    yyless(YYLENG - 1);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    VALUED_TOKEN(NAME_VARIABLE, T_STRING_VARNAME);
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
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$" LABEL "[" {
    yyless(YYLENG - 1);
    PUSH_STATE(ST_VAR_OFFSET);
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE,ST_VAR_OFFSET>"$" LABEL {
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_VAR_OFFSET>"]" {
    POP_STATE();
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_VAR_OFFSET>[0] | [1-9][0-9]* {
    VALUED_TOKEN(NUMBER_DECIMAL, T_NUM_STRING);
}

<ST_VAR_OFFSET>LNUM | HNUM | BNUM {
    /* Offset must be treated as a string */
    VALUED_TOKEN(STRING_SINGLE, T_NUM_STRING);
}

<ST_VAR_OFFSET>"[" {
    VALUED_TOKEN(PUNCTUATION, *YYTEXT); // ???
}

<ST_VAR_OFFSET>LABEL {
    VALUED_TOKEN(NAME_VARIABLE, T_STRING); // TODO: it's a constant
}

<ST_IN_SCRIPTING>[,;()] {
    VALUED_TOKEN(PUNCTUATION, *YYTEXT);
}

<ST_IN_SCRIPTING,ST_LOOKING_FOR_PROPERTY>WHITESPACE+ {
    VALUED_TOKEN(IGNORABLE, T_WHITESPACE);
}

<ST_IN_SCRIPTING>"/**" WHITESPACE {
#if 1
    mydata->in_doc_comment = true;
    BEGIN(ST_COMMENT_MULTI);
    VALUED_TOKEN(COMMENT_DOCUMENTATION, T_DOC_COMMENT);
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

<ST_IN_SCRIPTING>"/*" {
    mydata->in_doc_comment = false;
    BEGIN(ST_COMMENT_MULTI);
    VALUED_TOKEN(COMMENT_MULTILINE, T_COMMENT);
}

<ST_COMMENT_MULTI>"*/" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(mydata->in_doc_comment ? COMMENT_DOCUMENTATION : COMMENT_MULTILINE, T_COMMENT);
}

<ST_IN_SCRIPTING> LABEL | NAMESPACED_LABEL {
    size_t i;
    int type, token;

    token = 0;
    type = IGNORABLE;
    for (i = 0; i < ARRAY_SIZE(keywords); i++) {
        if (0 == ascii_strcasecmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
            if (case_insentive[keywords[i].type] || 0 == strcmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
                type = keywords[i].type;
                token = keywords[i].token;
            }
            break;
        }
    }
#if 0
    // il faudrait pouvoir regarder en avant pour distinguer fonction / constante / classe
    // par contre, si on marque le passage d'opérateurs objet (:: et ->) on devrait savoir que c'est un nom de méthode
    for (i = 0; i < ARRAY_SIZE(functions); i++) {
        if (0 == ascii_strcasecmp_l(functions[i].name, functions[i].name_len, YYTEXT, YYLENG)) {
            VALUED_TOKEN(NAME_FUNCTION);
        }
    }
#endif
    VALUED_TOKEN(type, token);
}

<ST_IN_SCRIPTING>LNUM {
    if ('0' == *YYTEXT) {
        VALUED_TOKEN(NUMBER_OCTAL, T_LNUMBER);
    } else {
        VALUED_TOKEN(NUMBER_DECIMAL, T_LNUMBER);
    }
}

<ST_IN_SCRIPTING>HNUM {
    VALUED_TOKEN(NUMBER_HEXADECIMAL, T_LNUMBER);
}

<ST_IN_SCRIPTING>BNUM {
    VALUED_TOKEN(NUMBER_BINARY, T_LNUMBER);
}

<ST_IN_SCRIPTING>DNUM | EXPONENT_DNUM {
    VALUED_TOKEN(NUMBER_FLOAT, T_DNUMBER);
}

<ST_IN_SCRIPTING>'</script' WHITESPACE* ">" NEWLINE? {
    if (myoptions->version < 7) {
        BEGIN(INITIAL);
        VALUED_TOKEN(NAME_TAG, T_CLOSE_TAG);
    } else {
        VALUED_TOKEN(IGNORABLE, T_INLINE_HTML);
    }
}

<ST_IN_SCRIPTING>"?>" NEWLINE? {
    BEGIN(INITIAL);
    VALUED_TOKEN(NAME_TAG, T_CLOSE_TAG);
}

<ST_IN_SCRIPTING>"%>" NEWLINE? {
    if (myoptions->version < 7 && myoptions->asp_tags) {
        yyless(STR_LEN("%>"));
        BEGIN(INITIAL);
        VALUED_TOKEN(NAME_TAG, T_CLOSE_TAG);
    } else {
        VALUED_TOKEN(IGNORABLE, T_INLINE_HTML);
    }
}

<ST_IN_SCRIPTING>"`" {
    BEGIN(ST_BACKQUOTE);
    VALUED_TOKEN(STRING_BACKTICK, *YYTEXT);
}

<ST_IN_SCRIPTING>"b"? "'" {
    BEGIN(ST_SINGLE_QUOTES);
    VALUED_TOKEN(STRING_SINGLE, T_CONSTANT_ENCAPSED_STRING);
}

<ST_IN_SCRIPTING>"b"? "\"" {
    BEGIN(ST_DOUBLE_QUOTES);
    VALUED_TOKEN(STRING_DOUBLE, '"');
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

    VALUED_TOKEN(default_token_type[YYSTATE], T_START_HEREDOC);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"\\u{" [0-9a-fA-F]+ "}" {
    if (myoptions->version >= 7) {
        VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
    } else {
        VALUED_TOKEN(default_token_type[YYSTATE], T_ENCAPSED_AND_WHITESPACE);
    }
}

<ST_DOUBLE_QUOTES>"\\\"" {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_BACKQUOTE>"\\`" {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>("\\0"[0-9]{2}) | ("\\" 'x' [0-9A-Fa-f]{2}) | ("\\"[$efrntv\\]) {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_HEREDOC,ST_NOWDOC>NEWLINE LABEL ";" NEWLINE {
    int old_state;
    // beginning and end of the LABEL
    // - s is on the first character of the LABEL
    // - e is on the comma (';'), after the last character of the LABEL
    YYCTYPE *s, *e;

    s = YYTEXT;
    e = YYCURSOR;
    old_state = YYSTATE;
    // skip newline before label
    if ('\r' == *s) {
        ++s;
    }
    if ('\n' == *s) {
        ++s;
    }
    --e; // YYCURSOR is on the next character after the last NEWLINE, so "decrement" it first
    // then skip newline after label (\r?\n? in reverse order)
    if ('\n' == *e) {
        --e;
    }
    if ('\r' == *e) {
        --e;
    }
    if (mydata->doclabel_len == SIZE_T(e - s) && 0 == memcmp(s, mydata->doclabel, mydata->doclabel_len)) {
        yyless((e - YYTEXT)); // replay last NEWLINE and ';' to recognize as ignorable and PUNCTUATION
        free(mydata->doclabel);
        mydata->doclabel = NULL;
        mydata->doclabel_len = 0;

        BEGIN(ST_IN_SCRIPTING);
    }
    VALUED_TOKEN(default_token_type[old_state], T_END_HEREDOC);
}

<ST_DOUBLE_QUOTES>"\"" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_DOUBLE, T_ENCAPSED_AND_WHITESPACE);
}

<ST_IN_SCRIPTING> "$" LABEL {
    VALUED_TOKEN(NAME_VARIABLE, T_VARIABLE);
}

<ST_BACKQUOTE>"\\" [\\`] {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_BACKQUOTE>"`" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_BACKTICK, *YYTEXT);
}

<ST_SINGLE_QUOTES>"\\" [\\'] {
    VALUED_TOKEN(ESCAPED_CHAR, T_ENCAPSED_AND_WHITESPACE);
}

<ST_SINGLE_QUOTES>"'" {
    BEGIN(ST_IN_SCRIPTING);
    VALUED_TOKEN(STRING_SINGLE, T_CONSTANT_ENCAPSED_STRING);
}

<INITIAL>ANY_CHAR {
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

<ST_COMMENT_MULTI>ANY_CHAR {
    VALUED_TOKEN(mydata->in_doc_comment ? COMMENT_DOCUMENTATION : COMMENT_MULTILINE, T_COMMENT);
}

<*>NEWLINE {
    int type;

    if (-1 == (type = default_token_value[YYSTATE])) {
        type = *YYTEXT;
    }
    VALUED_TOKEN(default_token_type[YYSTATE], type);
}

<*>ANY_CHAR { // should be the last "rule"
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
    (const char * const []) { "php3", "php4", "php5", NULL },
    (const char * const []) { "*.php", "*.php[345]", "*.inc", NULL },
    (const char * const []) { "text/x-php", "application/x-httpd-php", NULL },
    (const char * const []) { "php", "php-cli", "php5*", NULL },
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
    NULL // dependencies
};
