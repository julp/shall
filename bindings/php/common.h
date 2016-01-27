#pragma once

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "php.h"
#include "php_ini.h"
#include <shall/cpp.h>
#include <shall/formatter.h>
#include <shall/shall.h>
#include <shall/tokens.h>
#include <shall/types.h>
#undef S

#define STRINGIFY(x) #x
#define STRINGIFY_EXPANDED(x) STRINGIFY(x)

#ifdef DEBUG
# include <stdio.h>
# define debug(format, ...) \
    fprintf(stderr, "[%d:%s] " format "\n", __LINE__, __func__, ## __VA_ARGS__)
#else
# define debug(format, ...)
#endif /* DEBUG */

// PHP >= 7
#if PHP_MAJOR_VERSION >= 7

typedef size_t zend_strlen_t;

# define MAKE_STD_ZVAL(z) /* NOP */
// add one level of indirection depending on version
# define ZVALPX(z) z
// its "opposite"
# define ZVALRX(z) &z
// ZEND_ACC_FINAL_CLASS was renamed into ZEND_ACC_FINAL
# define ZEND_ACC_FINAL_CLASS ZEND_ACC_FINAL
// EG(called_scope) was changed
# define CALLED_SCOPE \
    zend_get_called_scope(EG(current_execute_data))
// class entry's name is a zend_string, not a char * anymore
# define CE_STRNAME(n) \
    ZSTR_VAL(n)
# define CE_STRLEN(ce) \
    ZSTR_LEN(ce->name)
// declare a zend string vs char *n + long n_len
# define ZSTR_DECL(n) \
    zend_string *n
// zend_parse_* modifier for zend_string vs char * + long *
# define ZSTR_MODIFIER "S"
// zend_parse_* arguments for zend_string vs char * + long *
# define ZSTR_ARG(s) &s
// for key lookups in hashtable (since PHP 7, '\0' have to be "ignored")
# define S(s) s, STR_LEN(s)
// boolean type was split into 2 (true/false are 2 different types)
# define Z_BVAL_P(z) (IS_TRUE == Z_TYPE(*(z)))
// a macro for portability with old (Z|RET)(VAL|URN)_STRINGL? where string is copied (ie 3rd argument was 1)
# define ZVAL_STRING_COPY(zval, string) \
    ZVAL_STRING(zval, string)
# define ZVAL_STRINGL_COPY(zval, string, length) \
    ZVAL_STRINGL(zval, string, length)
# define RETURN_STRING_COPY(string) \
    RETURN_STRING(string)
# define RETURN_STRINGL_COPY(string, length) \
    RETURN_STRINGL(string, length)
# define RETVAL_STRINGL_COPY(string, length) \
    RETVAL_STRINGL(string, length)
# define add_next_index_string_copy(zval, str) \
    add_next_index_string(zval, str)
// hashtable with CE names
# define ce_hash_exists(h, ce) \
    zend_hash_exists(h, ce->name)
# define REGISTER_INTERNAL_CLASS_EX(ceptr, parentceptr) \
    zend_register_internal_class_ex(ceptr, parentceptr);

#else /* PHP < 7 */

typedef int zend_strlen_t;

# define ZVAL_UNDEF(z) \
    (z) = NULL
# define Z_ISUNDEF(z) \
    (NULL == (z))
# define ZVALPX(z) *z
# define ZVALRX(z) z
# define CE_STRNAME(n) \
    n
# define CE_STRLEN(ce) \
    ce->name_length
# define CALLED_SCOPE \
    EG(called_scope)
# define ZSTR_DECL(n) \
    char *n; \
    zend_strlen_t n##_len
# define ZSTR_MODIFIER "s"
# define ZSTR_ARG(s) &s, &s##_len
# define S(s) s, STR_SIZE(s)
# define ZVAL_STRING_COPY(zval, string) \
    ZVAL_STRING(zval, string, 1)
# define ZVAL_STRINGL_COPY(zval, string, length) \
    ZVAL_STRINGL(zval, string, length, 1)
# define RETURN_STRING_COPY(string) \
    RETURN_STRING(string, 1)
# define RETURN_STRINGL_COPY(string, length) \
    RETURN_STRINGL(string, length, 1)
# define RETVAL_STRINGL_COPY(string, length) \
    RETVAL_STRINGL(string, length, 1)
# define add_next_index_string_copy(zval, str) \
    add_next_index_string(zval, str, 1)
# define ce_hash_exists(h, ce) \
    zend_hash_exists(h, ce->name, ce->name_length + 1)
# define REGISTER_INTERNAL_CLASS_EX(ceptr, parentceptr) \
    zend_register_internal_class_ex(ceptr, parentceptr, NULL TSRMLS_CC);

#endif /* PHP >= 7 */

#define CE_NAME(ce) \
    CE_STRNAME(ce->name)

#define CE_NAMELEN(ce) \
    CE_STRLEN(ce)

#define ADD_NAMESPACE(buffer, prefix, name) \
    do { \
        size_t name_len = strlen(name); \
        assert(STR_LEN(prefix) + name_len < STR_SIZE(buffer)); \
        memcpy(buffer, prefix, STR_LEN(prefix)); \
        memcpy(buffer + STR_LEN(prefix), name, name_len + 1); \
    } while (0);
