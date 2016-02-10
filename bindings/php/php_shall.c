#include "common.h"
#include "lexer_class.h"
#include "lexer_methods.h"
#include "formatter_class.h"
#include "formatter_methods.h"

#if PHP_MAJOR_VERSION >= 7
# define smart_str smart_string
# define smart_str_free smart_string_free
# define smart_str_appendl smart_string_appendl
# define smart_str_appends smart_string_appends
# include "ext/standard/php_smart_string.h"
#else
# include "ext/standard/php_smart_str.h"
#endif /* PHP >= 7 */

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

#define ARRAY_OF_LEXERS
PHP_FUNCTION(Shall_highlight)
{
    char *dest;
    zval *lexers;
    size_t lexerc;
    size_t dest_len;
    zval *lexer = NULL;
    zval *formatter = NULL;
    char *source = NULL;
    zend_strlen_t source_len = 0;
    Shall_Lexer_object *l = NULL;
    Shall_Formatter_object *f = NULL;

#ifdef ARRAY_OF_LEXERS
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sAO", &source, &source_len, &lexers, &formatter, Shall_Formatter_ce_ptr)) {
#else
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sOO", &source, &source_len, &lexer, Shall_Lexer_ce_ptr, &formatter, Shall_Formatter_ce_ptr)) {
#endif
        RETURN_FALSE;
    }
#ifdef ARRAY_OF_LEXERS
    switch (Z_TYPE_P(lexers)) {
        case IS_OBJECT:
            if (instanceof_function(Z_OBJCE_P(lexers), Shall_Lexer_ce_ptr TSRMLS_CC)) {
                // ok
                lexerc = 1;
            } else {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument must be a Shall\\Lexer object");
                RETURN_FALSE;
            }
            break;
        case IS_ARRAY:
        {
            if (0 == (lexerc = zend_hash_num_elements(Z_ARRVAL_P(lexers)))) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty array found");
                RETURN_FALSE;
            }
            break;
        }
        default:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument must be a Shall\\Lexer object or an array of Shall\\Lexer");
            RETURN_FALSE;
    }
    {
        Lexer *lexerv[lexerc];

        if (IS_OBJECT == Z_TYPE_P(lexers)) {
            SHALL_FETCH_LEXER(l, lexer, true);
            lexerv[0] = l->lexer;
        } else {
            int i;
            zval *ZVALPX(value);

            // parcours des valeurs de ht
            i = 0;
#if PHP_MAJOR_VERSION >= 7
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(lexers), value) {
#else
            int key_type;
            ulong key_num;
            char *key_name;
            HashPosition pos;
            uint key_name_len;

            for (
                zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(lexers), &pos);
                SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_P(lexers), (void **) &value, &pos);
                zend_hash_move_forward_ex(Z_ARRVAL_P(lexers), &pos)
            ) {
#endif /* PHP >= 7 */
                if (IS_OBJECT == Z_TYPE_P(ZVALPX(value)) && instanceof_function(Z_OBJCE_P(ZVALPX(value)), Shall_Lexer_ce_ptr TSRMLS_CC)) {
                    SHALL_FETCH_LEXER(l, ZVALPX(value), true);
                    lexerv[i++] = l->lexer;
                } else {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument must be a Shall\\Lexer object");
                    RETURN_FALSE;
                }
            } ZEND_HASH_FOREACH_END();
        }
#else
    SHALL_FETCH_LEXER(l, lexer, true);
#endif
    SHALL_FETCH_FORMATTER(f, formatter, true);
debug("HL WITH IMP = %s/%p/%p/%p", lexer_implementation_name(lexer_implementation(l->lexer)), l->lexer, lexer, Z_OBJ_P(lexer));
#ifdef ARRAY_OF_LEXERS
        highlight_string(source, source_len, &dest, &dest_len, f->formatter, lexerc, lexerv);
    }
#else
    highlight_string(source, source_len, &dest, &dest_len, f->formatter, 1, &l->lexer);
#endif

    RETURN_STRINGL_COPY(dest, dest_len);
}

/* ========== registering ========== */

PHP_INI_BEGIN()
PHP_INI_END()

/*
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
*/

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_0or1arg, 0, 0, 0)
    ZEND_ARG_INFO(0, arg1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ainfo_shall_1or2arg, 0, 0, 1)
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

#define PHPINFO_LEXER_SEPARATOR ", "

static void list_lexer_cb(const LexerImplementation *imp, void *data)
{
    smart_str_appends((smart_str *) data, lexer_implementation_name(imp));
    smart_str_appendl((smart_str *) data, PHPINFO_LEXER_SEPARATOR, STR_LEN(PHPINFO_LEXER_SEPARATOR));
}

static PHP_MINIT_FUNCTION(shall)
{
    REGISTER_INI_ENTRIES();

    // token constants
#define TOKEN(constant, parent, description, cssclass) \
    REGISTER_NS_LONG_CONSTANT("Shall\\Token", #constant, constant, CONST_CS | CONST_PERSISTENT TSRMLS_CC);
#include <shall/keywords.h>
#undef TOKEN

    shall_register_Lexer_class(TSRMLS_C);
    shall_register_Formatter_class(TSRMLS_C);

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(shall)
{
    shall_unregister_Lexer_class(TSRMLS_C);
    shall_unregister_Formatter_class(TSRMLS_C);

    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}

static PHP_MINFO_FUNCTION(shall)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "shall", "enabled");
    {
#include <shall/version.h>
        Version v;
        char tmp[VERSION_STRING_MAX_LENGTH];

        version_get(v);
        version_to_string(v, tmp, STR_SIZE(tmp));
        php_info_print_table_row(2, "Version", tmp);
    }
    {
        smart_str str = { 0 };

        lexer_implementation_each(list_lexer_cb, (void *) &str);
        if (str.len >= STR_LEN(PHPINFO_LEXER_SEPARATOR)) {
            str.c[str.len - STR_LEN(PHPINFO_LEXER_SEPARATOR)] = '\0';
        }
        php_info_print_table_row(2, "Lexers", str.c);
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
