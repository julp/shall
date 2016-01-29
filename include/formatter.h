#pragma once

#include "types.h"
#include "options.h"
#include "xtring.h"
#ifdef TEST
# include "hashtable.h"
#endif /* TEST */

typedef void FormatterData;

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
     * Callback called when a lexer is pushed
     */
    int (*start_lexing)(const char *, String *, FormatterData *);
    /**
     * Callback called when a lexer is poped
     */
    int (*end_lexing)(const char *, String *, FormatterData *);
    /**
     * Callback called at formatter's destruction
     */
    void (*finalize)(FormatterData *);
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
     * A pointer, for the user, to associate data to Formatter instance
     */
    void *userdata;
    /**
     * (Variable) Space to store current values of options
     */
    OptionValue optvals[];
};

OptionValue *formatter_implementation_default_get_option_ptr(Formatter *, int, size_t, const char *, size_t);
