#pragma once

#include "common.h"

typedef struct {
#if PHP_MAJOR_VERSION < 7
    zend_object zo;
#endif /* PHP < 7 */

    Lexer *lexer;

#if PHP_MAJOR_VERSION >= 7
    zend_object zo;
#endif /* PHP >= 7 */
} Shall_Lexer_object;

/**
 * NOTE:
 * a zval wraps a zend_object
 *     which wraps a Shall_Lexer_object
 *         wich wraps a Lexer
 */

#if PHP_MAJOR_VERSION >= 7
# define SHALL_LEXER_FETCH_OBJ_P(/*Shall_Lexer_object **/ o, /*zend_object **/ object) \
    o = (Shall_Lexer_object *)((char *) (object) - XtOffsetOf(Shall_Lexer_object, zo))

# define FETCH_SHALL_LEXER_FROM_ZVAL(/*Shall_Lexer_object **/ o, /*zval **/ object) \
    SHALL_LEXER_FETCH_OBJ_P(o, Z_OBJ_P(object))

#else

# define SHALL_LEXER_FETCH_OBJ_P(/*Shall_Lexer_object **/ o, /*zend_object **/ object) \
    o = (Shall_Lexer_object *) zend_object_store_get_object(object TSRMLS_CC)

# define FETCH_SHALL_LEXER_FROM_ZVAL(/*Shall_Lexer_object **/ o, /*zval **/ object) \
    SHALL_LEXER_FETCH_OBJ_P(o, object)

#endif /* PHP >= 7 */

#define SHALL_FETCH_LEXER(/*Shall_Lexer_object **/ o, /*zval **/ object, /*int*/ check)                                   \
    do {                                                                                                                  \
        FETCH_SHALL_LEXER_FROM_ZVAL(o, object);                                                                           \
        if (check && NULL == o->lexer) {                                                                                  \
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid or unitialized %s object", CE_NAME(Shall_Lexer_ce_ptr)); \
            RETURN_FALSE;                                                                                                 \
        }                                                                                                                 \
    } while (0);

zend_class_entry *Shall_Lexer_ce_ptr;

void shall_lexer_create(const LexerImplementation *, zval *, zval * TSRMLS_DC);

void shall_register_Lexer_class(TSRMLS_D);
void shall_unregister_Lexer_class(TSRMLS_D);
