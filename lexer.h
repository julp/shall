#ifndef LEXER_H

# define LEXER_H

# include "option.h"
# include "darray.h"

/**
 * Common data of any lexer
 *
 * @note each member have to be ALIGNED(sizeof(OptionValue)),
 * this is a shortcut to union with an OptionValue and let us
 * to have a direct access to the final value.
 */
typedef struct {
    /**
     * Lexer's state/condition
     */
    int state ALIGNED(sizeof(OptionValue));
    /**
     * Stack
     */
    DArray *state_stack ALIGNED(sizeof(OptionValue));
    /**
     * Mark next token kind
     */
    int next_label ALIGNED(sizeof(OptionValue));
} LexerData;

/**
 * Lexer option
 */
typedef struct {
    /**
     * Option's name
     */
    const char *name;
    /**
     * Its type, one of OPT_TYPE_* constants
     */
    OptionType type;
    /**
     * Offset of the member into the struct that override LexerData
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
} LexerOption;

# define END_OF_LEXER_OPTIONS \
    { NULL, 0, 0, OPT_DEF_INT(0), NULL }

typedef struct LexerInput LexerInput;

/**
 * Define a lexer (acts as a class)
 */
typedef struct {
    /**
     * Official name of the lexer.
     * Should start with an uppercase letter and include only ASCII alphanumeric characters plus underscores
     */
    const char *name;
    /**
     * A string for self documentation
     */
    const char *docstr;
    /**
     * Optionnal (may be NULL) array of additionnal names 
     */
    const char * const *aliases;
    /**
     * Optionnal (may be NULL) array of glob patterns to associate a lexer
     * to different file extensions.
     * Example: "*.rb", "*.rake", ... for Ruby
     */
    const char * const *patterns;
    /**
     * Optionnal (may be NULL) array of MIME types appliable.
     * Eg: text/html for HTML.
     */
    const char * const *mimetypes;
    /**
     * Optionnal (may be NULL) array of (glob) patterns to recognize interpreter.
     * Not fullpaths, only basenames.
     * Example: "php", "php-cli", "php[45]" for PHP
     */
    const char * const *interpreters;
    /**
     * Optionnal (may be NULL) callback for additionnal initialization that can be
     * done elsewhere
     */
    void (*init)(LexerData *);
    /**
     * Optionnal (may be NULL) callback to find out a suitable lexer
     * for an input string. Higher is the returned value more accurate
     * is the lexer
     */
    int (*analyse)(const char *, size_t);
    /**
     * Callback for lexing an input string into tokens
     */
    int (*yylex)(LexerInput *, LexerData *);
    /**
     * Size to allocate to create a Lexer
     * default is `sizeof(LexerData)`
     */
    size_t data_size;
    /**
     * Available options
     */
    /*const*/ LexerOption /*const */*options;
} LexerImplementation;

/**
 * Use a lexer (acts as an instance)
 */
struct Lexer {
    /**
     * Implementation on which depends the lexer (like its "class")
     */
    const LexerImplementation *imp;
    /**
     * (Variable) Space to store current values of options
     */
    OptionValue optvals[];
};

# define SHALL_LEXER_DEFINED

#endif /* !LEXER_H */
