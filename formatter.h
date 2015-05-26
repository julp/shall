#ifndef FORMATTER_H

# define FORMATTER_H

# include "types.h"
# include "xtring.h"
# include "hashtable.h"

typedef void FormatterData;

#define S(s) s, STR_LEN(s)

/**
 * Formatter option
 */
typedef struct {
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
} FormatterOption;

#define END_OF_FORMATTER_OPTIONS \
    { NULL, 0, 0, 0, OPT_DEF_INT(0), NULL }

// typedef struct Formatter Formatter;

/**
 * Define a formatter (acts as a class)
 */
struct FormatterImplementation {
    /**
     * Official name of the formatter.
     * Should start with an uppercase letter and include only ASCII alphanumeric characters plus underscores
     */
    const char *name;
    /**
     * A string for self documentation
     */
    const char *docstr;
#if 0
    int (*init)(FormatterData *);
#endif
    /**
     * Callback to get address of a given option.
     * Called to get or change current value of an option.
     */
    OptionValue *(*get_option_ptr)(Formatter *, int, size_t, const char *, size_t);
    /**
     * Optionnal (may be NULL) callback called (once) at the beginning of the tokenisation process
     */
    int (*start_document)(String *, FormatterData *);
    /**
     * Optionnal (may be NULL) callback called (once) at the end of the tokenisation process
     */
    int (*end_document)(String *, FormatterData *);
    /**
     * Callback called before any change of token type
     * Meaning if we have 2 different consecutive tokens sharing the same type,
     * start_token is called only once.
     * Example: on "12f", if the lexer considers each character separately,
     * we will have the following calls:
     * + start_document
     * + start_token before '1'
     * + write_token on '1'
     * + write_token on '2'
     * + end_token after '2'
     * + start_token before 'f'
     * + write_token on 'f'
     * + end_token after 'f'
     * + end_document
     */
    int (*start_token)(int token, String *, FormatterData *);
    /**
     * Callback called after any change of token type
     */
    int (*end_token)(int token, String *, FormatterData *);
    /**
     * Callback to write any token
     */
    int (*write_token)(String *, const char *, size_t, FormatterData *);
    /**
     * Size to allocate to create a Formatter
     * default is `sizeof(FormatterData)`
     */
    size_t data_size;
    /**
     * Available options
     */
    /*const*/ FormatterOption /*const */*options;
};

/**
 * Use a formatter (acts as an instance)
 */
struct Formatter {
    /**
     * Implementation on which depends the formatter (like its "class")
     */
    const FormatterImplementation *imp;
#ifdef TEST
    /**
     * Hashtable to associate option's name to its value
     */
    HashTable optmap;
#endif
    /**
     * (Variable) Space to store current values of options
     */
    OptionValue optvals[];
};

extern OptionValue *formatter_implementation_default_get_option_ptr(Formatter *, int, size_t, const char *, size_t);

#endif /* FORMATTER_H */
