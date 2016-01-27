#pragma once

#include "common.h"

typedef struct {
#if PHP_MAJOR_VERSION < 7
    zend_object zo;
#endif /* PHP < 7 */

    Formatter *formatter;
#if 0
    // TODO: do it once for each class, not instance?
    // http://lxr.php.net/xref/PHP_5_6/ext/intl/converter/converter.c#252
    zend_fcall_info fci[7];
    zend_fcall_info_cache fcc[7];
#endif
#if PHP_MAJOR_VERSION >= 7
    zend_object zo;
#endif /* PHP >= 7 */
} Shall_Formatter_object;

#if PHP_MAJOR_VERSION >= 7
# define SHALL_FORMATTER_FETCH_OBJ_P(/*Shall_Formatter_object **/ o, /*zend_object **/ object) \
    o = (Shall_Formatter_object *)((char *) (object) - XtOffsetOf(Shall_Formatter_object, zo))

# define SHALL_FETCH_FORMATTER(/*Shall_Formatter_object **/ o, /*zval **/ object, /*int*/ check)                              \
    do {                                                                                                                      \
        SHALL_FORMATTER_FETCH_OBJ_P(o, Z_OBJ_P(object));                                                                      \
        if (check && NULL == o->formatter) {                                                                                  \
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid or unitialized %s object", CE_NAME(Shall_Formatter_ce_ptr)); \
            RETURN_FALSE;                                                                                                     \
        }                                                                                                                     \
    } while (0);
#else
# define SHALL_FETCH_FORMATTER(/*Shall_Formatter_object **/ o, /*zval **/ object, /*int*/ check)                              \
    do {                                                                                                                      \
        o = (Shall_Formatter_object *) zend_object_store_get_object(object TSRMLS_CC);                                        \
        if (check && NULL == o->formatter) {                                                                                  \
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid or unitialized %s object", CE_NAME(Shall_Formatter_ce_ptr)); \
            RETURN_FALSE;                                                                                                     \
        }                                                                                                                     \
    } while (0);
#endif /* PHP >= 7 */

typedef struct {
    zval ZVALPX(this);
    HashTable options;
} PHPFormatterData;

extern HashTable formatters;
extern const FormatterImplementation phpfmt;
extern zend_class_entry *Shall_Formatter_ce_ptr;

void shall_register_Formatter_class(TSRMLS_D);
void shall_unregister_Formatter_class(TSRMLS_D);
