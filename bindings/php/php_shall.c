#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "php.h"
#include "php_ini.h"
#include "ext/standard/php_smart_str.h"
#define DEBUG
#include <shall/cpp.h>
#include "../../formatter.h" // TODO: temporary
#include <shall/shall.h>
#include <shall/tokens.h>
#include <shall/option.h>

#define STRINGIFY(x) #x
#define STRINGIFY_EXPANDED(x) STRINGIFY(x)

typedef struct {
    zend_object zo;

    Lexer *lexer;
} Shall_Lexer_object;

static zend_class_entry *Shall_Lexer_ce_ptr;
// TEST
static HashTable lexer_classes;
static HashTable formatters;

/* ========== general helpers ========== */

static void array_push_string_cb(const char *string, void *data)
{
    add_next_index_string((zval *) data, string, 1);
}

static void php_get_option(int type, OptionValue *optvalptr, zval *return_value)
{
    switch (type) {
        case OPT_TYPE_BOOL:
            RETVAL_BOOL(OPT_GET_BOOL(*optvalptr));
            break;
        case OPT_TYPE_INT:
            RETVAL_LONG(OPT_GET_INT(*optvalptr));
            break;
        case OPT_TYPE_STRING:
            RETVAL_STRINGL(OPT_STRVAL(*optvalptr), OPT_STRLEN(*optvalptr), 1);
            break;
        case OPT_TYPE_LEXER:
            if (NULL != OPT_LEXPTR(*optvalptr)) {
                zval *wl;

                wl = (zval *) OPT_LEXPTR(*optvalptr);
//                 zval_addref_p(wl);
//                 RETVAL_ZVAL(wl, 0, 0);
                ZVAL_COPY_VALUE(return_value, wl);
                zval_addref_p(return_value);
            }
            break;
    }
}

static int formatter_set_option_compat_cb(void *object, const char *name, OptionType type, OptionValue optval, void **UNUSED(ptr))
{
    return formatter_set_option((Formatter *) object, name, type, optval);
}

static inline Lexer *php_lexer_unwrap(void *object)
{
    Shall_Lexer_object *o;

    TSRMLS_FETCH();
    o = (Shall_Lexer_object *) zend_object_store_get_object((zval *) object TSRMLS_CC);

    return o->lexer;
}

typedef int (*set_option_t)(void *, const char *, OptionType, OptionValue, void **);

static int php_set_option(void *object, const char *name, zval *value, int reject_lexer, set_option_t cb)
{
    int type;
    zval *ptr;
    OptionValue optval;

    ptr = NULL;
    switch (Z_TYPE_P(value)) {
        case IS_BOOL:
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
            if (instanceof_function(Z_OBJCE_P(value), Shall_Lexer_ce_ptr)) {
                type = OPT_TYPE_LEXER;
                zval_addref_p(value);
                OPT_LEXPTR(optval) = value;
                OPT_LEXUWF(optval) = php_lexer_unwrap;
            }
            break;
        default:
            return 0;
    }
    cb(object, name, type, optval, (void **) &ptr);
    if (NULL != ptr) {
        zval_delref_p(ptr);
    }

    return 1;
}

static void php_set_options(void *lexer_or_formatter, zval *options, int reject_lexer, int (*cb)(void *, const char *, OptionType, OptionValue, void **))
{
    int key_type;
    ulong key_num;
    char *key_name;
    uint key_name_len;
    HashPosition pos;
    zval **entry;

    for (
        zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(options), &pos);
        SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_P(options), (void **) &entry, &pos);
        zend_hash_move_forward_ex(Z_ARRVAL_P(options), &pos)
    ) {
        key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(options), &key_name, &key_name_len, &key_num, 0, &pos);
        if (HASH_KEY_IS_STRING == key_type) {
            php_set_option(lexer_or_formatter, key_name, *entry, reject_lexer, cb);
        }
    }
}

/* ========== Lexer ========== */

zend_object_handlers Shall_Lexer_handlers;

/* helpers */

/* php internals */

static void Shall_Lexer_objects_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{
//     Shall_Lexer_object *o;

//     o = (Shall_Lexer_object *) object;
    // TODO: decrement reference count on lexer option
//     lexer_destroy(o->lexer, (on_lexer_destroy_cb_t) zval_delref_p);
    zend_objects_destroy_object(object, handle TSRMLS_CC);
//     lexer_each_sublexers(o->lexer, (on_lexer_destroy_cb_t) zval_delref_p);
}

static void Shall_Lexer_objects_free(void *object TSRMLS_DC)
{
    Shall_Lexer_object *o;

    o = (Shall_Lexer_object *) object;
    zend_object_std_dtor(&o->zo TSRMLS_C);
    lexer_destroy(o->lexer, (on_lexer_destroy_cb_t) zval_delref_p);
//     lexer_destroy(o->lexer, NULL);
    efree(o);
}

static zend_object_value Shall_Lexer_object_create(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    Shall_Lexer_object *intern;

    intern = ecalloc(1, sizeof(*intern));
    intern->lexer = lexer_create(lexer_implementation_by_name(ce->name + STR_LEN("Shall\\Lexer\\")));
    zend_object_std_init(&intern->zo, ce TSRMLS_C);
    retval.handle = zend_objects_store_put(intern, Shall_Lexer_objects_dtor, Shall_Lexer_objects_free, NULL TSRMLS_CC);
    retval.handlers = &Shall_Lexer_handlers;

    return retval;
}

#define SHALL_FETCH_LEXER(/*Shall_Lexer_object **/ o, /*zval **/ object)                                               \
    do {                                                                                                               \
        o = (Shall_Lexer_object *) zend_object_store_get_object(object TSRMLS_CC);                                     \
        if (NULL == o->lexer) {                                                                                        \
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid or unitialized %s object", Shall_Lexer_ce_ptr->name); \
            RETURN_FALSE;                                                                                              \
        }                                                                                                              \
    } while (0);

#if 0
static void wrap_Lexer(zval *object, const Lexer *lexer)
{
    Shall_Lexer_object *o;

    object_init_ex(object, Shall_Lexer_ce_ptr);
    o = (Shall_Lexer_object *) zend_object_store_get_object(object TSRMLS_CC);
    o->lexer = lexer;
}
#else
#define wrap_Lexer(/*zend_class_entry **/ ce, /*zval **/ object, /*const Lexer **/ lexer) \
    do { \
        Shall_Lexer_object *o; \
 \
        object_init_ex(object, ce); \
        o = (Shall_Lexer_object *) zend_object_store_get_object(object TSRMLS_CC); \
        o->lexer = lexer; \
    } while(0);
#endif

#define ADD_NAMESPACE(buffer, prefix, name) \
    do { \
        size_t name_len = strlen(name); \
        assert(STR_LEN(prefix) + name_len < STR_SIZE(buffer)); \
        memcpy(buffer, prefix, STR_LEN(prefix)); \
        memcpy(buffer + STR_LEN(prefix), name, name_len + 1); \
    } while (0);

/* instance methods */

PHP_FUNCTION(Shall_Base_Lexer_getOption)
{
    int type;
    int name_len = 0;
    char *name = NULL;
    zval *object = NULL;
    Shall_Lexer_object *o;
    OptionValue *optvalptr;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Lexer_ce_ptr, &name, &name_len)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(o, object);
    type = lexer_get_option(o->lexer, name, &optvalptr);
    php_get_option(type, optvalptr, return_value);
}

PHP_FUNCTION(Shall_Base_Lexer_setOption)
{
    int name_len = 0;
    char *name = NULL;
    zval *value = NULL;
    zval *object = NULL;
    Shall_Lexer_object *o;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz", &object, Shall_Lexer_ce_ptr, &name, &name_len, &value)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(o, object);
    RETURN_BOOL(php_set_option((void *) o->lexer, name, value, 0, (set_option_t) lexer_set_option));
}

/* class methods */

/* abstract base Lexer */

PHP_FUNCTION(Shall_Base_Lexer__construct)
{
    if (EG(scope) == EG(called_scope)) {
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

        imp = lexer_implementation_by_name(EG(called_scope)->name + STR_LEN("Shall\\Lexer\\"));
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

        imp = lexer_implementation_by_name(EG(called_scope)->name + STR_LEN("Shall\\Lexer\\"));
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

        imp = lexer_implementation_by_name(EG(called_scope)->name + STR_LEN("Shall\\Lexer\\"));
        RETURN_STRING(lexer_implementation_name(imp), 1);
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

        SHALL_FETCH_LEXER(o, return_value);
        php_set_options((void *) o->lexer, options, 0, (set_option_t) lexer_set_option);
    }
}

/* ========== Formatter ========== */

typedef struct {
    zend_object zo;

    Formatter *formatter;
#if 0
    // TODO: do it once for each class, not instance?
    // http://lxr.php.net/xref/PHP_5_6/ext/intl/converter/converter.c#252
    zend_fcall_info fci[5];
    zend_fcall_info_cache fcc[5];
#endif
} Shall_Formatter_object;

static zend_class_entry *Shall_Formatter_ce_ptr;

zend_object_handlers Shall_Formatter_handlers;

/* interface/callbacks */

typedef struct {
    zval *this;
    HashTable options;
} PHPFormatterData;

static void PHP_CALLBACK(const char *method, size_t method_len, int argc, zval ***params, String *out, void *data TSRMLS_DC)
{
    zval fname;
    zval *retval_ptr;
    zend_fcall_info fci;
//         zend_fcall_info_cache fcc;
    PHPFormatterData *mydata;

    mydata = (PHPFormatterData *) data;
    ZVAL_STRINGL(&fname, method, method_len, 1);
    fci.size = sizeof(fci);
    fci.function_table = &Z_OBJCE_P(mydata->this)->function_table; // NULL
    fci.function_name = &fname; // NULL
    fci.symbol_table = NULL;
    fci.object_ptr = mydata->this; // reflector_ptr
    fci.retval_ptr_ptr = &retval_ptr;
    fci.param_count = argc; // ctor_argc
    fci.params = params;
    fci.no_separation = 1;

#if 0
    fcc.initialized = 1;
//         zend_hash_find(&ce->function_table, lc_name, name_len + 1, (void**) &mptr) == SUCCESS
    fcc.function_handler = /*Z_OBJ_HT_P(mydata->this)->*/std_object_handlers.get_method(&mydata->this, method, STR_LEN(method), key TSRMLS_CC); // ce_ptr->constructor
    fcc.calling_scope = Shall_Formatter_ce_ptr; // ce_ptr
    fcc.called_scope = Z_OBJCE_P(mydata->this); // Z_OBJCE_P(reflector_ptr)
    fcc.object_ptr = mydata->this; // reflector_ptr
#endif

    if (FAILURE == zend_call_function(&fci, NULL/*&fcc*/ TSRMLS_CC)) {
        // error
    } else {
        if (retval_ptr) {
//                 COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
            switch (Z_TYPE_P(retval_ptr)) {
                case IS_NULL:
                    break;
                case IS_STRING:
                    string_append_string_len(out, Z_STRVAL_P(retval_ptr), Z_STRLEN_P(retval_ptr));
                    break;
                default:
                    // error
                    break;
            }
        }
    }
}

static int php_start_document(String *out, FormatterData *data)
{
    PHP_CALLBACK("start_document", STR_LEN("start_document"), 0, NULL, out, data TSRMLS_CC);

    return 0;
}

static int php_end_document(String *out, FormatterData *data)
{
    PHP_CALLBACK("end_document", STR_LEN("end_document"), 0, NULL, out, data TSRMLS_CC);

    return 0;
}

static int php_start_token(int token, String *out, FormatterData *data)
{
    zval *ztoken, **params[1];

    MAKE_STD_ZVAL(ztoken);
    ZVAL_LONG(ztoken, token);
    params[0] = &ztoken;
    PHP_CALLBACK("start_token", STR_LEN("start_token"), 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&ztoken);

    return 0;
}

static int php_end_token(int token, String *out, FormatterData *data)
{
    zval *ztoken, **params[1];

    MAKE_STD_ZVAL(ztoken);
    ZVAL_LONG(ztoken, token);
    params[0] = &ztoken;
    PHP_CALLBACK("end_token", STR_LEN("end_token"), 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&ztoken);

    return 0;
}

static int php_write_token(String *out, const char *token, size_t token_len, FormatterData *data)
{
    zval *ztoken, **params[1];

    MAKE_STD_ZVAL(ztoken);
    ZVAL_STRINGL(ztoken, token, token_len, 1);
    params[0] = &ztoken;
    PHP_CALLBACK("write_token", STR_LEN("write_token"), 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&ztoken);

    return 0;
}

#if 0
static OptionValue *php_define_option(Formatter *fmt, const char *name, /*size_t name_len, */size_t UNUSED(offset), OptionValue optval)
{
    size_t name_len;
    OptionValue *optvalptr;

    name_len = strlen(name);
    optvalptr = emalloc(sizeof(*optvalptr));
    zend_hash_add(&mydata->options, name, name_len + 1, &optvalptr, sizeof(optvalptr), NULL);

    return optvalptr;
}
#endif

// TODO: for pure PHP formatter, we should just use zend_hash_update
static OptionValue *php_get_option_ptr(Formatter *fmt, int define, size_t UNUSED(offset), const char *name, size_t name_len)
{
    OptionValue *optvalptr;
    PHPFormatterData *mydata;

    optvalptr = NULL;
    mydata = (PHPFormatterData *) &fmt->optvals;
    if (define) {
        optvalptr = emalloc(sizeof(*optvalptr));
        zend_hash_add(&mydata->options, name, name_len + 1, &optvalptr, sizeof(optvalptr), NULL);
    } else {
        zend_hash_find(&mydata->options, name, name_len + 1, (void **) &optvalptr);
    }

    return optvalptr;
}

static const FormatterImplementation phpfmt = {
    "PHP", // unused
    "", // unused
    php_get_option_ptr,
    php_start_document,
    php_end_document,
    php_start_token,
    php_end_token,
    php_write_token,
    sizeof(PHPFormatterData),
    NULL
};

/* php internals */

static void Shall_Formatter_objects_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{
    zend_objects_destroy_object(object, handle TSRMLS_CC);
}

static void Shall_Formatter_objects_free(void *object TSRMLS_DC)
{
    Shall_Formatter_object *o;
//     PHPFormatterData *mydata;

    o = (Shall_Formatter_object *) object;
    zend_object_std_dtor(&o->zo TSRMLS_C);
//     zend_hash_destroy(myht); // TODO
    if (&phpfmt == o->formatter->imp) {
        PHPFormatterData *mydata;

        mydata = (PHPFormatterData *) &o->formatter->optvals;
        zend_hash_destroy(&mydata->options);
    }
    formatter_destroy(o->formatter);
    efree(o);
}

static zend_object_value Shall_Formatter_object_create(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    Shall_Formatter_object *intern;
    const FormatterImplementation *imp;

// debug("[D] Shall_Formatter_object_create for %s", ce->name);
    intern = ecalloc(1, sizeof(*intern));
    if (zend_hash_exists(&formatters, ce->name, ce->name_length + 1)) {
        imp = formatter_implementation_by_name(ce->name + STR_LEN("Shall\\Formatter\\"));
    } else {
        imp = &phpfmt;
    }
    intern->formatter = formatter_create(imp);
    zend_object_std_init(&intern->zo, ce TSRMLS_C);
    retval.handle = zend_objects_store_put(intern, Shall_Formatter_objects_dtor, Shall_Formatter_objects_free, NULL TSRMLS_CC);
    retval.handlers = &Shall_Lexer_handlers;
#if 0
    if (&phpfmt == imp) {
        ((PHPFormatterData *) &intern->formatter->data)->this = retval;
    }
#endif

    return retval;
}

#define SHALL_FETCH_FORMATTER(/*Shall_Formatter_object **/ o, /*zval **/ object)                                           \
    do {                                                                                                                   \
        o = (Shall_Formatter_object *) zend_object_store_get_object(object TSRMLS_CC);                                     \
        if (NULL == o->formatter) {                                                                                        \
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid or unitialized %s object", Shall_Formatter_ce_ptr->name); \
            RETURN_FALSE;                                                                                                  \
        }                                                                                                                  \
    } while (0);

/* helpers */

/* class methods */

/* abstract base Formatter */

PHP_FUNCTION(Shall_forbidden__construct)
{
//     return_value = getThis();
    if (EG(scope) == EG(called_scope)) {
        zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "%s cannot be instantiated", EG(scope)->name);
    } else {
        zval *options = NULL;
        Shall_Formatter_object *o;

        return_value = getThis();
        if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &options)) {
            return;
        }
        SHALL_FETCH_FORMATTER(o, return_value);
        if (&phpfmt == o->formatter->imp) {
            PHPFormatterData *mydata;

            mydata = (PHPFormatterData *) &o->formatter->optvals;
            mydata->this = return_value;
            zend_hash_init(&mydata->options, 8, NULL, NULL/*dtor*/, 0);
#if 1
            {
                zend_class_entry *ce_before_base, *ce;

                for (ce = ce_before_base = Z_OBJCE_P(return_value); NULL != ce->parent; ce_before_base = ce, ce = ce->parent)
                    ;
                if (ce_before_base != Z_OBJCE_P(return_value)) {
                    if (zend_hash_exists(&formatters, ce_before_base->name, ce_before_base->name_length + 1)) {
                        const FormatterImplementation *imp;

                        imp = formatter_implementation_by_name(ce_before_base->name + STR_LEN("Shall\\Formatter\\"));
                        debug("[F] %s > %s (hérite d'un builtin formatter: %s)", Z_OBJCE_P(return_value)->name, ce_before_base->name, formatter_implementation_name(imp));
                    } else {
                        debug("[F] %s n'hérite pas d'un builtin formatter", Z_OBJCE_P(return_value)->name);
                    }
                } else {
                    debug("[F] %s hérite directement de Shall\\Formatter\\Base", Z_OBJCE_P(return_value)->name);
                }
            }
#endif
        }
        if (NULL != options) {
            php_set_options((void *) o->formatter, options, 1, formatter_set_option_compat_cb);
        }
    }
}

/* real formatter */

/* instance methods */

PHP_FUNCTION(Shall_Base_Formatter_getOption)
{
    int type;
    int name_len = 0;
    char *name = NULL;
    zval *object = NULL;
    Shall_Formatter_object *o;
    OptionValue *optvalptr;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &name, &name_len)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_FORMATTER(o, object);
    type = formatter_get_option(o->formatter, name, &optvalptr);
    php_get_option(type , optvalptr, return_value);
}


PHP_FUNCTION(Shall_Base_Formatter_setOption)
{
    int name_len = 0;
    char *name = NULL;
    zval *value = NULL;
    zval *object = NULL;
    Shall_Formatter_object *o;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz", &object, Shall_Formatter_ce_ptr, &name, &name_len, &value)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_FORMATTER(o, object);
    php_set_option((void *) o->formatter, name, value, 1, formatter_set_option_compat_cb);
}

#if 0
PHP_FUNCTION(Shall_Base_Formatter_start_document)
{
    zval *object;
    Shall_Formatter_object *o;

    if (FAILURE == zend_parse_parameters_none()) {
        return;
    }
    SHALL_FETCH_FORMATTER(o, object);
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
    int token_len;

    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, Shall_Formatter_ce_ptr, &token, &token_len)) {
        RETURN_FALSE;
    }
    RETURN_STRINGL(token, token_len, 1);
}

/* ========== functions ========== */

#if 0
zend_API.c
ZEND_API zend_class_entry *zend_get_class_entry(const zval *zobject TSRMLS_DC)
ZEND_API int zend_get_object_classname(const zval *object, const char **class_name, zend_uint *class_name_len TSRMLS_DC)

zend_execute_API.c
ZEND_API int zend_lookup_class(const char *name, int name_length, zend_class_entry ***ce TSRMLS_DC) = zend_lookup_class_ex(name, name_length, NULL, 1, ce TSRMLS_CC)
ZEND_API int zend_lookup_class_ex(const char *name, int name_length, const zend_literal *key, int use_autoload, zend_class_entry ***ce TSRMLS_DC)
ZEND_API zend_class_entry *zend_fetch_class(const char *class_name, uint class_name_len, int fetch_type TSRMLS_DC)
zend_class_entry *zend_fetch_class_by_name(const char *class_name, uint class_name_len, const zend_literal *key, int fetch_type TSRMLS_DC)
#endif
static void shall_lexer_create(const LexerImplementation *imp, zval *options, zval *out TSRMLS_DC)
{
    const char *imp_name;
    zend_class_entry **ceptr = NULL;

    imp_name = lexer_implementation_name(imp);
#if 0
    char buffer[1024];

    ADD_NAMESPACE(buffer, "Shall\\Lexer\\", imp_name);
    if (SUCCESS == zend_lookup_class_ex(buffer, strlen(buffer), NULL, 0, &ceptr TSRMLS_CC)) {
#else
    // TEST
    if (SUCCESS == zend_hash_find(&lexer_classes, (char *) imp_name, strlen(imp_name) + 1, (void **) &ceptr)) {
#endif
        Lexer *lexer;

        lexer = lexer_create(imp);
        wrap_Lexer(*ceptr, out, lexer);
        if (NULL != options) {
            php_set_options((void *) lexer, options, 0, (set_option_t) lexer_set_option);
        }
    }
}

PHP_FUNCTION(Shall_lexer_guess)
{
    int src_len = 0;
    char *src = NULL;
    zval *options = NULL;
    const LexerImplementation *imp;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &src, &src_len, &options)) {
        RETURN_FALSE;
    }
    if (NULL != (imp = lexer_implementation_guess(src, src_len))) {
        shall_lexer_create(imp, options, return_value TSRMLS_CC);
    }
}

PHP_FUNCTION(Shall_lexer_by_name)
{
    int name_len = 0;
    char *name = NULL;
    zval *options = NULL;
    const LexerImplementation *imp;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &name, &name_len, &options)) {
        RETURN_FALSE;
    }
    if (NULL != (imp = lexer_implementation_by_name(name))) {
        shall_lexer_create(imp, options, return_value TSRMLS_CC);
    }
}

PHP_FUNCTION(Shall_lexer_for_filename)
{
    Lexer *lexer = NULL;
    zval *options = NULL;
    int filename_len = 0;
    char *filename = NULL;
    const LexerImplementation *imp;
    zend_class_entry **ceptr = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &filename, &filename_len, &options)) {
        RETURN_FALSE;
    }
    if (NULL != (imp = lexer_implementation_for_filename(filename))) {
        shall_lexer_create(imp, options, return_value TSRMLS_CC);
    }
}

PHP_FUNCTION(Shall_highlight)
{
    char *dest;
    size_t dest_len;
    zval *lexer = NULL;
    zval *formatter = NULL;
    int source_len = 0;
    char *source = NULL;
    Shall_Lexer_object *l = NULL;
    Shall_Formatter_object *f = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sOO", &source, &source_len, &lexer, Shall_Lexer_ce_ptr, &formatter, Shall_Formatter_ce_ptr)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(l, lexer);
    SHALL_FETCH_FORMATTER(f, formatter);
    dest_len = highlight_string(l->lexer, f->formatter, source, &dest);

    RETURN_STRINGL(dest, dest_len, 1);
}

/* ========== registering ========== */

PHP_INI_BEGIN()
PHP_INI_END()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shall_void, 0, 0, 1)
    ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shall_0or1arg, 0, 0, 1)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shall_1or2arg, 0, 0, 2)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shall_1arg, 0, 0, 2)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shall_2arg, 0, 0, 3)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_0or1arg, 0, 0, 0)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_1or2arg, 0, 0, 1)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_1arg, 0, 0, 1)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_2arg, 0, 0, 2)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_3arg, 0, 0, 3)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
    ZEND_ARG_INFO(0, arg3)
ZEND_END_ARG_INFO()

#define SHALL_NS_MAPPED_FE(name, ns, classname, mename, arg_info) \
    ZEND_RAW_FENTRY(ZEND_NS_NAME(#ns, #name), ZEND_FN(ns##_##classname##_##mename), arg_info, 0)

#define SHALL_NS_FE(ns, name, arg_info) \
    ZEND_RAW_FENTRY(ZEND_NS_NAME(#ns, #name), ZEND_FN(ns##_##name), arg_info, 0)

static const zend_function_entry shall_functions[] = {
    SHALL_NS_FE(Shall, lexer_guess,        ainfo_shall_1or2arg)
    SHALL_NS_FE(Shall, lexer_by_name,      ainfo_shall_0or1arg)
    SHALL_NS_FE(Shall, lexer_for_filename, ainfo_shall_0or1arg)
    SHALL_NS_FE(Shall, highlight,          ainfo_shall_3arg)
    PHP_FE_END
};

static PHP_RINIT_FUNCTION(shall)
{
    return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(shall)
{
    return SUCCESS;
}

zend_function_entry Shall_Lexer_Base_class_functions[] = {
    ZEND_RAW_FENTRY("__construct",  ZEND_FN(Shall_Base_Lexer__construct),   ainfo_shall_0or1arg, ZEND_ACC_PRIVATE)
    ZEND_RAW_FENTRY("getOption",    ZEND_FN(Shall_Base_Lexer_getOption),    ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("setOption",    ZEND_FN(Shall_Base_Lexer_setOption),    ainfo_shall_2arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("getName",      ZEND_FN(Shall_Base_Lexer_getName),      ainfo_shall_void,    ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_RAW_FENTRY("getAliases",   ZEND_FN(Shall_Base_Lexer_getAliases),   ainfo_shall_void,    ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_RAW_FENTRY("getMimeTypes", ZEND_FN(Shall_Base_Lexer_getMimeTypes), ainfo_shall_void,    ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

zend_function_entry Shall_Lexer_class_functions[] = {
    ZEND_RAW_FENTRY("__construct", ZEND_FN(Shall_Lexer__construct), ainfo_shall_0or1arg, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

zend_function_entry Shall_Formatter_Base_class_functions[] = {
    ZEND_RAW_FENTRY("__construct",    ZEND_FN(Shall_forbidden__construct),          ainfo_shall_0or1arg, ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("start_document", ZEND_FN(Shall_Base_Formatter_start_document), ainfo_shall_void,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("end_document",   ZEND_FN(Shall_Base_Formatter_end_document),   ainfo_shall_void,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("start_token",    ZEND_FN(Shall_Base_Formatter_start_token),    ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("end_token",      ZEND_FN(Shall_Base_Formatter_end_token),      ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("write_token",    ZEND_FN(Shall_Base_Formatter_write_token),    ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("getOption",      ZEND_FN(Shall_Base_Formatter_getOption),      ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("setOption",      ZEND_FN(Shall_Base_Formatter_setOption),      ainfo_shall_2arg,    ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static void create_lexer_class_cb(const LexerImplementation *imp, void *data)
{
    char buffer[1024];
    const char *imp_name;
    zend_class_entry ce, *ceptr;

    imp_name = lexer_implementation_name(imp);
    ADD_NAMESPACE(buffer, "Shall\\Lexer\\", imp_name);
    INIT_OVERLOADED_CLASS_ENTRY_EX(ce, buffer, strlen(buffer), Shall_Lexer_class_functions, NULL, NULL, NULL, NULL, NULL);
    ceptr = zend_register_internal_class_ex(&ce, (zend_class_entry *) data /* Shall_Lexer_ce_ptr */, NULL TSRMLS_CC);
    ceptr->ce_flags |= ZEND_ACC_FINAL_CLASS;

#if 0
    {
        zval *ary;

        MAKE_STD_ZVAL(ary);
        array_init(ary);
        lexer_implementation_each_alias(imp, array_push_string_cb, (void *) ary);
        zend_declare_class_constant(ceptr, "ALIASES", STR_LEN("ALIASES"), ary TSRMLS_DC);
//         zend_declare_property(ceptr, "aliases", STR_LEN("aliases"), ary, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    }
#endif

    // TEST
    zend_hash_add(&lexer_classes, (char *) imp_name, strlen(imp_name) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
    zend_hash_add(&lexer_classes, buffer, strlen(buffer) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
}

#define PHPINFO_LEXER_SEPARATOR ", "

static void list_lexer_cb(const LexerImplementation *imp, void *data)
{
    smart_str_appends((smart_str *) data, lexer_implementation_name(imp));
    smart_str_appendl((smart_str *) data, PHPINFO_LEXER_SEPARATOR, STR_LEN(PHPINFO_LEXER_SEPARATOR));
}

static void create_formatter_class_cb(const FormatterImplementation *imp, void *data)
{
    char buffer[1024];
    const char *imp_name;
    zend_class_entry ce, *ceptr;

    imp_name = formatter_implementation_name(imp);
    ADD_NAMESPACE(buffer, "Shall\\Formatter\\", imp_name);
    INIT_OVERLOADED_CLASS_ENTRY_EX(ce, buffer, strlen(buffer), NULL, NULL, NULL, NULL, NULL, NULL);
    ceptr = zend_register_internal_class_ex(&ce, (zend_class_entry *) data/* Shall_Formatter_ce_ptr */, NULL TSRMLS_CC);

    // TEST
    zend_hash_add(&formatters, buffer, strlen(buffer) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
}

static PHP_MINIT_FUNCTION(shall)
{
    zend_class_entry ce;

    REGISTER_INI_ENTRIES();

    // TEST
    zend_hash_init(&lexer_classes, SHALL_LEXER_COUNT, NULL, NULL, 1);
    zend_hash_init(&formatters, SHALL_FORMATTER_COUNT, NULL, NULL, 1);

    // Abstract base lexer
    INIT_NS_CLASS_ENTRY(ce, "Shall\\Lexer", "Base", Shall_Lexer_Base_class_functions);
    ce.create_object = Shall_Lexer_object_create;
    // TODO: clone
    Shall_Lexer_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    Shall_Lexer_ce_ptr->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
    memcpy(&Shall_Lexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    // Real lexers
    lexer_implementation_each(create_lexer_class_cb, (void *) Shall_Lexer_ce_ptr);

    // Abstract base formatter
    INIT_NS_CLASS_ENTRY(ce, "Shall\\Formatter", "Base", Shall_Formatter_Base_class_functions);
    ce.create_object = Shall_Formatter_object_create;
    // TODO: clone
    Shall_Formatter_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    Shall_Formatter_ce_ptr->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
    memcpy(&Shall_Formatter_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    // Builtin formatters
    formatter_implementation_each(create_formatter_class_cb, (void *) Shall_Formatter_ce_ptr);

    // token constants
#define TOKEN(constant, description, cssclass) \
    REGISTER_NS_LONG_CONSTANT("Shall\\Token", #constant, constant, CONST_CS | CONST_PERSISTENT TSRMLS_CC);
#include <shall/keywords.h>
#undef TOKEN

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(shall)
{
    // TEST
    zend_hash_destroy(&lexer_classes);
    zend_hash_destroy(&formatters);

    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}

static PHP_MINFO_FUNCTION(shall)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "shall", "enabled");
    {
        smart_str str = { 0 };

        lexer_implementation_each(list_lexer_cb, (void *) &str);
        if (str.len >= STR_LEN(PHPINFO_LEXER_SEPARATOR)) {
            str.c[str.len - STR_LEN(PHPINFO_LEXER_SEPARATOR)] = '\0';
        }
        php_info_print_table_row(2, "Lexers", str);
        smart_str_free(&str);
    }
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

zend_module_entry shall_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "shall",
    shall_functions,
    PHP_MINIT(shall),
    PHP_MSHUTDOWN(shall),
    NULL,
    PHP_RSHUTDOWN(shall),
    PHP_MINFO(shall),
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHALL
ZEND_GET_MODULE(shall)
#endif
