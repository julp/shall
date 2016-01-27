#include "common.h"
#include "options.h"
#include "formatter_class.h"

static int formatter_set_option_compat_cb(void *object, const char *name, OptionType type, OptionValue optval, void **UNUSED(ptr))
{
    return formatter_set_option((Formatter *) object, name, type, optval);
}

PHP_FUNCTION(Shall_forbidden__construct)
{
//     return_value = getThis();
    if (EG(scope) == CALLED_SCOPE) {
        zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "%s cannot be instantiated", CE_NAME(EG(scope)));
    } else {
        zval *options = NULL;
        Shall_Formatter_object *o;

        return_value = getThis();
        if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &options)) {
            return;
        }
        SHALL_FETCH_FORMATTER(o, return_value, true);
        if (&phpfmt == o->formatter->imp) {
            PHPFormatterData *mydata;

            mydata = (PHPFormatterData *) &o->formatter->optvals;
#if PHP_MAJOR_VERSION >= 7
//             ZVAL_MAKE_REF(&mydata->this);
//             Z_ADDREF/*_P*/(mydata->this);
            ZVAL_COPY_VALUE(&mydata->this, return_value);
#else
            mydata->this = return_value;
#endif /* PHP >= 7 */
            zend_hash_init(&mydata->options, 8, NULL, NULL/*dtor*/, 0);
#if 1
            {
                zend_class_entry *ce_before_base, *ce;

                for (ce = ce_before_base = Z_OBJCE_P(return_value); NULL != ce->parent; ce_before_base = ce, ce = ce->parent)
                    ;
                if (ce_before_base != Z_OBJCE_P(return_value)) {
                    if (ce_hash_exists(&formatters, ce)) {
                        const FormatterImplementation *imp;

                        imp = formatter_implementation_by_name(CE_NAME(ce_before_base) + STR_LEN("Shall\\Formatter\\"));
                        debug("[F] %s > %s (hérite d'un builtin formatter: %s)", CE_NAME(Z_OBJCE_P(return_value)), CE_NAME(ce_before_base), formatter_implementation_name(imp));
                    } else {
                        debug("[F] %s n'hérite pas d'un builtin formatter", CE_NAME(Z_OBJCE_P(return_value)));
                    }
                } else {
                    debug("[F] %s hérite directement de Shall\\Formatter\\Base", CE_NAME(Z_OBJCE_P(return_value)));
                }
            }
#endif
        }
        if (NULL != options) {
            php_set_options((void *) o->formatter, options, 1, formatter_set_option_compat_cb TSRMLS_CC);
        }
    }
}

/* real formatter */

/* instance methods */

PHP_FUNCTION(Shall_Base_Formatter_getOption)
{
    int type;
    zend_strlen_t name_len = 0;
    char *name = NULL;
    zval *object = NULL;
    Shall_Formatter_object *o;
    OptionValue *optvalptr;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &name, &name_len)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_FORMATTER(o, object, true);
    type = formatter_get_option(o->formatter, name, &optvalptr);
    php_get_option(type , optvalptr, return_value);
}


PHP_FUNCTION(Shall_Base_Formatter_setOption)
{
    zend_strlen_t name_len = 0;
    char *name = NULL;
    zval *value = NULL;
    zval *object = NULL;
    Shall_Formatter_object *o;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz", &object, Shall_Formatter_ce_ptr, &name, &name_len, &value)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_FORMATTER(o, object, true);
    php_set_option((void *) o->formatter, name, value, 1, formatter_set_option_compat_cb TSRMLS_CC);
}

#if 0
PHP_FUNCTION(Shall_Base_Formatter_start_document)
{
    zval *object;
    Shall_Formatter_object *o;

    if (FAILURE == zend_parse_parameters_none()) {
        return;
    }
    SHALL_FETCH_FORMATTER(o, object, true);
    RETURN_STRING(f->imp->start_document(?, &f->data), 1);
}
#endif

PHP_FUNCTION(Shall_Base_Formatter_start_document)
{
    zval *object;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, Shall_Formatter_ce_ptr)) {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(Shall_Base_Formatter_end_document)
{
    zval *object;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, Shall_Formatter_ce_ptr)) {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(Shall_Base_Formatter_start_token)
{
    long token;
    zval *object;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol", &object, Shall_Formatter_ce_ptr, &token)) {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(Shall_Base_Formatter_end_token)
{
    long token;
    zval *object;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol", &object, Shall_Formatter_ce_ptr, &token)) {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(Shall_Base_Formatter_write_token)
{
    zval *object;
    char *token;
    zend_strlen_t token_len;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &token, &token_len)) {
        RETURN_FALSE;
    }
    RETURN_STRINGL_COPY(token, token_len);
}

PHP_FUNCTION(Shall_Base_Formatter_start_lexing)
{
    zval *object;
    char *lexname;
    zend_strlen_t lexname_len;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &lexname, &lexname_len)) {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(Shall_Base_Formatter_end_lexing)
{
    char *lexname;
    zend_strlen_t lexname_len;
    zval *object;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &lexname, &lexname_len)) {
        RETURN_FALSE;
    }
}
