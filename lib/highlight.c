/**
 * @file lib/highlight.c
 * @brief the place where all processing is done
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lexer.h"
#include "formatter.h"
#include "xtring.h"
#include "shall.h"
#undef TOKEN // TODO: conflict with "# define TOKEN(type)" of lexer.h
#include "tokens.h"
#include "dlist.h"
#include "hashtable.h"

#define RECURSION_LIMIT 8

#define T_IGNORE 258

#if 0 /* UNSUED */
#define FL_REWRITE_EOL_AS_CR (1<<0)
#define FL_REWRITE_EOL_AS_LF (1<<1)
// #define FL_REWRITE_EOL_AS_CRLF

enum {
    CR,
    LF,
    CR_LF,
};

static const named_element_t eol[] = {
    [ CR ] = { S("\r") },
    [ LF ] = { S("\n") },
    [ CR_LF ] = { S("\r\n") },
};
#endif

/* <temporary external stuffs from bison> */

enum { YYPUSH_MORE = 4 };
#if 0
extern yypstate *yypstate_new(void);
extern void yypstate_delete (yypstate *ps);

#define YYSIZE_T size_t
#define YYINITDEPTH 200
typedef int16_t yytype_int16;

struct yypstate
  {
    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;
    /* Used to determine if this is the first time this instance has
       been used.  */
    int yynew;
  };
#endif
/* </temporary external stuffs from bison> */

typedef struct {
    void *ps; // yypstate * (bison)
    Lexer *lexer;
    LexerData *data;
    bool user_lexer;
} LexerListElement;

typedef struct {
    int type; // delegation's type: DELEGATE_FULL or DELEGATE_UNTIL
    int state; // if != -1 state to set when DONE for the lexer who calls for a delegation
    const YYCTYPE *limits; // the end of the substring to lex/parse
    const LexerImplementation *imp;
} DelegationStackElement;

typedef struct {
    HashTable lexers;
    DList lexer_stack;
    DListElement *current_lexer_offset;
    int delegation_stack_offset;
    DelegationStackElement elements[16];
} ProcessingContext;

typedef struct {
    String *output;
    Formatter *fmt;
    int previous_token_type;
    LexerReturnValue buffer[10000], *cursor;
} OutputBufferContext;

static bool lexer_data_init(LexerData *data, size_t data_size)
{
    bzero(data, data_size);
    if (NULL != (data->state_stack = malloc(sizeof(*data->state_stack)))) {
        darray_init(data->state_stack, 0, sizeof(data->state));
    }

    return NULL != data->state_stack;
}

static void lexer_data_destroy(LexerData *data)
{
    if (NULL != data->state_stack) {
        darray_destroy(data->state_stack);
        free(data->state_stack);
    }
}

static void lexer_data_reset(LexerData *data)
{
    if (NULL != data->state_stack) {
        darray_clear(data->state_stack);
    }
    data->state = 0; // INITIAL
}

static void unregister_lexer(void *data)
{
    LexerListElement *lle;

    lle = (LexerListElement *) data;
    if (NULL != lle->lexer->imp->finalize) {
        lle->lexer->imp->finalize(lle->data);
    }
    if (NULL != lle->lexer->imp->yypstate_delete && NULL != lle->ps) {
        lle->lexer->imp->yypstate_delete(lle->ps);
    }
    lexer_data_destroy(lle->data);
    free(lle->data);
    if (!lle->user_lexer) {
        lexer_destroy(lle->lexer, NULL);
    }
    free(lle);
}

static LexerListElement *processing_context_init(ProcessingContext *pc, Lexer *lexer)
{
    pc->delegation_stack_offset = 0;
    dlist_init(&pc->lexer_stack, NULL);
    hashtable_init(&pc->lexers, 0, value_hash, value_equal, NULL, NULL, unregister_lexer);
    append_lexer(pc, lexer);
    pc->current_lexer_offset = pc->lexer_stack.head;

    return (LexerListElement *) pc->lexer_stack.head->data;
}

static void processing_context_destroy(ProcessingContext *pc)
{
    dlist_clear(&pc->lexer_stack);
    hashtable_destroy(&pc->lexers);
}

// TODO: forbids to stack a same LexerImplementation twice?
static void _add_lexer_real(ProcessingContext *pc, Lexer *lexer, bool UNUSED(prepend), bool keep)
{
    bool known;
    LexerListElement *lle;

    debug("[STACK] %s", lexer->imp->name);
    if (!(known = hashtable_direct_get(&pc->lexers, lexer->imp, &lle))) {
        lle = malloc(sizeof(*lle));
        lle->lexer = lexer;
        lle->user_lexer = keep;
        if (NULL != lexer->imp->yypstate_new) {
            lle->ps = lexer->imp->yypstate_new();
        } else {
            lle->ps = NULL;
        }
        lle->data = malloc(lexer->imp->data_size);
        lexer_data_init(lle->data, lexer->imp->data_size);
        hashtable_direct_put(&pc->lexers, 0, lexer->imp, lle, NULL);
    } else {
        // reset_lexer(data);?
        lexer_destroy(lexer, NULL);
        lexer = lle->lexer;
    }
//     if (prepend) {
        // TODO: dlist_insert_after(rv->current_lexer_offset)
//     } else {
        dlist_append(&pc->lexer_stack, lle);
//     }
# if 0
    debug("<XXX>");
    for (DListElement *e = pc->lexer_stack.head; NULL != e; e = e->next) {
        lle = (LexerListElement *) e->data;
        debug("XXX %s", lle->lexer->imp->name);
    }
    debug("</XXX>");
# endif
    // NOTE: the init callback may stack an other lexer
    // so make sure to register the current one BEFORE
    if (!known && NULL != lexer->imp->init) {
        lexer->imp->init(lexer->optvals, lle->data, pc);
    }
}

void prepend_lexer(void *pc, Lexer *lexer)
{
    _add_lexer_real((ProcessingContext *) pc, lexer, true, true);
}

void prepend_lexer_implementation(void *pc, const LexerImplementation *limp)
{
    _add_lexer_real((ProcessingContext *) pc, lexer_create(limp), true, false);
}

void unprepend_lexer(void *ctxt, const LexerImplementation *UNUSED(limp))
{
    ProcessingContext *pc;

    pc = (ProcessingContext *) ctxt;
    // TODO: dlist_remove_first
}

void append_lexer(void *ctxt, Lexer *lexer)
{
    _add_lexer_real((ProcessingContext *) ctxt, lexer, false, true);
}

// TODO: if we don't allow to stack twice a same lexer, only create a lexer (lexer_create) if one already exists
void append_lexer_implementation(void *ctxt, const LexerImplementation *limp)
{
    _add_lexer_real((ProcessingContext *) ctxt, lexer_create(limp), false, false);
}

void unappend_lexer(void *ctxt, const LexerImplementation *UNUSED(limp))
{
    ProcessingContext *pc;

    pc = (ProcessingContext *) ctxt;
    // reset_lexer(data);?
    // TODO: the lexer we unstack may be not the last
    // TODO: dlist_remove_last
    if (pc->lexer_stack.head != pc->lexer_stack.tail) {
        dlist_remove_tail(&pc->lexer_stack);
    }
}

static LexerListElement *delegation_push(ProcessingContext *pc, const LexerInput *yy, int type, int state)
{
    LexerListElement *lle_before_push, *lle_after_push;

    lle_before_push = (LexerListElement *) pc->current_lexer_offset->data;
    pc->current_lexer_offset = pc->current_lexer_offset->next;
    lle_after_push = (LexerListElement *) pc->current_lexer_offset->data;
    pc->elements[pc->delegation_stack_offset].imp = lle_after_push->lexer->imp;
    pc->elements[pc->delegation_stack_offset].type = type;
    pc->elements[pc->delegation_stack_offset].state = state;
    pc->elements[pc->delegation_stack_offset].limits = YYLIMIT;
    pc->delegation_stack_offset++;
    debug("PUSH (LEXER %s => %s)", lle_before_push->lexer->imp->name, lle_after_push->lexer->imp->name);

    return lle_after_push;
}

static LexerListElement *delegation_pop(ProcessingContext *pc, LexerInput *yy/*, const YYCTYPE **yylimit_before_pop*/)
{
    const YYCTYPE *yylimit_before_pop;
    LexerListElement *lle_before_pop, *lle_after_pop;

    lle_before_pop = (LexerListElement *) pc->current_lexer_offset->data;
    yylimit_before_pop = YYLIMIT;
#if 0
    if (NULL != yylimit_before_pop) {
        *yylimit_before_pop = YYLIMIT;
    }
#endif
    pc->current_lexer_offset = pc->current_lexer_offset->prev;
    lle_after_pop = (LexerListElement *) pc->current_lexer_offset->data;
    YYTEXT = YYCURSOR;
    --pc->delegation_stack_offset;
    YYLIMIT = pc->elements[pc->delegation_stack_offset].limits;
    if (DELEGATE_FULL == pc->elements[pc->delegation_stack_offset].type) {
        unappend_lexer(pc, pc->elements[pc->delegation_stack_offset].imp);
    }
    if (-1 != pc->elements[pc->delegation_stack_offset].state) {
        lle_after_pop->data->state = pc->elements[pc->delegation_stack_offset].state;
    }
    // TODO: offset are now wrong with buffering?
    debug("POP (LEXER %s => %s, YYLIMIT %zu => %zu)", lle_before_pop->lexer->imp->name, lle_after_pop->lexer->imp->name, SIZE_T(yylimit_before_pop - YYSRC), SIZE_T(YYLIMIT - YYSRC));

    return lle_after_pop;
}

static void buffer_init(OutputBufferContext *obc, Formatter *fmt)
{
    obc->fmt = fmt;
    obc->cursor = obc->buffer;
    obc->output = string_new();
    obc->previous_token_type = -1;
}

static void buffer_flush(OutputBufferContext *obc, bool hard_flush)
{
    LexerReturnValue *rvp;

    if (obc->cursor != obc->buffer) {
        for (rvp = obc->buffer; rvp < obc->cursor; rvp++) {
//             debug("[TOKEN] >%.*s< (%d) (%s <= %s)", (int) (rvp->yyend - rvp->yystart), rvp->yystart, rvp->token_value, tokens[rvp->token_default_type].name, -1 == obc->previous_token_type ? "\xe2\x88\x85" /* U+2205 */ : tokens[obc->previous_token_type].name);
            if (obc->previous_token_type != rvp->token_default_type) {
                if (-1 != obc->previous_token_type/* && IGNORABLE != obc->previous_token_type*/) {
                    obc->fmt->imp->end_token(obc->previous_token_type, obc->output, &obc->fmt->optvals);
                }
//                 if (IGNORABLE != rvp->token_default_type) {
                    obc->fmt->imp->start_token(rvp->token_default_type, obc->output, &obc->fmt->optvals);
//                 }
            }
            obc->fmt->imp->write_token(obc->output, (const char *) rvp->yystart, rvp->yyend - rvp->yystart, &obc->fmt->optvals);
            obc->previous_token_type = rvp->token_default_type;
        }
        if (hard_flush) {
            obc->fmt->imp->end_token(obc->previous_token_type, obc->output, &obc->fmt->optvals);
        }
        obc->cursor = obc->buffer;
//         STRING_APPEND_STRING(obc->output, "\n==== FLUSHED =====\n");
    }
}

/**
 * Highlight a string according to given lexer(s) and formatter
 *
 * @param src the input string
 * @param src_len its length
 * @param dst the output string
 * @param dst_len its length if not null
 * @param fmt the formatter to generate output from tokens
 * @param lexerc the number of lexers in lexerv (have to >= 1)
 * @param lexerv an array of lexers to tokenize the input string
 * (the top lexer have to be at index 0)
 *
 * @return zero if successfull
 */
SHALL_API int highlight_string(const char *src, size_t src_len, char **dst, size_t *dst_len, Formatter *fmt, size_t lexerc, Lexer **lexerv/*, uint32_t flags*/)
{
    String *buffer;
    bool skip_parser;
    LexerInput xx, *yy;
    ProcessingContext pc;
    int status, ret, what;
    LexerListElement *lle;
    OutputBufferContext obc;
    const YYCTYPE *prev_yycursor;
    size_t l, buffer_len, yycursor_unchanged;
    const char * const src_end = src + src_len;

    assert(lexerc > 0); // nothing to do, returns ""?
    assert(NULL != lexerv);

    ret = 0;
    yy = &xx;
    skip_parser = false;
    status = YYPUSH_MORE;
    yycursor_unchanged = 0;
    lle = processing_context_init(&pc, lexerv[0]);

    // skip UTF-8 BOM
    if (src_len >= STR_LEN(UTF8_BOM) && 0 == memcmp(src, UTF8_BOM, STR_LEN(UTF8_BOM))) {
        src += STR_LEN(UTF8_BOM);
        src_len -= STR_LEN(UTF8_BOM);
    }
# define previous_token_type obc.previous_token_type
    buffer_init(&obc, fmt);
    buffer = obc.output;
//     yy.bol = 1;
//     yy.lineno = 0;
    if (NULL != fmt->imp->start_document) {
        fmt->imp->start_document(buffer, &fmt->optvals);
    }
    // skip shebang
    if (src_len > STR_LEN(SHELLMAGIC) && 0 == memcmp(src, SHELLMAGIC, STR_LEN(SHELLMAGIC))) {
        const char *lf;

        for (lf = src; lf < src_end && ('\n' != *lf && '\r' != *lf); ++lf)
            ;
        if (lf < src_end) {
            // TODO: highlight it?
#if 1
            fmt->imp->start_token(previous_token_type = IGNORABLE, buffer, &fmt->optvals);
            fmt->imp->write_token(buffer, src, ++lf - src, &fmt->optvals);
#endif
            src_len -= lf - src;
            src = lf;
        }
    }
    YYSRC = (const YYCTYPE *) src;
    YYLIMIT = (const YYCTYPE *) src + src_len;
    prev_yycursor = YYCURSOR = (const YYCTYPE *) src;
    if (NULL != fmt->imp->start_lexing) {
        fmt->imp->start_lexing(lle->lexer->imp->name, buffer, &fmt->optvals);
    }
    for (l = 1; l < lexerc; l++) {
        append_lexer(&pc, lexerv[l]);
    }
    do {
        YYTEXT = YYCURSOR;
        what = lle->lexer->imp->yylex(yy, lle->data, lle->lexer->optvals, obc.cursor, (void *) &pc);
        // trivial safety against infinite loop
        if (YYCURSOR == prev_yycursor) {
            if (++yycursor_unchanged >= RECURSION_LIMIT) {
                // TODO: return a real error code
                ret = 1;
                debug("[ ERR ] recursion found with lexer %s on %.*s at offset %zu", lle->lexer->imp->name, (int) (obc.cursor->yyend - obc.cursor->yystart), obc.cursor->yystart, SIZE_T(obc.cursor->yyend - obc.cursor->yystart));
                goto abandon_or_done;
            }
        } else {
            yycursor_unchanged = 0;
            prev_yycursor = YYCURSOR;
        }
        if (HAS_FLAG(what, TOKEN)) {
            int yyleng;

retry_as_token:
            assert(obc.cursor->yystart != NULL);
            assert(obc.cursor->yyend != NULL);
            assert(obc.cursor->yyend >= obc.cursor->yystart);

            yyleng = obc.cursor->yyend - obc.cursor->yystart;
            if (!skip_parser && T_IGNORE != obc.cursor->token_value && NULL != lle->lexer->imp->yypush_parse) {
                status = lle->lexer->imp->yypush_parse(lle->ps, obc.cursor->token_value, &obc.cursor);
            } else {
                // TODO: merge current token with previous one to limit memory consumption? (if they have the same token_default_type)
            }
            /**
             * If we found a parse error, fallback to lexer alone
             * TODO: implémenter un rattrapage d'erreur au niveau de bison ?
             */
            if (1 == status) {
                debug("parse error on >%.*s< (%d) (%s)", (int) (obc.cursor->yyend - obc.cursor->yystart), obc.cursor->yystart, obc.cursor->token_value, tokens[obc.cursor->token_default_type].name);
                skip_parser = true;
                status = YYPUSH_MORE;
                goto abandon_or_done; // WARNING: temporary
            }
        }
        switch ((what & ~TOKEN)) {
            case DONE:
            {
                bool something_to_flush;

                something_to_flush = obc.cursor != obc.buffer;
                debug("[DONE] %s", lle->lexer->imp->name);
                buffer_flush(&obc, true);
#if 1
                if (something_to_flush && NULL != fmt->imp->end_lexing) {
                    fmt->imp->end_lexing(lle->lexer->imp->name, buffer, &fmt->optvals);
                    /**
                     * TODO: see note below
                     */
                    previous_token_type = IGNORABLE;
                }
#endif
                if (NULL != pc.current_lexer_offset->prev) {
                    lle = delegation_pop(&pc, yy);
#if 1
                    debug("something_to_flush for %s = %s", lle->lexer->imp->name, something_to_flush ? "true" : "false");
                    if (something_to_flush && NULL != fmt->imp->start_lexing) {
                        fmt->imp->start_lexing(lle->lexer->imp->name, buffer, &fmt->optvals);
                        /**
                         * TODO: see note below
                         */
                        previous_token_type = IGNORABLE;
                    }
#endif
                } else {
                    /**
                     * At this point, the lexer stack is empty and we likely haven't any token left
                     * so, end the loop
                     */
                    goto abandon_or_done;
                }
                break;
            }
            case DELEGATE_FULL:  // child/sub lexer "decides" on its own where to stop
            case DELEGATE_UNTIL: // parent lexer defined where child/sub lexer have to stop
            {
                LexerReturnValue copy;

                copy = *obc.cursor;
//                 buffer_flush(&obc, true);
                if (NULL != pc.current_lexer_offset->next) {
                    // TODO: offset are now wrong with buffering?
                    debug("PUSH YYLIMIT (%zu => %zu)", SIZE_T(YYLIMIT - YYSRC), SIZE_T(copy.child_limit - YYSRC));
                    lle = delegation_push(&pc, yy, what & ~TOKEN, -1);
#if 1
                    if (NULL != fmt->imp->start_lexing) {
                        fmt->imp->start_lexing(lle->lexer->imp->name, buffer, &fmt->optvals);
                        /**
                         * TODO: we should ask to the formatter if it needs us to force the reinitialization of the previous token
                         * ie if, for him, two successive tokens of the same type but for two different lexers have to be merged or
                         * not.
                         *
                         * Use the return value of the callback to do so?
                         */
                        previous_token_type = IGNORABLE;
                    }
#endif
                    if (DELEGATE_UNTIL == (what & ~TOKEN)) {
                        if (!HAS_FLAG(what, TOKEN)) {
                            YYCURSOR = YYTEXT; // come back before we read this token if delegation is active right now
                        }
                        YYLIMIT = copy.child_limit;
                    }
                    // for DELEGATE_(UNTIL|FULL)_AFTER_TOKEN, we need to keep (= advance the cursor) of our LexerReturnValue buffer
                    if (HAS_FLAG(what, TOKEN)) {
                        ++obc.cursor;
                    }
//                     buffer_flush(&obc, true);
                } else {
                    debug("lexer stack is empty");
                    YYCURSOR = copy.child_limit;
                    /**
                     * There is no other lexer in the stack to delegate the input as requested
                     * so treat the whole as a token of the default type the caller provided us.
                     **/
//                     what = TOKEN;
                    what &= ~(DELEGATE_FULL | DELEGATE_UNTIL);
                    obc.cursor->token_value = T_IGNORE;
                    obc.cursor->token_default_type = obc.cursor->delegation_fallback;
                    goto retry_as_token;
                }
                break;
            }
            case 0: // TOKEN &= ~TOKEN == 0
                ++obc.cursor;
                // alreay handled
                break;
            default:
                assert(0);
                break;
        }
    } while (YYPUSH_MORE == status/* || NULL == lle->lexer->imp->yypush_parse*/);
abandon_or_done:
    buffer_flush(&obc, true);
    // TODO: while (lle->current_lexer_offset-- > 0): end_lexing?
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }
    processing_context_destroy(&pc);

    // set result string
    buffer_len = buffer->len;
    *dst = string_orphan(buffer);
    if (NULL != dst_len) {
        *dst_len = buffer_len;
    }

    return ret;
}

/**
 * Generate a sample of highlighting for the given formatter
 *
 * @param dst the output string
 * @param dst_len its length if not null
 * @param fmt the formatter to generate output from tokens
 */
SHALL_API void highlight_sample(char **dst, size_t *dst_len, Formatter *fmt)
{
    String *buffer;
    size_t i, buffer_len;

    buffer = string_new();
    if (NULL != fmt->imp->start_document) {
        fmt->imp->start_document(buffer, &fmt->optvals);
    }
    for (i = 0; i < _TOKEN_COUNT; i++) {
        fmt->imp->start_token(i, buffer, &fmt->optvals);
        fmt->imp->write_token(buffer, tokens[i].name, tokens[i].name_len, &fmt->optvals);
        fmt->imp->write_token(buffer, S(": "), &fmt->optvals);
        fmt->imp->write_token(buffer, tokens[i].description, strlen(tokens[i].description), &fmt->optvals);
        fmt->imp->end_token(i, buffer, &fmt->optvals);

        fmt->imp->start_token(IGNORABLE, buffer, &fmt->optvals);
        fmt->imp->write_token(buffer, S("\n"), &fmt->optvals);
        fmt->imp->end_token(IGNORABLE, buffer, &fmt->optvals);
    }
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }
    // set result string
    buffer_len = buffer->len;
    *dst = string_orphan(buffer);
    if (NULL != dst_len) {
        *dst_len = buffer_len;
    }
}
