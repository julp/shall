%code requires {
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
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

/**
 * Spec/language reference:
 * - https://github.com/php/php-langspec/blob/master/spec/00-specification-for-php.md
 *
 * This file is based on PHP 7.3.8
 *
 * https://raw.githubusercontent.com/php/php-src/PHP-7.3.8/Zend/zend_language_parser.y
 *
 * Don't forget to update this reference and copyright!
 */

typedef struct {
    int attr;
    LexerReturnValue *head, *tail;
} List;
}

%{
#include <stdlib.h>

#include "lexer.h"
#undef TOKEN
#include "tokens.h"
#include "php.h"

/* Zend/zend_compile.c */
static named_element_t builtin_types[] = {
    NE("array"), // from "type" rule
    NE("bool"),
    NE("callable"), // from "type" rule
    NE("int"),
    NE("iterable"),
    NE("float"),
    NE("object"),
    NE("string"),
    NE("void"),
};

static int handle_type(List *dollar1) {
    int type;

    type = NAME_CLASS;
    if (dollar1->head == dollar1->tail) {
        named_element_t key = { (char *) dollar1->head->yystart, (int) (dollar1->head->yyend - dollar1->head->yystart) };

        if (NULL != bsearch(&key, builtin_types, ARRAY_SIZE(builtin_types), sizeof(builtin_types[0]), named_elements_casecmp)) {
            type = KEYWORD_TYPE;
        }
    }

    return type;
}

static int yyerror(const char *msg)
{
    fprintf(stderr, "[ ERR ] parsing error: %s\n", msg);

    return 0;
}

List *list_new(LexerReturnValue *element)
{
    List *list;

    list = malloc(sizeof(*list));
    list->head = list->tail = element;
    element->next = NULL;

    return list;
}

List *list_append(List *list, LexerReturnValue *element)
{
    list->tail->next = element;
    list->tail = element;
    element->next = NULL;

    return list;
}

List *list_merge(List *a, List *b)
{
    a->tail->next = b->head;
    a->tail = b->tail;
    free(b);

    return a;
}

void list_apply(List *list, int type)
{
    LexerReturnValue *el;

    for (el = list->head; NULL != el; el = el->next) {
        el->token_default_type = type;
    }
    free(list);
}
%}

%define api.pure full
%define api.push-pull push

%union {
    int type;
    List *list;
    void *dummy;
    LexerReturnValue *rv;
}

%token <rv> T_IGNORE "internaly used by shall, has to be the very first"

%left T_INCLUDE_REQUIRE //T_EVAL
%left ','
%left T_LOGICAL_BINARY_OP
%right T_PRINT
%right T_YIELD
%right T_DOUBLE_ARROW
%right T_YIELD_FROM
%left '=' T_EQUAL_OP
%left '?' ':'
// %right T_COALESCE
// %left T_BOOLEAN_OR
// %left T_BOOLEAN_AND
%left '|'
%left '^'
%left '&'
// %nonassoc T_IS_EQUAL T_IS_NOT_EQUAL T_IS_IDENTICAL T_IS_NOT_IDENTICAL T_SPACESHIP T_IS_SMALLER_OR_EQUAL T_IS_GREATER_OR_EQUAL
%nonassoc '<' '>'
// %left T_SL T_SR
%left '+' '-' '.'
%left '*' '/' '%'
%right '!'
%nonassoc T_INSTANCEOF
%right '~' T_INC T_DEC T_CAST '@'
// %right T_POW
%right '['
%nonassoc T_NEW T_CLONE
%left T_NOELSE
%left T_ELSEIF
%left T_ELSE
%left T_ENDIF
%right T_STATIC T_ABSTRACT T_FINAL T_VISIBILITY

%token <rv> T_INC "++"
%token <rv> T_DEC "--"
%token <rv> T_ELLIPSIS "..."
%token <rv> T_STRING "a string"
%token <rv> T_NS_SEPARATOR "\\"
%token <rv> T_DOUBLE_ARROW "=>"
%token <rv> T_OBJECT_OPERATOR "->"
%token <rv> T_PAAMAYIM_NEKUDOTAYIM "::"
%token <rv> T_BINARY_OP "a binary operator"
%token <rv> T_EQUAL_OP "an operator with assignment (eg .=)"

%token <rv> T_LNUMBER "an integer (decimal)"
%token <rv> T_DNUMBER "a float"

%token <rv> T_ABSTRACT "abstract"
%token <rv> T_ARRAY "array"
%token <rv> T_AS "as"
%token <rv> T_BREAK "break"
%token <rv> T_CALLABLE "callable"
%token <rv> T_CASE "case"
%token <rv> T_CAST "a cast"
%token <rv> T_CATCH "catch"
%token <rv> T_CLASS "class"
%token <rv> T_CLONE "clone"
%token <rv> T_CONST "const"
%token <rv> T_CONTINUE "continue"
%token <rv> T_DECLARE "declare"
%token <rv> T_DEFAULT "default"
%token <rv> T_DO "do"
%token <rv> T_ECHO "echo"
%token <rv> T_ELSE "else"
%token <rv> T_ELSEIF "elseif"
%token <rv> T_ENDIF "endif"
%token <rv> T_ENDDECLARE "enddeclare"
%token <rv> T_ENDFOR "endfor"
%token <rv> T_ENDFOREACH "endforeach"
%token <rv> T_ENDSWITCH "endswitch"
%token <rv> T_ENDWHILE "endwhile"
%token <rv> T_EXIT "exit"
%token <rv> T_EXTENDS "extends"
%token <rv> T_FINAL "final"
%token <rv> T_FINALLY "finally"
%token <rv> T_FOR "for"
%token <rv> T_FOREACH "foreach"
%token <rv> T_FUNCTION "function"
%token <rv> T_GLOBAL "global"
%token <rv> T_GOTO "goto"
%token <rv> T_HALT_COMPILER "__halt_compiler"
%token <rv> T_IF "if"
%token <rv> T_IMPLEMENTS "implements"
%token <rv> T_INSTANCEOF "instanceof"
%token <rv> T_INSTEADOF "insteadof"
%token <rv> T_INTERFACE "interface"
%token <rv> T_ISSET "isset"
%token <rv> T_LIST "list"
%token <rv> T_NEW "new"
%token <rv> T_NAMESPACE "namespace"
%token <rv> T_PRINT "print"
%token <rv> T_RETURN "return"
%token <rv> T_STATIC "static"
%token <rv> T_SWITCH "switch"
%token <rv> T_THROW "throw"
%token <rv> T_TRAIT "trait"
%token <rv> T_TRY "try"
%token <rv> T_UNSET "unset"
%token <rv> T_USE "use"
%token <rv> T_VAR "var (PHP 4)"
%token <rv> T_WHILE "while"
%token <rv> T_YIELD "yield"
%token <rv> T_YIELD_FROM "yield from"

%token <rv> T_END_HEREDOC
%token <rv> T_START_HEREDOC

%token <rv> T_VARIABLE "a variable"
%token <rv> T_FUNCTION_LIKE_KEYWORD "empty/eval"
%token <rv> T_VISIBILITY "public/protected/private"
%token <rv> T_INCLUDE_REQUIRE "(include|require)(_once)?"
%token <rv> T_LOGICAL_BINARY_OP "and|or|xor"
%token <rv> T_INTERNAL_CONSTANT "one of __(CLASS|TRAIT|FUNCTION|METHOD|LINE|FILE|DIR|NAMESPACE)__"

%token <rv> T_NUM_STRING
%token <rv> T_DOLLAR_OPEN_CURLY_BRACES
%token <rv> T_STRING_VARNAME
%token <rv> T_CURLY_OPEN
%token <rv> T_ENCAPSED_AND_WHITESPACE
%token <rv> T_CONSTANT_ENCAPSED_STRING

%type <rv> identifier semi_reserved reserved_non_modifiers property_name
%type <type> use_type
%type <list> use_declaration use_declarations unprefixed_use_declarations group_use_declaration unprefixed_use_declaration name name_list namespace_name type

%start top_statement_list

%%

// TODO: regrouper or/and/xor sous un token à part
reserved_non_modifiers:
        T_INCLUDE_REQUIRE | T_FUNCTION_LIKE_KEYWORD | T_LOGICAL_BINARY_OP
    |   T_INSTANCEOF | T_NEW | T_CLONE | T_EXIT | T_IF | T_ELSEIF | T_ELSE | T_ENDIF | T_ECHO | T_DO | T_WHILE | T_ENDWHILE
    |   T_FOR | T_ENDFOR | T_FOREACH | T_ENDFOREACH | T_DECLARE | T_ENDDECLARE | T_AS | T_TRY | T_CATCH | T_FINALLY
    |   T_THROW | T_USE | T_INSTEADOF | T_GLOBAL | T_VAR | T_UNSET | T_ISSET | T_CONTINUE | T_GOTO
    |   T_FUNCTION | T_CONST | T_RETURN | T_PRINT | T_YIELD | T_LIST | T_SWITCH | T_ENDSWITCH | T_CASE | T_DEFAULT | T_BREAK
    |   T_ARRAY | T_CALLABLE | T_EXTENDS | T_IMPLEMENTS | T_NAMESPACE | T_TRAIT | T_INTERFACE | T_CLASS
    |   T_INTERNAL_CONSTANT
;

semi_reserved:
        reserved_non_modifiers
    |   T_STATIC | T_ABSTRACT | T_FINAL | T_VISIBILITY
;

identifier:
        T_STRING      { $$->token_default_type = NAME_CONSTANT; }
    |   semi_reserved { $$->token_default_type = NAME_CONSTANT; }
;

top_statement_list:
        top_statement_list top_statement
    |   %empty
;

namespace_name:
        T_STRING                               { $$ = list_new($1); }
    |   namespace_name T_NS_SEPARATOR T_STRING { $$ = list_append(list_append($1, $2), $3); }
;

name:
        namespace_name                            { $$ = $1; }
    |   T_NAMESPACE T_NS_SEPARATOR namespace_name { $$ = list_append($3, $2); }
    |   T_NS_SEPARATOR namespace_name             { $$ = list_append($2, $1); }
;

// TODO: instruct for a flush of the tokens here?
top_statement:
        statement
    |   function_declaration_statement
    |   class_declaration_statement
    |   trait_declaration_statement
    |   interface_declaration_statement
    |   T_HALT_COMPILER '(' ')' ';'
    |   T_NAMESPACE namespace_name ';'
    |   T_NAMESPACE namespace_name '{' top_statement_list '}'
    |   T_NAMESPACE '{' top_statement_list '}'
    |   T_USE mixed_group_use_declaration ';'    { /* possible mix of classes/functions/constants */}
    |   T_USE use_type group_use_declaration ';' { /* functions or constants, depends on $2 */ list_apply($3, $2); }
    |   T_USE use_declarations ';'               { /* classes */ list_apply($2, NAME_CLASS); }
    |   T_USE use_type use_declarations ';'      { /* functions or constants, depends on $2 */ list_apply($3, $2); }
    |   T_CONST const_list ';'                   {}
;

use_type:
        T_FUNCTION { $$ = NAME_FUNCTION; }
    |   T_CONST    { $$ = NAME_CONSTANT; }
;

group_use_declaration:
        namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations '}'                { $$ = $4; list_apply($1, NAME_NAMESPACE); $2->token_default_type = NAME_NAMESPACE; }
    |   T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations '}' { $$ = $5; list_apply($2, NAME_NAMESPACE); $1->token_default_type = $3->token_default_type = NAME_NAMESPACE; }
;

mixed_group_use_declaration:
        namespace_name T_NS_SEPARATOR '{' inline_use_declarations '}'                { list_apply($1, NAME_NAMESPACE); $2->token_default_type = NAME_NAMESPACE; }
    |   T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' inline_use_declarations '}' { list_apply($2, NAME_NAMESPACE); $1->token_default_type = $3->token_default_type = NAME_NAMESPACE; }
;

possible_comma:
        %empty
    |   ','
;

inline_use_declarations:
        inline_use_declarations ',' inline_use_declaration { /* NOP */ }
    |   inline_use_declaration                             { /* NOP */ }
;

unprefixed_use_declarations:
        unprefixed_use_declarations ',' unprefixed_use_declaration { $$ = list_merge($1, $3); }
    |   unprefixed_use_declaration                                 { $$ = $1; }
;

use_declarations:
        use_declarations ',' use_declaration { $$ = list_merge($1, $3); }
    |   use_declaration                      { $$ = $1; }
;

inline_use_declaration:
        unprefixed_use_declaration          { /* a class */ list_apply($1, NAME_CLASS); }
    |   use_type unprefixed_use_declaration { /* a function or constant, depends on $1 */ list_apply($2, $1); }
;

unprefixed_use_declaration:
        namespace_name               { $$ = $1; }
    |   namespace_name T_AS T_STRING { $$ = list_append($1, $3); }
;

use_declaration:
        unprefixed_use_declaration                { $$ = $1; }
    |   T_NS_SEPARATOR unprefixed_use_declaration { $$ = list_append($2, $1); }
;

const_list:
        const_list ',' const_decl
    |   const_decl
;

inner_statement_list:
        inner_statement_list inner_statement
    |   %empty
;


inner_statement:
        statement
    |   function_declaration_statement
    |   class_declaration_statement
    |   trait_declaration_statement
    |   interface_declaration_statement
    |   T_HALT_COMPILER '(' ')' ';'     { YYERROR; }
;


statement:
        '{' inner_statement_list '}'
    |   if_stmt
    |   alt_if_stmt
    |   T_WHILE '(' expr ')' while_statement
    |   T_DO statement T_WHILE '(' expr ')' ';'
    |   T_FOR '(' for_exprs ';' for_exprs ';' for_exprs ')' for_statement
    |   T_SWITCH '(' expr ')' switch_case_list
    |   T_BREAK optional_expr ';'
    |   T_CONTINUE optional_expr ';'
    |   T_RETURN optional_expr ';'
    |   T_GLOBAL global_var_list ';'
    |   T_STATIC static_var_list ';'
    |   T_ECHO echo_expr_list ';'
    //|   T_INLINE_HTML
    |   expr ';'
    |   T_UNSET '(' unset_variables possible_comma ')' ';'
    |   T_FOREACH '(' expr T_AS foreach_variable ')' foreach_statement
    |   T_FOREACH '(' expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable ')' foreach_statement
    |   T_DECLARE '(' const_list ')' declare_statement
    |   ';' /* empty statement */
    |   T_TRY '{' inner_statement_list '}' catch_list finally_statement
    |   T_THROW expr ';'
    |   T_GOTO T_STRING ';'
    |   T_STRING ':'
;

catch_list:
        %empty
    |   catch_list T_CATCH '(' catch_name_list T_VARIABLE ')' '{' inner_statement_list '}'
;

catch_name_list:
        name                     { list_apply($1, NAME_CLASS_EXCEPTION); }
    |   catch_name_list '|' name { list_apply($3, NAME_CLASS_EXCEPTION); }
;

finally_statement:
        %empty
    |   T_FINALLY '{' inner_statement_list '}'
;

unset_variables:
        unset_variable
    |   unset_variables ',' unset_variable
;

unset_variable:
        variable
;

function_declaration_statement:
        function returns_ref T_STRING backup_doc_comment '(' parameter_list ')' return_type backup_fn_flags '{' inner_statement_list '}' backup_fn_flags { $3->token_default_type = NAME_FUNCTION; }
;

is_reference:
        %empty
    |   '&'
;

is_variadic:
        %empty
    |   T_ELLIPSIS
;

class_declaration_statement:
        class_modifiers T_CLASS T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}' { $3->token_default_type = NAME_CLASS; }
    |   T_CLASS T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}'                 { $2->token_default_type = NAME_CLASS; }
;

class_modifiers:
        class_modifier
    |   class_modifiers class_modifier
;

class_modifier:
        T_ABSTRACT
    |   T_FINAL
;

trait_declaration_statement:
        T_TRAIT T_STRING backup_doc_comment '{' class_statement_list '}' { $2->token_default_type = NAME_CLASS_TRAIT; }
;

interface_declaration_statement:
        T_INTERFACE T_STRING interface_extends_list backup_doc_comment '{' class_statement_list '}' { $2->token_default_type = NAME_CLASS_INTERFACE; }
;

extends_from:
        %empty
    |   T_EXTENDS name { list_apply($2, NAME_CLASS); }
;

interface_extends_list:
        %empty
    |   T_EXTENDS name_list { list_apply($2, NAME_CLASS); }
;

implements_list:
        %empty
    |   T_IMPLEMENTS name_list { list_apply($2, NAME_CLASS); }
;

foreach_variable:
        variable
    |   '&' variable
    |   T_LIST '(' array_pair_list ')'
    |   '[' array_pair_list ']'
;

for_statement:
        statement
    |   ':' inner_statement_list T_ENDFOR ';'
;

foreach_statement:
        statement
    |   ':' inner_statement_list T_ENDFOREACH ';'
;

declare_statement:
        statement
    |   ':' inner_statement_list T_ENDDECLARE ';'
;

switch_case_list:
        '{' case_list '}'
    |   '{' ';' case_list '}'
    |   ':' case_list T_ENDSWITCH ';'
    |   ':' ';' case_list T_ENDSWITCH ';'
;

case_list:
        %empty
    |   case_list T_CASE expr case_separator inner_statement_list
    |   case_list T_DEFAULT case_separator inner_statement_list
;

case_separator:
        ':'
    |   ';'
;

while_statement:
        statement
    |   ':' inner_statement_list T_ENDWHILE ';'
;


if_stmt_without_else:
        T_IF '(' expr ')' statement
    |   if_stmt_without_else T_ELSEIF '(' expr ')' statement
;

if_stmt:
        if_stmt_without_else %prec T_NOELSE
    |   if_stmt_without_else T_ELSE statement
;

alt_if_stmt_without_else:
        T_IF '(' expr ')' ':' inner_statement_list
    |   alt_if_stmt_without_else T_ELSEIF '(' expr ')' ':' inner_statement_list
;

alt_if_stmt:
        alt_if_stmt_without_else T_ENDIF ';'
    |   alt_if_stmt_without_else T_ELSE ':' inner_statement_list T_ENDIF ';'
;

parameter_list:
        non_empty_parameter_list
    |   %empty
;


non_empty_parameter_list:
        parameter
    |   non_empty_parameter_list ',' parameter
;

parameter:
        optional_type is_reference is_variadic T_VARIABLE
    |   optional_type is_reference is_variadic T_VARIABLE '=' expr
;


optional_type:
        %empty
    |   type_expr
;

type_expr:
        type     { list_apply($1, handle_type($1)); }
    |   '?' type { list_apply(list_append($2, $<rv>1), handle_type($2)); }
;

type:
        T_ARRAY    { $$ = list_new($1); }
    |   T_CALLABLE { $$ = list_new($1); }
    |   name       { $$ = $1; }
;

return_type:
        %empty
    |   ':' type_expr
;

argument_list:
        '(' ')'
    |   '(' non_empty_argument_list possible_comma ')'
;

non_empty_argument_list:
        argument
    |   non_empty_argument_list ',' argument
;

argument:
        expr
    |   T_ELLIPSIS expr
;

global_var_list:
        global_var_list ',' global_var
    |   global_var
;

global_var:
        simple_variable
;


static_var_list:
        static_var_list ',' static_var
    |   static_var
;

static_var:
        T_VARIABLE
    |   T_VARIABLE '=' expr
;

class_statement_list:
        class_statement_list class_statement
    |   %empty
;


class_statement:
        variable_modifiers property_list ';'
    |   method_modifiers T_CONST class_const_list ';'
    |   T_USE name_list trait_adaptations
    |   method_modifiers function returns_ref identifier backup_doc_comment '(' parameter_list ')' return_type backup_fn_flags method_body backup_fn_flags
;

name_list:
        name               { $$ = $1; }
    |   name_list ',' name { $$ = list_merge($1, $3); }
;

trait_adaptations:
        ';'
    |   '{' '}'
    |   '{' trait_adaptation_list '}'
;

trait_adaptation_list:
        trait_adaptation
    |   trait_adaptation_list trait_adaptation
;

trait_adaptation:
        trait_precedence ';'
    |   trait_alias ';'
;

trait_precedence:
        absolute_trait_method_reference T_INSTEADOF name_list
;

trait_alias:
        trait_method_reference T_AS T_STRING
    |   trait_method_reference T_AS reserved_non_modifiers
    |   trait_method_reference T_AS member_modifier identifier
    |   trait_method_reference T_AS member_modifier
;

trait_method_reference:
        identifier
    |   absolute_trait_method_reference
;

absolute_trait_method_reference:
        name T_PAAMAYIM_NEKUDOTAYIM identifier { /* TODO */ }
;

method_body:
        ';' /* abstract method */
    |   '{' inner_statement_list '}'
;

variable_modifiers:
        non_empty_member_modifiers
    |   T_VAR
;

method_modifiers:
        %empty
    |   non_empty_member_modifiers
;

non_empty_member_modifiers:
        member_modifier
    |   non_empty_member_modifiers member_modifier
;

member_modifier:
        T_VISIBILITY
    |   T_STATIC
    |   T_ABSTRACT
    |   T_FINAL
;

property_list:
        property_list ',' property
    |   property
;

property:
        T_VARIABLE backup_doc_comment          { $1->token_default_type = NAME_VARIABLE_INSTANCE; }
    |   T_VARIABLE '=' expr backup_doc_comment { $1->token_default_type = NAME_VARIABLE_INSTANCE; }
;

class_const_list:
        class_const_list ',' class_const_decl
    |   class_const_decl
;

class_const_decl:
    identifier '=' expr backup_doc_comment { $1->token_default_type = NAME_CLASS_CONSTANT; }
;

const_decl:
        T_STRING '=' expr backup_doc_comment { $1->token_default_type = NAME_CONSTANT; }
;

echo_expr_list:
        echo_expr_list ',' echo_expr
    |   echo_expr
;
echo_expr:
        expr
;

for_exprs:
        %empty
    |   non_empty_for_exprs
;

non_empty_for_exprs:
        non_empty_for_exprs ',' expr
    |   expr
;

anonymous_class:
        T_CLASS ctor_arguments extends_from implements_list backup_doc_comment '{' class_statement_list '}'
;

new_expr:
        T_NEW class_name_reference ctor_arguments
    |   T_NEW anonymous_class
;

expr:
        variable
    |   T_LIST '(' array_pair_list ')' '=' expr
    |   '[' array_pair_list ']' '=' expr
    |   variable '=' expr
    |   variable '=' '&' variable
    |   T_CLONE expr
    |   variable T_EQUAL_OP expr
    |   variable T_INC
    |   T_INC variable
    |   variable T_DEC
    |   T_DEC variable
    |   expr T_BINARY_OP expr
    |   '+' expr %prec T_INC
    |   '-' expr %prec T_INC
    |   '!' expr
    |   '~' expr
    |   expr T_INSTANCEOF class_name_reference
    |   '(' expr ')'
    |   new_expr
    |   expr '?' expr ':' expr
    |   expr '?' ':' expr
    |   internal_functions_in_yacc
    |   T_CAST expr
    |   T_EXIT exit_expr
    |   '@' expr
    |   scalar
    |   '`' backticks_expr '`'
    |   T_PRINT expr
    |   T_YIELD
    |   T_YIELD expr
    |   T_YIELD expr T_DOUBLE_ARROW expr
    |   T_YIELD_FROM expr
    |   function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
    |   T_STATIC function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
;

function:
        T_FUNCTION
;


backup_doc_comment:
        %empty
;

backup_fn_flags:
        %empty
;

returns_ref:
        %empty
    |   '&'
;

lexical_vars:
        %empty
    |   T_USE '(' lexical_var_list ')'
;

lexical_var_list:
        lexical_var_list ',' lexical_var
    |   lexical_var
;

lexical_var:
        T_VARIABLE
    |   '&' T_VARIABLE
;

function_call:
        name argument_list                                                   { list_apply($1, NAME_FUNCTION); }
    |   class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    |   variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    |   callable_expr argument_list
;

class_name:
        T_STATIC
    |   name     { list_apply($1, NAME_CLASS); }
;

class_name_reference:
        class_name
    |   new_variable
;

exit_expr:
        %empty
    |   '(' optional_expr ')'
;

backticks_expr:
    /* JULP: see comment of "encaps_list" rule */
        /*%empty
    |   T_ENCAPSED_AND_WHITESPACE
    |*/   encaps_list
;

ctor_arguments:
        %empty
    |   argument_list
;

dereferencable_scalar:
        T_ARRAY '(' array_pair_list ')'
    |   '[' array_pair_list ']'
    |   T_CONSTANT_ENCAPSED_STRING
;

scalar:
        T_LNUMBER
    |   T_DNUMBER
    |   T_INTERNAL_CONSTANT
    |   T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC
    |   T_START_HEREDOC T_END_HEREDOC
    |   '"' encaps_list '"'
    |   T_START_HEREDOC encaps_list T_END_HEREDOC
    |   dereferencable_scalar
    |   constant
;

constant:
        name                                                  { list_apply($1, NAME_CONSTANT); }
    |   class_name T_PAAMAYIM_NEKUDOTAYIM identifier          { $3->token_default_type = NAME_CONSTANT; }
    |   variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier { $3->token_default_type = NAME_CONSTANT; }
;

optional_expr:
        %empty
    |   expr
;

variable_class_name:
        dereferencable
;

dereferencable:
        variable
    |   '(' expr ')'
    |   dereferencable_scalar
;

callable_expr:
        callable_variable
    |   '(' expr ')'
    |   dereferencable_scalar
;

callable_variable:
        simple_variable
    |   dereferencable '[' optional_expr ']'
    |   constant '[' optional_expr ']'
    |   dereferencable '{' expr '}'
    |   dereferencable T_OBJECT_OPERATOR property_name argument_list { $3->token_default_type = NAME_METHOD; }
    |   function_call
;

variable:
        callable_variable
    |   static_member
    |   dereferencable T_OBJECT_OPERATOR property_name
;

simple_variable:
        T_VARIABLE
    |   '$' '{' expr '}'
    |   '$' simple_variable
;

static_member:
        class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
    |   variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
;

new_variable:
        simple_variable
    |   new_variable '[' optional_expr ']'
    |   new_variable '{' expr '}'
    |   new_variable T_OBJECT_OPERATOR property_name
    |   class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
    |   new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
;

member_name:
        identifier      { $1->token_default_type = NAME_FUNCTION; }
    |   '{' expr '}'
    |   simple_variable
;

property_name:
        T_STRING
    |   '{' expr '}'
    |   simple_variable
;

array_pair_list:
        non_empty_array_pair_list
;

possible_array_pair:
        %empty
    |   array_pair
;

non_empty_array_pair_list:
        non_empty_array_pair_list ',' possible_array_pair
    |   possible_array_pair
;

array_pair:
        expr T_DOUBLE_ARROW expr
    |   expr
    |   expr T_DOUBLE_ARROW '&' variable
    |   '&' variable
    |   expr T_DOUBLE_ARROW T_LIST '(' array_pair_list ')'
    |   T_LIST '(' array_pair_list ')'
;

encaps_list:
    /* JULP: this change was necessary to allow consecutive T_ENCAPSED_AND_WHITESPACE tokens:
     * PHP send only one (the longest possible one) at the time but this is not our case where
     * each character (almost) is a token in order to handle escaped characters and sequences.
     */
/*if 1 */
        %empty
    |   non_empty_encaps_list
;

non_empty_encaps_list:
        encaps_list_element
    |   non_empty_encaps_list encaps_list_element
;

encaps_list_element:
        encaps_var
    |   T_ENCAPSED_AND_WHITESPACE
/* else */
/*
        encaps_list encaps_var
    |   encaps_list T_ENCAPSED_AND_WHITESPACE
    |   encaps_var
    |   T_ENCAPSED_AND_WHITESPACE encaps_var
*/
/* end */
;

encaps_var:
        T_VARIABLE
    |   T_VARIABLE '[' encaps_var_offset ']'
    |   T_VARIABLE T_OBJECT_OPERATOR T_STRING
    |   T_DOLLAR_OPEN_CURLY_BRACES expr '}'
    |   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
    |   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
    |   T_CURLY_OPEN variable '}'
;

encaps_var_offset:
        T_STRING
    |   T_NUM_STRING
    |   '-' T_NUM_STRING
    |   T_VARIABLE
;


internal_functions_in_yacc:
        T_ISSET '(' isset_variables possible_comma ')'
    |   T_INCLUDE_REQUIRE expr
    |   T_FUNCTION_LIKE_KEYWORD '(' expr ')'
;

isset_variables:
        isset_variable
    |   isset_variables ',' isset_variable
;

isset_variable:
        expr
;
