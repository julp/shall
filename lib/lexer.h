#pragma once

#include "types.h"
#include "options.h"
#include "darray.h"

#ifndef DOXYGEN
# define SHELLMAGIC "#!"
# define UTF8_BOM "\xEF\xBB\xBF"
#endif /* !DOXYGEN */

#define YYLEX_ARGS LexerInput *yy, LexerData *data, const OptionValue *options, LexerReturnValue *rv, void *ctxt
#define YYCTYPE  unsigned char
#define YYSRC    (yy->src)
#define YYTEXT   (yy->yytext)
// # define YYLINENO (yy->lineno)
#define YYLIMIT  (yy->limit)  /*re2c:define:YYLIMIT  = yy->limit;*/
#define YYCURSOR (yy->cursor) /*re2c:define:YYCURSOR = yy->cursor;*/
#define YYMARKER (yy->marker) /*re2c:define:YYMARKER = yy->marker;*/
#define YYLENG   (yy->cursor - yy->yytext)
#define YYFILL(n) \
    do { \
        if ((YYCURSOR/* + n*/) >= YYLIMIT) { \
            return 0; \
        } \
    } while(0);

#define yyless(x) \
    do { \
        YYCURSOR = YYTEXT + x; \
    } while (0);

#define STATE(name)  yyc##name /* re2c:condenumprefix */
#define BEGIN(state) YYSETCONDITION(STATE(state))
#define YYSTATE      YYGETCONDITION()

#define YYGETCONDITION()  data->state
#define YYSETCONDITION(s) data->state = s
#if 0
# define YYDEBUG(s, c) fprintf(stderr, "state: %d char: %c\n", s, c)
#else
# define YYDEBUG(s, c)
#endif

#define yymore() goto yymore_restart

#define IS_BOL \
    (YYSRC == YYTEXT || IS_NL(YYTEXT[-1]))

#define SIZE_T(v) ((size_t) (v))

enum {
    TOKEN = 1,
    DONE = 2,
//     DONE_AFTER_TOKEN = 3,
//     NEWLINE = 4,
    _DELEGATE = 8,
    DELEGATE_FULL = 10,
//     DELEGATE_FULL_AFTER_TOKEN = 11
    DELEGATE_UNTIL = 12
//     DELEGATE_UNTIL_AFTER_TOKEN = 13,
};

#ifdef DEBUG
# define TRACK_ORIGIN \
    do { \
        rv->return_line = __LINE__; \
        rv->return_file = __FILE__; \
        rv->return_func = __func__; \
    } while (0);
#else
# define TRACK_ORIGIN
#endif

#define DONE() \
    do { \
        TRACK_ORIGIN; \
        return DONE; \
    } while (0);

/*
#define NEWLINE(type) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = NULL; \
        rv->token_default_type = type; \
        return NEWLINE; \
    } while (0);
*/

#define TOKEN(type) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = NULL; \
        rv->token_default_type = type; \
        return TOKEN; \
    } while (0);

#define TOKEN_OUTSRC(type, start, end) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = start; \
        rv->yyend = end; \
        rv->token_value = 0; \
        rv->child_limit = NULL; \
        rv->token_default_type = type; \
        return TOKEN; \
    } while (0);

#define DONE_AFTER_TOKEN(type) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = NULL; \
        rv->token_default_type = type; \
        return DONE | TOKEN; \
    } while (0);

#define VALUED_TOKEN(type, value) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = value; \
        rv->child_limit = NULL; \
        rv->token_default_type = type; \
        return TOKEN; \
    } while (0);

#define DELEGATE_UNTIL(fallback) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = YYCURSOR; \
        rv->token_default_type = 0; \
        rv->delegation_fallback = fallback; \
        return DELEGATE_UNTIL; \
    } while (0);

#define DELEGATE_UNTIL_AFTER_TOKEN(limit, fallback, type) \
    do { \
        TRACK_ORIGIN; \
        rv->token_value = 0; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->child_limit = limit; \
        rv->token_default_type = type; \
        rv->delegation_fallback = fallback; \
        return DELEGATE_UNTIL | TOKEN; \
    } while (0);

#define DELEGATE_FULL(fallback) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = YYCURSOR; \
        rv->token_default_type = 0; \
        rv->delegation_fallback = fallback; \
        return DELEGATE_FULL; \
    } while (0);

#define DELEGATE_FULL_AFTER_TOKEN(limit, fallback, type) \
    do { \
        TRACK_ORIGIN; \
        rv->yystart = YYTEXT; \
        rv->yyend = YYCURSOR; \
        rv->token_value = 0; \
        rv->child_limit = limit; \
        rv->token_default_type = type; \
        rv->delegation_fallback = fallback; \
        return DELEGATE_FULL | TOKEN; \
    } while (0);

#define PUSH_STATE(new_state) \
    do { \
        darray_push(data->state_stack, &YYSTATE); \
        BEGIN(new_state); \
    } while (0);

#define POP_STATE() \
    do { \
        darray_pop(data->state_stack, &YYSTATE); \
    } while (0);

/*
#  define YYCTYPE        char
#  define YYPEEK()       *cursor
#  define YYSKIP()       ++cursor
#  define YYBACKUP()     marker = cursor
#  define YYBACKUPCTX()  ctxmarker = cursor
#  define YYRESTORE()    cursor = marker
#  define YYRESTORECTX() cursor = ctxmarker
#  define YYLESSTHAN(n)  limit - cursor < n
#  define YYFILL(n)      {}
*/

enum {
    NONE,
    CLASS,
    FUNCTION,
    NAMESPACE
};

#define S(s) s, STR_LEN(s)

/**
 * Common data of any lexer for its internal state
 *
 * May be overriden by each lexer
 */
typedef struct {
    /**
     * Lexer's state/condition
     */
    int state;
    /**
     * States stack
     */
    DArray *state_stack;
    /**
     * Mark next token kind
     */
    int next_label;
} LexerData;

/**
 * Different positionning stuffs for tokenization
 */
struct LexerInput {
    /**
     * Beginning of the string
     */
    YYCTYPE *src;
    /**
     * Current position
     */
    YYCTYPE *cursor;
    /**
     * End of string
     */
    YYCTYPE */* const*/ limit;
    /**
     * For internal use by re2c (its "marker")
     */
    YYCTYPE *marker;
//     size_t yyleng;
    /**
     * Begin of current token
     */
    YYCTYPE *yytext;
//     size_t lineno;
//     int bol;
};

#define LEXER_UNWRAP(optval) \
    (NULL == OPT_LEXUWF(optval) ? (Lexer *) OPT_LEXPTR(optval) : (OPT_LEXUWF(optval)(OPT_LEXPTR(optval))))

typedef struct LexerInput LexerInput;
typedef struct LexerReturnValue LexerReturnValue;

/**
 * Define a lexer (acts as a class)
 */
struct LexerImplementation {
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
     * Optionnal (may be NULL) callback to find out a suitable lexer
     * for an input string. Higher is the returned value more accurate
     * is the lexer
     */
    int (*analyse)(const char *, size_t);
    /**
     * Optionnal (may be NULL) callback for additionnal initialization before
     * beginning tokenization
     */
    void (*init)(const OptionValue *, LexerData *, void *);
    /**
     * Callback for lexing an input string into tokens
     */
    int (*yylex)(YYLEX_ARGS);
    /**
     * Optionnal (may be NULL) callback for additionnal deinitialization when
     * tokenisation is finised
     */
    void (*finalize)(LexerData *);
    /**
     * Size to allocate to create internal state for a lexer
     * Default is `sizeof(LexerData)`
     */
    size_t data_size;
    /**
     * Available options
     */
    /*const*/ LexerOption /*const */*options;
    /**
     * Implicit dependencies to other lexers
     */
    const struct LexerImplementation * const *implicit;
};

/**
 * Use a lexer (acts as an instance)
 */
struct Lexer {
    /**
     * Implementation on which depends the lexer (like its "class")
     */
    const LexerImplementation *imp;
    /**
     * Variable space to store current values of options
     */
    OptionValue optvals[];
};

// TODO: rename as YYSTYPE
struct LexerReturnValue {
    const YYCTYPE *yystart;
    const YYCTYPE *yyend;
    int token_value; // TOKEN
    int token_default_type; // TOKEN
    YYCTYPE *child_limit; // DELEGATE_*
    int delegation_fallback; // DELEGATE_*
#ifdef DEBUG
    int return_line;
    const char *return_file;
    const char *return_func;
#endif
};

#define YYSTRNCMP(x) \
    strcmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG/*, STR_LEN(x)*/)

#define YYSTRNCASECMP(x) \
    ascii_strcasecmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG/*, STR_LEN(x)*/)

#define NE(s) \
    { s, STR_LEN(s) }

#define IS_NL(c) \
    ('\r' == (c) || '\n' == (c))

#define HANDLE_CR_LF \
    do { \
        if ('\r' == *YYCURSOR && YYCURSOR < YYLIMIT && '\n' == YYCURSOR[1]) { \
            ++YYCURSOR; \
        } \
    } while (0);

typedef struct {
    const char *name;
    size_t name_len;
} named_element_t;

typedef struct {
    named_element_t ne;
    int type;
} typed_named_element_t;

int named_elements_cmp(const void *a, const void *b);
int named_elements_casecmp(const void *a, const void *b);

void append_lexer(void *, Lexer *);
void prepend_lexer(void *, Lexer *);
void append_lexer_implementation(void *, const LexerImplementation *);
void prepend_lexer_implementation(void *, const LexerImplementation *);
void unappend_lexer(void *, const LexerImplementation *);
void unprepend_lexer(void *, const LexerImplementation *);
