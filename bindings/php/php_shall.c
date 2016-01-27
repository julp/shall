#include "common.h"
#include "lexer_class.h"
#include "lexer_methods.h"

#include "ext/standard/php_smart_str.h"

// TEST
static HashTable formatters;

/* ========== general helpers ========== */

static int formatter_set_option_compat_cb(void *object, const char *name, OptionType type, OptionValue optval, void **UNUSED(ptr))
{
    return formatter_set_option((Formatter *) object, name, type, optval);
}

/* ========== Formatter ========== */

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

static union {
    struct {
        zval ZVALPX(start_document);
        zval ZVALPX(end_document);
        zval ZVALPX(start_token);
        zval ZVALPX(write_token);
        zval ZVALPX(end_token);
        zval ZVALPX(start_lexing);
        zval ZVALPX(end_lexing);
    };
    zval ZVALPX(all)[7];
} fmt_callback_names;
static zend_class_entry *Shall_Formatter_ce_ptr;

zend_object_handlers Shall_Formatter_handlers;

/* interface/callbacks */

typedef struct {
    zval ZVALPX(this);
    HashTable options;
} PHPFormatterData;

static void PHP_CALLBACK(
    zval ZVALPX(fname),
    int argc,
#if PHP_MAJOR_VERSION >= 7
    zval *params,
#else
    zval ***params,
#endif /* PHP >= 7 */
    String *out,
    void *data TSRMLS_DC
) {
    zend_fcall_info fci;
    zval ZVALPX(retval_ptr);
//         zend_fcall_info_cache fcc;
    PHPFormatterData *mydata;

    mydata = (PHPFormatterData *) data;
    fci.size = sizeof(fci);
    fci.function_name = fname; // NULL
#if PHP_MAJOR_VERSION >= 7
    fci.function_table = &Z_OBJCE/*_P*/(mydata->this)->function_table; // NULL
    fci.object = Z_OBJ/*_P*/(mydata->this);
    fci.retval = &retval_ptr;
#else
//     fci.function_name = fname; // NULL
    fci.function_table = &Z_OBJCE_P(mydata->this)->function_table; // NULL
    fci.object_ptr = mydata->this; // reflector_ptr
    fci.retval_ptr_ptr = &retval_ptr;
#endif /* PHP >= 7 */
    fci.symbol_table = NULL;
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
        if (!Z_ISUNDEF(retval_ptr)) {
//                 COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
            switch (Z_TYPE_P(ZVALRX(retval_ptr))) {
                case IS_NULL:
                    break;
                case IS_STRING:
                    string_append_string_len(out, Z_STRVAL_P(ZVALRX(retval_ptr)), Z_STRLEN_P(ZVALRX(retval_ptr)));
                    break;
                default:
                    // error
                    break;
            }
            zval_ptr_dtor(&retval_ptr);
        }
    }
}

static int php_start_document(String *out, FormatterData *data)
{
    TSRMLS_FETCH();
    PHP_CALLBACK(fmt_callback_names.start_document, 0, NULL, out, data TSRMLS_CC);

    return 0;
}

static int php_end_document(String *out, FormatterData *data)
{
    TSRMLS_FETCH();
    PHP_CALLBACK(fmt_callback_names.end_document, 0, NULL, out, data TSRMLS_CC);

    return 0;
}

#if PHP_MAJOR_VERSION >= 7
# define PARAM_1_NAME \
    params[0]
# define PARAM_1_DECL \
    zval params[1]
#else
# define PARAM_1_NAME \
    zparam_1
# define PARAM_1_DECL \
    zval *zparam_1, **params[1]; \
    MAKE_STD_ZVAL(PARAM_1_NAME); \
    params[0] = &PARAM_1_NAME
#endif /* PHP >= 7 */

static int php_start_token(int token, String *out, FormatterData *data)
{
    PARAM_1_DECL;

    TSRMLS_FETCH();
    ZVAL_LONG(ZVALRX(PARAM_1_NAME), token);
    PHP_CALLBACK(fmt_callback_names.start_token, 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&PARAM_1_NAME);

    return 0;
}

static int php_end_token(int token, String *out, FormatterData *data)
{
    PARAM_1_DECL;

    TSRMLS_FETCH();
    ZVAL_LONG(ZVALRX(PARAM_1_NAME), token);
    PHP_CALLBACK(fmt_callback_names.end_token, 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&PARAM_1_NAME);

    return 0;
}

static int php_write_token(String *out, const char *token, size_t token_len, FormatterData *data)
{
    PARAM_1_DECL;

    TSRMLS_FETCH();
    ZVAL_STRINGL_COPY(ZVALRX(PARAM_1_NAME), token, token_len);
    PHP_CALLBACK(fmt_callback_names.write_token, 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&PARAM_1_NAME);

    return 0;
}

static int php_start_lexing(const char *lexname, String *out, FormatterData *data)
{
    PARAM_1_DECL;

    TSRMLS_FETCH();
    ZVAL_STRING_COPY(ZVALRX(PARAM_1_NAME), lexname);
    PHP_CALLBACK(fmt_callback_names.start_lexing, 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&PARAM_1_NAME);

    return 0;
}

static int php_end_lexing(const char *lexname, String *out, FormatterData *data)
{
    PARAM_1_DECL;

    TSRMLS_FETCH();
    ZVAL_STRING_COPY(ZVALRX(PARAM_1_NAME), lexname);
    PHP_CALLBACK(fmt_callback_names.end_lexing, 1, params, out, data TSRMLS_CC);
    zval_ptr_dtor(&PARAM_1_NAME);

    return 0;
}

#if 0
static OptionValue *php_define_option(Formatter *fmt, const char *name, /*size_t name_len, */size_t UNUSED(offset), OptionValue optval)
{
    size_t name_len;
    OptionValue *optvalptr;

    name_len = strlen(name);
    optvalptr = emalloc(sizeof(*optvalptr));
#if PHP_MAJOR_VERSION >= 7
    zend_hash_str_add_new_ptr(&mydata->options, name, name_len, &optvalptr);
#else
    zend_hash_add(&mydata->options, name, name_len + 1, &optvalptr, sizeof(optvalptr), NULL);
#endif /* PHP >= 7 */

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
#if PHP_MAJOR_VERSION >= 7
        zend_hash_str_add_new_ptr(&mydata->options, name, name_len, &optvalptr);
#else
        zend_hash_add(&mydata->options, name, name_len + 1, &optvalptr, sizeof(optvalptr), NULL);
#endif /* PHP >= 7 */
    } else {
#if PHP_MAJOR_VERSION >= 7
        optvalptr = zend_hash_str_find_ptr(&mydata->options, name, name_len);
#else
        zend_hash_find(&mydata->options, name, name_len + 1, (void **) &optvalptr);
#endif /* PHP >= 7 */
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
    php_start_lexing,
    php_end_lexing,
    sizeof(PHPFormatterData),
    NULL
};

/* php internals */

static void Shall_Formatter_objects_dtor(
#if PHP_MAJOR_VERSION >= 7
    zend_object *object
#else
    void *object, zend_object_handle handle TSRMLS_DC
#endif /* PHP >= 7 */
) {
    zend_objects_destroy_object(
        object
#if PHP_MAJOR_VERSION < 7
        , handle TSRMLS_CC
#endif /* PHP < 7 */
    );
}

static void Shall_Formatter_objects_free(
#if PHP_MAJOR_VERSION >= 7
    zend_object *object
#else
    void *object TSRMLS_DC
#endif /* PHP >= 7 */
) {
    Shall_Formatter_object *o;
//     PHPFormatterData *mydata;

#if PHP_MAJOR_VERSION >= 7
    SHALL_FORMATTER_FETCH_OBJ_P(o, object);
#else
    o = (Shall_Formatter_object *) object;
#endif /* PHP >= 7 */
debug("%s %p", formatter_implementation_name(formatter_implementation(o->formatter)), o->formatter);
//     zend_hash_destroy(myht); // TODO
    if (&phpfmt == o->formatter->imp) {
        PHPFormatterData *mydata;

        mydata = (PHPFormatterData *) &o->formatter->optvals;
        zend_hash_destroy(&mydata->options);
    }
    formatter_destroy(o->formatter);
    zend_object_std_dtor(&o->zo TSRMLS_CC);
#if PHP_MAJOR_VERSION < 7
    efree(o);
#endif /* PHP < 7 */
}

static
#if PHP_MAJOR_VERSION >= 7
zend_object *
#else
zend_object_value
#endif /* PHP >= 7 */
Shall_Formatter_object_create(zend_class_entry *ce TSRMLS_DC)
{
    Shall_Formatter_object *intern;
    const FormatterImplementation *imp;

// debug("[D] Shall_Formatter_object_create for %s", CE_NAME(ce));
#if PHP_MAJOR_VERSION >= 7
    intern = ecalloc(1, sizeof(*intern) + zend_object_properties_size(ce));
#else
    zend_object_value retval;

    intern = emalloc(sizeof(*intern));
    memset(&intern->zo, 0, sizeof(zend_object));
#endif /* PHP >= 7 */
    if (ce_hash_exists(&formatters, ce)) {
        imp = formatter_implementation_by_name(CE_NAME(ce) + STR_LEN("Shall\\Formatter\\"));
    } else {
        imp = &phpfmt;
    }
    intern->formatter = formatter_create(imp);
    zend_object_std_init(&intern->zo, ce TSRMLS_CC);
#if PHP_MAJOR_VERSION >= 7
    intern->zo.handlers
#else
    retval.handle = zend_objects_store_put(intern, Shall_Formatter_objects_dtor, Shall_Formatter_objects_free, NULL TSRMLS_CC);
    retval.handlers
#endif /* PHP >= 7 */
        = &Shall_Formatter_handlers;
#if 0
    if (&phpfmt == imp) {
        ((PHPFormatterData *) &intern->formatter->data)->this = retval;
    }
#endif

    return
#if PHP_MAJOR_VERSION >= 7
        &intern->zo
#else
        retval
#endif /* PHP >= 7 */
    ;
}

/* helpers */

/* class methods */

/* abstract base Formatter */

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

/* ========== functions ========== */

PHP_FUNCTION(Shall_lexer_guess)
{
    zend_strlen_t src_len = 0;
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
    zend_strlen_t name_len = 0;
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
    zend_strlen_t filename_len = 0;
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
    zend_strlen_t source_len = 0;
    char *source = NULL;
    Shall_Lexer_object *l = NULL;
    Shall_Formatter_object *f = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sOO", &source, &source_len, &lexer, Shall_Lexer_ce_ptr, &formatter, Shall_Formatter_ce_ptr)) {
        RETURN_FALSE;
    }
    SHALL_FETCH_LEXER(l, lexer, true);
    SHALL_FETCH_FORMATTER(f, formatter, true);
debug("HL WITH IMP = %s/%p/%p/%p", lexer_implementation_name(lexer_implementation(l->lexer)), l->lexer, lexer, Z_OBJ_P(lexer));
    highlight_string(l->lexer, f->formatter, source, source_len, &dest, &dest_len);

    RETURN_STRINGL_COPY(dest, dest_len);
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

zend_function_entry Shall_Formatter_Base_class_functions[] = {
    ZEND_RAW_FENTRY("__construct",    ZEND_FN(Shall_forbidden__construct),          ainfo_shall_0or1arg, ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("start_document", ZEND_FN(Shall_Base_Formatter_start_document), ainfo_shall_void,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("end_document",   ZEND_FN(Shall_Base_Formatter_end_document),   ainfo_shall_void,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("start_token",    ZEND_FN(Shall_Base_Formatter_start_token),    ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("end_token",      ZEND_FN(Shall_Base_Formatter_end_token),      ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("write_token",    ZEND_FN(Shall_Base_Formatter_write_token),    ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("start_lexing",   ZEND_FN(Shall_Base_Formatter_start_lexing),   ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("end_lexing",     ZEND_FN(Shall_Base_Formatter_end_lexing),     ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("getOption",      ZEND_FN(Shall_Base_Formatter_getOption),      ainfo_shall_1arg,    ZEND_ACC_PUBLIC)
    ZEND_RAW_FENTRY("setOption",      ZEND_FN(Shall_Base_Formatter_setOption),      ainfo_shall_2arg,    ZEND_ACC_PUBLIC)
    PHP_FE_END
};

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
    TSRMLS_FETCH();
    INIT_OVERLOADED_CLASS_ENTRY_EX(ce, buffer, strlen(buffer), NULL, NULL, NULL, NULL, NULL, NULL);
    ceptr = REGISTER_INTERNAL_CLASS_EX(&ce, (zend_class_entry *) data/* Shall_Formatter_ce_ptr */);

    // TEST
#if PHP_MAJOR_VERSION >= 7
    zend_hash_str_add_new_ptr(&formatters, buffer, strlen(buffer), ceptr);
#else
    zend_hash_add(&formatters, buffer, strlen(buffer) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
#endif /* PHP >= 7 */
}

static PHP_MINIT_FUNCTION(shall)
{
    size_t i;
    zend_class_entry ce;

    REGISTER_INI_ENTRIES();

    for (i = 0; i < ARRAY_SIZE(fmt_callback_names.all); i++) {
        MAKE_STD_ZVAL(fmt_callback_names.all[i]);
    }
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.start_document), "start_document", STR_LEN("start_document"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.end_document), "end_document", STR_LEN("end_document"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.start_token), "start_token", STR_LEN("start_token"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.write_token), "write_token", STR_LEN("write_token"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.end_token), "end_token", STR_LEN("end_token"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.start_lexing), "start_lexing", STR_LEN("start_lexing"));
    ZVAL_STRINGL_COPY(ZVALRX(fmt_callback_names.end_lexing), "end_lexing", STR_LEN("end_lexing"));

    // TEST
    zend_hash_init(&formatters, SHALL_FORMATTER_COUNT, NULL, NULL, 1);

    // Abstract base formatter
    INIT_NS_CLASS_ENTRY(ce, "Shall\\Formatter", "Base", Shall_Formatter_Base_class_functions);
    ce.create_object = Shall_Formatter_object_create;
    // TODO: clone
    Shall_Formatter_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    Shall_Formatter_ce_ptr->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
    memcpy(&Shall_Formatter_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
#if PHP_MAJOR_VERSION >= 7
    Shall_Formatter_handlers.offset = XtOffsetOf(Shall_Formatter_object, zo);
    Shall_Formatter_handlers.dtor_obj = Shall_Formatter_objects_dtor/*zend_objects_destroy_object*/;
    Shall_Formatter_handlers.free_obj = Shall_Formatter_objects_free;
#endif
    // Builtin formatters
    formatter_implementation_each(create_formatter_class_cb, (void *) Shall_Formatter_ce_ptr);

    // token constants
#define TOKEN(constant, description, cssclass) \
    REGISTER_NS_LONG_CONSTANT("Shall\\Token", #constant, constant, CONST_CS | CONST_PERSISTENT TSRMLS_CC);
#include <shall/keywords.h>
#undef TOKEN

    shall_register_Lexer_class(TSRMLS_C);

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(shall)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(fmt_callback_names.all); i++) {
// TODO: segfault in PHP 7
#if PHP_MAJOR_VERSION < 7
        zval_dtor(ZVALRX(fmt_callback_names.all[i]));
#endif /* PHP < 7 */
    }

    shall_unregister_Lexer_class(TSRMLS_C);

    // TEST
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
