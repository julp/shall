#include "common.h"
#include "options.h"
#include "lexer_class.h"

void php_get_option(int type, OptionValue *optvalptr, zval *return_value)
{
    switch (type) {
        case OPT_TYPE_BOOL:
            RETVAL_BOOL(OPT_GET_BOOL(*optvalptr));
            break;
        case OPT_TYPE_INT:
            RETVAL_LONG(OPT_GET_INT(*optvalptr));
            break;
        case OPT_TYPE_STRING:
            RETVAL_STRINGL_COPY(OPT_STRVAL(*optvalptr), OPT_STRLEN(*optvalptr));
            break;
        case OPT_TYPE_LEXER:
            if (NULL != OPT_LEXPTR(*optvalptr)) {
#if PHP_MAJOR_VERSION >= 7
                zend_object *obj;

                obj = (zend_object *) OPT_LEXPTR(*optvalptr);
                RETVAL_OBJ(obj);
#else
                zval *wl;

                wl = (zval *) OPT_LEXPTR(*optvalptr);
                RETVAL_ZVAL(wl, 0, 0);
#endif /* PHP >= 7 */
            }
            break;
    }
}

// PHP 5: fetch a Lexer * from a zval * (not a zend_object *)
// PHP 7: fetch a Lexer * from a zend_object * (not a zval *)
static inline Lexer *php_lexer_unwrap(void *object)
{
    Shall_Lexer_object *o;

    TSRMLS_FETCH();
    {
#if PHP_MAJOR_VERSION >= 7
        zend_object *obj;

        obj = (zend_object *) object;
        SHALL_LEXER_FETCH_OBJ_P(o, obj);
debug("UNWRAP lexer is %p/X/%p", o->lexer, obj);
debug("UNWRAP imp = %s", lexer_implementation_name(lexer_implementation(o->lexer)));
#else
        zval *zobj;

        zobj = (zval *) object;
        FETCH_SHALL_LEXER_FROM_ZVAL(o, zobj);
debug("UNWRAP lexer is %p/%p/%p", o->lexer, zobj, Z_OBJ_P(zobj));
debug("UNWRAP imp = %s", lexer_implementation_name(lexer_implementation(o->lexer)));
#endif /* PHP >= 7 */
    }

    return o->lexer;
}

int php_set_option(void *object, const char *name, zval *value, int reject_lexer, set_option_t cb TSRMLS_DC)
{
    int type;
    zval *ptr;
    OptionValue optval;

    ptr = NULL;
    switch (Z_TYPE_P(value)) {
#if PHP_MAJOR_VERSION >= 7
        case IS_TRUE:
        case IS_FALSE:
#else
        case IS_BOOL:
#endif /* PHP >= 7 */
            type = OPT_TYPE_BOOL;
            OPT_SET_BOOL(optval, Z_BVAL_P(value));
            break;
        case IS_LONG:
            type = OPT_TYPE_INT;
            OPT_SET_INT(optval, Z_LVAL_P(value));
            break;
        case IS_STRING:
            type = OPT_TYPE_STRING;
            OPT_STRVAL(optval) = Z_STRVAL_P(value);
            OPT_STRLEN(optval) = Z_STRLEN_P(value);
            break;
        case IS_OBJECT:
            // TODO: NULL to "remove" sublexers
            if (instanceof_function(Z_OBJCE_P(value), Shall_Lexer_ce_ptr TSRMLS_CC)) {
                type = OPT_TYPE_LEXER;
debug("[addref] %p/%p", value, Z_OBJ_P(value));
                zval_addref_p(value);
                OPT_LEXUWF(optval) = php_lexer_unwrap;
#if PHP_MAJOR_VERSION >= 7
                OPT_LEXPTR(optval) = Z_OBJ_P(value);
debug("set lexer %p/%p/%p/%s as option", php_lexer_unwrap(Z_OBJ_P(value)), value, Z_OBJ_P(value), lexer_implementation_name(lexer_implementation(lexer_unwrap(optval))));
#else
                OPT_LEXPTR(optval) = value;
debug("set lexer %p/%p/%p/%s as option", php_lexer_unwrap(value), value, Z_OBJ_P(value), lexer_implementation_name(lexer_implementation(lexer_unwrap(optval))));
#endif /* PHP >= 7 */
            }
            break;
        default:
            return 0;
    }
    if (OPT_SUCCESS == cb(object, name, type, optval, (void **) &ptr)) {
        if (NULL != ptr) {
#if PHP_MAJOR_VERSION >= 7
            zval_ptr_dtor(ptr);
#else
            zval_ptr_dtor(&ptr);
#endif /* PHP >= 7 */
        }
        return 1;
    } else {
        return 0;
    }
}

void php_set_options(void *lexer_or_formatter, zval *options, int reject_lexer, int (*cb)(void *, const char *, OptionType, OptionValue, void **) TSRMLS_DC)
{
#if PHP_MAJOR_VERSION >= 7
    zval *entry;
    zend_string *key;

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(options), key, entry) {
        php_set_option(lexer_or_formatter, ZSTR_VAL(key), entry, reject_lexer, cb);
    } ZEND_HASH_FOREACH_END();
#else
    int key_type;
    zval **entry;
    ulong key_num;
    char *key_name;
    HashPosition pos;
    uint key_name_len;

    for (
        zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(options), &pos);
        SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_P(options), (void **) &entry, &pos);
        zend_hash_move_forward_ex(Z_ARRVAL_P(options), &pos)
    ) {
        key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(options), &key_name, &key_name_len, &key_num, 0, &pos);
        if (HASH_KEY_IS_STRING == key_type) {
            php_set_option(lexer_or_formatter, key_name, *entry, reject_lexer, cb TSRMLS_CC);
        }
    }
#endif /* PHP >= 7 */
}
