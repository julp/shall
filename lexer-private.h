#ifndef LEXER_PRIVATE_H

# define LEXER_PRIVATE_H

/* this header is only for lexers */

# define YYLEX_ARGS LexerInput *yy, LexerData *data
# define YYCTYPE  unsigned char
# define YYTEXT   (yy->yytext)
// # define YYLINENO (yy->lineno)
# define YYLIMIT  (yy->limit)  /*re2c:define:YYLIMIT  = yy->limit;*/
# define YYCURSOR (yy->cursor) /*re2c:define:YYCURSOR = yy->cursor;*/
# define YYMARKER (yy->marker) /*re2c:define:YYMARKER = yy->marker;*/
# define YYLENG   (yy->cursor - yy->yytext)
# define YYFILL(n) \
    do { \
        if ((YYCURSOR/* + n*/) >= YYLIMIT) { \
            return 0; \
        } \
    } while(0);

#define yyless(x) \
    do { \
        YYCURSOR = YYTEXT + x; \
    } while (0);

# define STATE(name)  yyc##name /* re2c:condenumprefix */
# define BEGIN(state) YYSETCONDITION(STATE(state))
# define YYSTATE      YYGETCONDITION()

# define YYGETCONDITION()  data->state
# define YYSETCONDITION(s) data->state = s
# if 0
#  define YYDEBUG(s, c) fprintf(stderr, "state: %d char: %c\n", s, c)
# else
#  define YYDEBUG(s, c)
# endif

# define yymore() goto yymore_restart

#define SAVE_STATE    mydata->saved_state = YYSTATE
#define RESTORE_STATE YYSETCONDITION(mydata->saved_state)

#define PUSH_STATE(new_state) \
    do { \
        darray_push(&mydata->state_stack, &YYSTATE); \
        BEGIN(new_state); \
    } while (0);

#define POP_STATE() \
    do { \
        darray_pop(&mydata->state_stack, &YYSTATE); \
    } while (0);

#define YYSTRNCMP(x) \
    strncmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG, STR_LEN(x))

#define NE(s) { s, STR_LEN(s) }

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


/*
#   define YYCTYPE        char
#   define YYPEEK()       *cursor
#   define YYSKIP()       ++cursor
#   define YYBACKUP()     marker = cursor
#   define YYBACKUPCTX()  ctxmarker = cursor
#   define YYRESTORE()    cursor = marker
#   define YYRESTORECTX() cursor = ctxmarker
#   define YYLESSTHAN(n)  limit - cursor < n
#   define YYFILL(n)      {}
*/

enum {
    NONE,
    CLASS,
    FUNCTION,
    NAMESPACE
};

/**
 * Different positionning stuffs for tokenization
 */
struct LexerInput {
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

# define LEXER_UNWRAP(optval) \
    (NULL == OPT_LEXUWF(optval) ? (Lexer *) OPT_LEXPTR(optval) : (OPT_LEXUWF(optval)(OPT_LEXPTR(optval))))

#endif /* !LEXER_PRIVATE_H */
