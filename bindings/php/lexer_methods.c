#include "common.h"
#include "helpers.h"
#include "options.h"
#include "lexer_class.h"

/* class methods */

PHP_FUNCTION(Shall_Base_Lexer__construct)
{
    if (EG(scope) == CALLED_SCOPE) {
        zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Shall\\Lexer\\Base cannot be instantiated", 0 TSRMLS_CC);
    }
}

/**
 * NOTE:
 * EG(scope) = class Shall\Lexer\Base
 * EG(called_scope) = class Shall\Lexer\X
 **/
PHP_FUNCTION(Shall_Base_Lexer_getAliases)
{
    if (FAILURE == zend_parse_parameters_none()) {
        RETURN_FALSE;
    } else {
        const LexerImplementation *imp;

        imp = lexer_implementation_by_name(CE_NAME(CALLED_SCOPE) + STR_LEN("Shall\\Lexer\\"));
        array_init(return_value);
        lexer_implementation_each_alias(imp, array_push_string_cb, (void *) return_value);
    }
}

PHP_FUNCTION(Shall_Base_Lexer_getMimeTypes)
{
    if (FAILURE == zend_parse_parameters_none()) {
        RETURN_FALSE;
    } else {
        const LexerImplementation *imp;

        imp = lexer_implementation_by_name(CE_NAME(CALLED_SCOPE) + STR_LEN("Shall\\Lexer\\"));
        array_init(return_value);
        lexer_implementation_each_mimetype(imp, array_push_string_cb, (void *) return_value);
    }
}

PHP_FUNCTION(Shall_Base_Lexer_getName)
{
    if (FAILURE == zend_parse_parameters_none()) {
        RETURN_FALSE;
    } else {
        const LexerImplementation *imp;

        imp = lexer_implementation_by_name(CE_NAME(CALLED_SCOPE) + STR_LEN("Shall\\Lexer\\"));
        RETURN_STRING_COPY(lexer_implementation_name(imp));
    }
}

/* real lexer */

PHP_FUNCTION(Shall_Lexer__construct)
{
    zval *options = NULL;

    return_value = getThis();
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &options)) {
        return;
    }
    if (NULL != options) {
        Shall_Lexer_object *o;

        SHALL_FETCH_LEXER(o, return_value, true);
        php_set_options((void *) o->lexer, options, 0, (set_option_t) lexer_set_option TSRMLS_CC);
    }
}

/* instance methods */

PHP_FUNCTION(Shall_Base_Lexer_getOption)
{
    int type;
    zend_strlen_t name_len = 0;
    char *name = NULL;
    zval *object = NULL;
    Shall_Lexer_object *o;
    OptionValue *optvalptr;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Lexer_ce_ptr, &name, &name_len)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(o, object, true);
    type = lexer_get_option(o->lexer, name, &optvalptr);
    php_get_option(type, optvalptr, return_value);
}

PHP_FUNCTION(Shall_Base_Lexer_setOption)
{
    zend_strlen_t name_len = 0;
    char *name = NULL;
    zval *value = NULL;
    zval *object = NULL;
    Shall_Lexer_object *o;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz", &object, Shall_Lexer_ce_ptr, &name, &name_len, &value)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(o, object, true);
    RETURN_BOOL(php_set_option((void *) o->lexer, name, value, 0, (set_option_t) lexer_set_option TSRMLS_CC));
}
