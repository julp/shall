#ifndef TYPES_H

# define TYPES_H

# include <stddef.h> /* size_t */
# include <stdint.h> /* uint\d+_t */
# include "cpp.h"
# include "bool.h"

typedef struct LexerImplementation LexerImplementation;
typedef struct Lexer Lexer;

typedef struct FormatterImplementation FormatterImplementation;
typedef struct Formatter Formatter;

typedef enum {
    OPT_TYPE_PTR,
    OPT_TYPE_BOOL,
    OPT_TYPE_INT,
    OPT_TYPE_LEXER,
    OPT_TYPE_STRING
} OptionType;

# define FL_OPT_EMBED (1<<1)

typedef struct {
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
    };
    uint8_t flags ALIGNED(16);
} OptionValue;

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

#define OPT_PTR(opt) \
    (opt).ptr

#endif /* TYPES_H */
