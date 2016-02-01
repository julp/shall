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
    highlight_string(source, source_len, &dest, &dest_len, f->formatter, 1, &l->lexer);

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
