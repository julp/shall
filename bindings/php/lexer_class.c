#include "common.h"
#include "options.h"
#include "lexer_class.h"
#include "lexer_methods.h"

// wraps a Lexer * into the given zval * (not a zend_object *)
#if 0
static void wrap_Lexer(zend_class_entry *ce, zval *object, const Lexer *lexer TSRMLS_DC)
{
    Shall_Lexer_object *o;

    object_init_ex(object, ce);
    FETCH_SHALL_LEXER_FROM_ZVAL(o, object);
    o->lexer = lexer;
}
#else
#define wrap_Lexer(/*zend_class_entry **/ ce, /*zval **/ object, /*const Lexer **/ lexer) \
    do {                                                                                  \
        Shall_Lexer_object *o;                                                            \
                                                                                          \
        object_init_ex(object, ce);                                                       \
        FETCH_SHALL_LEXER_FROM_ZVAL(o, object);                                           \
        o->lexer = lexer;                                                                 \
        debug("WRAP lexer is %p", lexer); \
    } while(0);
#endif

// TEST
static HashTable lexer_classes;

zend_class_entry *Shall_Lexer_ce_ptr;
static zend_object_handlers Shall_Lexer_handlers;

static void zval_lexer_dec_ref(void *value)
{
    TSRMLS_FETCH();
debug("[delref] %p", value);
#if PHP_MAJOR_VERSION >= 7
    zval tmp;
    zend_object *obj;

    obj = (zend_object *) value;
    ZVAL_OBJ(&tmp, obj);
    zval_ptr_dtor(&tmp);
#else
    zval_ptr_dtor((zval **) &value);
// debug("Z_REFCOUNT_P = %d", Z_REFCOUNT_P((zval *) value));
#endif /* PHP >= 7 */
}

static void Shall_Lexer_objects_dtor(
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

static void Shall_Lexer_objects_free(
#if PHP_MAJOR_VERSION >= 7
    zend_object *object
#else
    void *object TSRMLS_DC
#endif /* PHP >= 7 */
) {
    Shall_Lexer_object *o;

#if PHP_MAJOR_VERSION >= 7
    SHALL_LEXER_FETCH_OBJ_P(o, object);
#else
    o = (Shall_Lexer_object *) object;
#endif /* PHP >= 7 */
    zend_object_std_dtor(&o->zo TSRMLS_CC);
//     if (NULL == o) {
// debug("return;");
//         return;
//     }
debug("%s %p", lexer_implementation_name(lexer_implementation(o->lexer)), o->lexer);
    lexer_destroy(o->lexer, zval_lexer_dec_ref);
//     o->lexer = NULL;
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
Shall_Lexer_object_create(zend_class_entry *ce TSRMLS_DC)
{
    Shall_Lexer_object *intern;

#if PHP_MAJOR_VERSION >= 7
    intern = ecalloc(1, sizeof(*intern) + zend_object_properties_size(ce));
#else
    zend_object_value retval;

    intern = emalloc(sizeof(*intern));
    memset(&intern->zo, 0, sizeof(zend_object));
#endif /* PHP >= 7 */
    zend_object_std_init(&intern->zo, ce TSRMLS_CC);
    intern->lexer = lexer_create(lexer_implementation_by_name(CE_NAME(ce) + STR_LEN("Shall\\Lexer\\")));
debug("lexer is %s/%p/%p", lexer_implementation_name(lexer_implementation(intern->lexer)), intern->lexer, &intern->zo);
#if PHP_MAJOR_VERSION >= 7
    intern->zo.handlers
#else
    retval.handle = zend_objects_store_put(intern, Shall_Lexer_objects_dtor, Shall_Lexer_objects_free, NULL TSRMLS_CC);
    retval.handlers
#endif /* PHP >= 7 */
        = &Shall_Lexer_handlers;

    return
#if PHP_MAJOR_VERSION >= 7
        &intern->zo
#else
        retval
#endif /* PHP >= 7 */
    ;
}

void shall_lexer_create(const LexerImplementation *imp, zval *options, zval *out TSRMLS_DC)
{
    const char *imp_name;
    zend_class_entry *ZVALPX(ceptr) = NULL;

    imp_name = lexer_implementation_name(imp);
#if 0
    char buffer[1024];

    ADD_NAMESPACE(buffer, "Shall\\Lexer\\", imp_name);
    if (SUCCESS == zend_lookup_class_ex(buffer, strlen(buffer), NULL, 0, &ceptr TSRMLS_CC)) {
#else
    // TEST
# if PHP_MAJOR_VERSION >= 7
    if (NULL != (ceptr = zend_hash_str_find_ptr(&lexer_classes, (char *) imp_name, strlen(imp_name)))) {
# else
    if (SUCCESS == zend_hash_find(&lexer_classes, (char *) imp_name, strlen(imp_name) + 1, (void **) &ceptr)) {
# endif
#endif
        Lexer *lexer;

        lexer = lexer_create(imp);
debug("lexer is %p/%s", lexer, lexer_implementation_name(lexer_implementation(lexer)));
        wrap_Lexer(ZVALPX(ceptr), out, lexer/* TSRMLS_CC*/);
        if (NULL != options) {
            php_set_options((void *) lexer, options, 0, (set_option_t) lexer_set_option TSRMLS_CC);
        }
    }
}

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_0or1arg, 0, 0, 0)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_1arg, 0, 0, 1)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_2arg, 0, 0, 2)
    ZEND_ARG_INFO(0, arg1)
    ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

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

static void create_lexer_class_cb(const LexerImplementation *imp, void *data)
{
    char buffer[1024];
    const char *imp_name;
    zend_class_entry ce, *ceptr;

    imp_name = lexer_implementation_name(imp);
    ADD_NAMESPACE(buffer, "Shall\\Lexer\\", imp_name);
    TSRMLS_FETCH();
    INIT_OVERLOADED_CLASS_ENTRY_EX(ce, buffer, strlen(buffer), Shall_Lexer_class_functions, NULL, NULL, NULL, NULL, NULL);
    ceptr = REGISTER_INTERNAL_CLASS_EX(&ce, (zend_class_entry *) data /* Shall_Lexer_ce_ptr */);
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
#if PHP_MAJOR_VERSION >= 7
    zend_hash_str_add_new_ptr(&lexer_classes, imp_name, strlen(imp_name), ceptr);
    zend_hash_str_add_new_ptr(&lexer_classes, buffer, strlen(buffer), ceptr);
#else
    zend_hash_add(&lexer_classes, (char *) imp_name, strlen(imp_name) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
    zend_hash_add(&lexer_classes, buffer, strlen(buffer) + 1, (void **) &ceptr, sizeof(ceptr), NULL);
#endif /* PHP >= 7 */
}

void shall_register_Lexer_class(TSRMLS_D)
{
    zend_class_entry ce;

    // TEST
    zend_hash_init(&lexer_classes, SHALL_LEXER_COUNT, NULL, NULL, 1);

    // Abstract base lexer
    INIT_NS_CLASS_ENTRY(ce, "Shall\\Lexer", "Base", Shall_Lexer_Base_class_functions);
    ce.create_object = Shall_Lexer_object_create;
    // TODO: clone
    Shall_Lexer_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    Shall_Lexer_ce_ptr->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
    memcpy(&Shall_Lexer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
#if PHP_MAJOR_VERSION >= 7
    Shall_Lexer_handlers.offset = XtOffsetOf(Shall_Lexer_object, zo);
    Shall_Lexer_handlers.dtor_obj = Shall_Lexer_objects_dtor/*zend_objects_destroy_object*/;
    Shall_Lexer_handlers.free_obj = Shall_Lexer_objects_free;
#endif
    // Real lexers
    lexer_implementation_each(create_lexer_class_cb, (void *) Shall_Lexer_ce_ptr);
}

void shall_unregister_Lexer_class(TSRMLS_D)
{
    // TEST
    zend_hash_destroy(&lexer_classes);
}
