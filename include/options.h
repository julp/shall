#pragma once

#ifndef DOXYGEN
# define OPT_STRVAL_FREE(opt, def) \
    do { \
        if (NULL != OPT_STRVAL(opt) && OPT_STRVAL(opt) != OPT_STRVAL(def)) { \
            free((char *) OPT_STRVAL(opt)); \
        } \
    } while (0);
#endif /* !DOXYGEN */

#include "types.h"

#define OPT_DEF_BOOL(value) \
    OPT_DEF_INT(!!value)

#define OPT_GET_BOOL(opt) \
    OPT_GET_INT(opt)

#define OPT_SET_BOOL(opt, val) \
    do { (opt).intval = !!val; } while (0);

#define OPT_DEF_INT(value) \
    { .intval = value }

#define OPT_GET_INT(opt) \
    (opt).intval

#define OPT_SET_INT(opt, val) \
    do { (opt).intval = val; } while (0);

#define OPT_DEF_STRING(value) \
    { .str = { value, STR_LEN(value) } }

#define OPT_STRVAL(opt) \
    (opt).str.val

#define OPT_STRLEN(opt) \
    (opt).str.len

#define OPT_DEF_LEXER \
    { .lexer = { NULL, NULL } }

#define OPT_LEXPTR(opt) \
    (opt).lexer.ptr

#define OPT_LEXUWF(opt) \
    (opt).lexer.unwrap_func

#define OPT_DEF_THEME \
    { .theme = NULL }

#define OPT_THEMPTR(opt) \
    (opt).theme

#define OPT_PTR(opt) \
    (opt).ptr

typedef enum {
    OPT_TYPE_PTR,
    OPT_TYPE_BOOL,
    OPT_TYPE_INT,
    OPT_TYPE_THEME,
    OPT_TYPE_LEXER,
    OPT_TYPE_STRING
} OptionType;

#define S(s) s, STR_LEN(s)

/**
 * Option value container
 * See it as variant or PHP's zval
 */
struct OptionValue {
    union { // have to be the first member
        int intval;
        struct {
            const char *val;
            size_t len;
        } str;
        struct {
            void *ptr;
            Lexer *(*unwrap_func)(void *);
        } lexer;
        void *ptr;
        const Theme *theme;
    };
    uint8_t flags ALIGNED(16);
};

/**
 * Declare an option
 */
struct OptionDeclaration {
    /**
     * Option's name
     */
    const char *name;
    /**
     * Option's name length
     */
    size_t name_len;
    /**
     * Its type, one of OPT_TYPE_* constants
     */
    OptionType type;
    /**
     * Offset of the member into the struct that override FormatterData
     */
    size_t offset;
//     int (*accept)(?);
    /**
     * Its default value
     */
    OptionValue defval;
    /**
     * A string for self documentation
     */
    const char *docstr;
};

#define END_OF_OPTIONS \
    { NULL, 0, 0, 0, OPT_DEF_INT(0), NULL }

#define LexerOption OptionDeclaration
#define FormatterOption OptionDeclaration

void option_copy(OptionType, OptionValue *, OptionValue, OptionValue);
int option_parse_as_string(OptionValue *, int, const char *, size_t, int);
