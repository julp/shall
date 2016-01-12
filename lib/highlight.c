#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> // TODO: temporary?

#include "lexer.h"
#include "formatter.h"
#include "xtring.h"
#include "shall.h"
#undef TOKEN // TODO: conflict with "# define TOKEN(type)" of lexer.h
#include "tokens.h"

#define RECURSION_LIMIT 8

#define LEXER_FLAG_KEEP (1<<0)

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

/*
static void destroy_nonuser_lexer_data(void *rawdata)
{
    LexerData *data;

    data = (LexerData *) rawdata;
    if (!HAS_FLAG(data->flags, LEXER_FLAG_KEEP)) {
        lexer_data_destroy(data);
        free(data);
    } else {
        data->state = 0;
        darray_set_size(data->state_stack, 0);
    }
}
*/

#if 0
typedef struct {
    LexerInput *yy;
    String *buffer;
    Formatter *fmt;
    int prev_token;
    /**
     * HashTable of LexerData *:
     * - to reuse previous data associated to a LexerImplementation (if any, we create one)
     *   instead of creating one new each time
     * - free them when done, but only those are orphaned (ie not associated to a Lexer as
     *   internally we won't create a Lexer *)
     */
    HashTable *lexers;
} xxx;

static void handle_event(event_t event, void *data, ...)
{
    xxx *x;
    va_list ap;

    x = (xxx *) data;
    va_start(ap, data);
// debug("%s %d", __func__, event);
    switch (event) {
        case EVENT_DONE:
        {
// debug("DONE");
            if (-1 != x->prev_token) {
                x->fmt->imp->end_token(x->prev_token, x->buffer, &x->fmt->optvals);
            }
            break;
        }
        case EVENT_PUSH:
        {
            bool known;
            LexerData *data;
            const LexerImplementation *imp;

            known = false;
            imp = va_arg(ap, const LexerImplementation *);
// debug("PUSH %s", imp->name);
            if (NULL == (data = va_arg(ap, LexerData *))) {
                if (!(known = hashtable_direct_get(x->lexers, imp, &data))) {
                    data = malloc(imp->data_size);
                    lexer_data_init(data, imp->data_size);
                }
            } else {
//                 reset_lexer(data);
                SET_FLAG(data->flags, LEXER_FLAG_KEEP);
            }
            if (!known) {
                hashtable_direct_put(x->lexers, 0, imp, data, NULL);
            }
            if (NULL != x->fmt->imp->start_lexing) {
                x->fmt->imp->start_lexing(imp->name, x->buffer, &x->fmt->optvals);
            }
            x->prev_token = -1;
            imp->yylex(x->yy, data, handle_event, x);
// debug("POP %s", imp->name);
            // lexer emit a DONE here
//             free(data);
            if (NULL != x->fmt->imp->end_lexing) {
                x->fmt->imp->end_lexing(imp->name, x->buffer, &x->fmt->optvals);
            }
            x->prev_token = -1;
            break;
        }
        case EVENT_REPLAY:
        {
            bool known;
            LexerData *data;
            const LexerImplementation *imp;
            YYCTYPE *saved_limit/*, *saved_cursor*/;

            known = false;
//             saved_cursor = x->yy->cursor;
//             if (x->yy->cursor < x->yy->limit) {
                saved_limit = x->yy->limit;
                x->yy->cursor = va_arg(ap, YYCTYPE *);
                x->yy->limit = va_arg(ap, YYCTYPE *);
// //             if (x->yy->cursor < x->yy->limit) {
// debug("REPLAY (%ld) >%.*s<", x->yy->limit - x->yy->cursor, (int) (x->yy->limit - x->yy->cursor), x->yy->cursor);
                imp = va_arg(ap, const LexerImplementation *);
#if 1
                if (NULL == (data = va_arg(ap, LexerData *))) {
                    if (!(known = hashtable_direct_get(x->lexers, imp, &data))) {
                        data = malloc(imp->data_size);
                        lexer_data_init(data, imp->data_size);
                    }
                } else {
//                     reset_lexer(data);
                    SET_FLAG(data->flags, LEXER_FLAG_KEEP);
                }
                if (!known) {
                    hashtable_direct_put(x->lexers, 0, imp, data, NULL);
                }
#else
                if (NULL == (data = va_arg(ap, LexerData *))) {
                    data = malloc(imp->data_size);
                    lexer_data_init(data, imp->data_size);
                }/* else {
                    reset_lexer(data);
                }*/
#endif
                if (NULL != x->fmt->imp->start_lexing) {
                    x->fmt->imp->start_lexing(imp->name, x->buffer, &x->fmt->optvals);
                }
                imp->yylex(x->yy, data, handle_event, x);
                if (NULL != x->fmt->imp->end_lexing) {
                    x->fmt->imp->end_lexing(imp->name, x->buffer, &x->fmt->optvals);
                }
//                 x->fmt->imp->end_lexing(imp->name);
//                 x->yy->cursor = saved_cursor;
//                 x->yy->yytext = x->yy->cursor;
//                 /*x->yy->yytext = */x->yy->cursor = x->yy->limit;
                x->yy->limit = saved_limit;
                x->yy->yytext = x->yy->cursor;
// debug("AFTER REPLAY YYCURSOR = %c (%d), YYTEXT = %c (%d)", *x->yy->cursor, *x->yy->cursor, *x->yy->yytext, *x->yy->yytext);
// debug("AFTER REPLAY (%ld) >%.*s<", x->yy->limit - x->yy->cursor, (int) (x->yy->limit - x->yy->cursor), x->yy->cursor);
//             }
            break;
        }
        case EVENT_TOKEN:
        {
            int yyleng, token;

            token = va_arg(ap, int);
            yyleng = x->yy->cursor - x->yy->yytext;
            if (x->prev_token != token) {
                if (x->prev_token != -1) {
                    x->fmt->imp->end_token(x->prev_token, x->buffer, &x->fmt->optvals);
                }
                x->fmt->imp->start_token(token, x->buffer, &x->fmt->optvals);
            }
            x->fmt->imp->write_token(x->buffer, (char *) x->yy->yytext, yyleng, &x->fmt->optvals);
            x->prev_token = token;
            break;
        }
        default:
            assert(0);
            break;
    }
    va_end(ap);
}

SHALL_API size_t highlight_string(Lexer *lexer, Formatter *fmt, const char *src, char **dest)
{
    int prev;
    LexerInput yy;
    String *buffer;
    size_t src_len, buffer_len;

    src_len = strlen(src);
    const char * const src_end = src + src_len;
    // skip UTF-8 BOM
    if (src_len >= STR_LEN(UTF8_BOM) && 0 == memcmp(src, UTF8_BOM, STR_LEN(UTF8_BOM))) {
        src += STR_LEN(UTF8_BOM);
        src_len -= STR_LEN(UTF8_BOM);
    }
    buffer = string_new();
    prev = -1;
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
            fmt->imp->start_token(prev = IGNORABLE, buffer, &fmt->optvals);
            fmt->imp->write_token(buffer, src, ++lf - src, &fmt->optvals);
            src_len -= lf - src;
            src = lf;
        }
    }
    /*yy.marker = *//*yy.yytext = */yy.cursor = (YYCTYPE *) src;
    yy.limit = (YYCTYPE *) src + src_len;
    {
        xxx x;
        HashTable lexers;
//         OptionValue *secondary_optvalptr;

        x.buffer = buffer;
        x.fmt = fmt;
        x.prev_token = prev;
        x.yy = &yy;
        hashtable_init(&lexers, 0, value_hash, value_equal, NULL, NULL, destroy_nonuser_lexer_data);
#if 0
        if (OPT_TYPE_LEXER == lexer_get_option(lexer, "secondary", &secondary_optvalptr)) {
            Lexer *secondary;

            secondary = LEXER_UNWRAP(*secondary_optvalptr);
            if (NULL != secondary) {
                SET_FLAG(((LexerData *) (secondary->optvals))->flags, LEXER_FLAG_KEEP);
                hashtable_direct_put(&lexers, 0, (ht_hash_t) secondary->imp, (LexerData *) secondary->optvals, NULL);
            }
        }
#endif
        x.lexers = &lexers;
        if (NULL != x.fmt->imp->start_lexing) {
            x.fmt->imp->start_lexing(lexer->imp->name, x.buffer, &x.fmt->optvals);
        }
        lexer->imp->yylex(&yy, (LexerData *) lexer->optvals, handle_event, &x);
        if (NULL != x.fmt->imp->end_lexing) {
            x.fmt->imp->end_lexing(lexer->imp->name, x.buffer, &x.fmt->optvals);
        }
        hashtable_destroy(&lexers);
    }
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }

    buffer_len = buffer->len;
    *dest = string_orphan(buffer);

    return buffer_len;
}
#endif

typedef struct {
    Lexer *lexer;
    LexerData *data;
} LexerListElement;

static void _stack_lexer_real(LexerReturnValue *pdata, Lexer *lexer, bool keep)
{
    bool known;
    LexerData *ldata;

debug("[STACK] %s", lexer->imp->name);
    if (!(known = hashtable_direct_get(&pdata->lexers, lexer->imp, &ldata))) {
        ldata = malloc(lexer->imp->data_size);
        lexer_data_init(ldata, lexer->imp->data_size);
        hashtable_direct_put(&pdata->lexers, 0, lexer->imp, ldata, NULL);
    }
#ifndef WITHOUT_DLIST
    LexerListElement *lle;

    lle = malloc(sizeof(*lle));
    lle->lexer = lexer;
    lle->data = ldata;
    dlist_append(&pdata->lexer_stack, lle);
# if 0
    debug("<XXX>");
    for (DListElement *e = pdata->lexer_stack.head; NULL != e; e = e->next) {
        lle = (LexerListElement *) e->data;
        debug("XXX %s", lle->lexer->imp->name);
    }
    debug("</XXX>");
# endif
#else
    pdata->lexer_stack[pdata->lexer_stack_offset].lexer = lexer;
    pdata->lexer_stack[pdata->lexer_stack_offset].data = ldata;
    pdata->lexer_stack_offset++;
#endif
    // NOTE: the init callback may stack an other lexer
    // so make sure to register the current one BEFORE
    if (!known && NULL != lexer->imp->init) {
        lexer->imp->init(pdata, ldata, lexer->optvals);
    }
}

void stack_lexer(LexerReturnValue *pdata, Lexer *lexer)
{
    _stack_lexer_real(pdata, lexer, true);
}

void stack_lexer_implementation(LexerReturnValue *pdata, const LexerImplementation *limp)
{
    _stack_lexer_real(pdata, lexer_create(limp), false);
}

void unstack_lexer(LexerReturnValue *pdata, const LexerImplementation *limp)
{
    // reset_lexer(data);?
    // TODO: the lexer we unstack may be not the last
#ifndef WITHOUT_DLIST
    if (pdata->lexer_stack.head != pdata->lexer_stack.tail) {
        dlist_remove_tail(&pdata->lexer_stack);
    }
#else
debug("[UNSTACK] %s", pdata->lexer_stack[pdata->lexer_stack_offset].lexer->imp->name);
    pdata->lexer_stack_offset--;
#endif
}

typedef struct {
    YYCTYPE *limits[100];
    int delegation_stack_offset;
} delegation_stack;

static void delegation_init(delegation_stack *stack)
{
    stack->delegation_stack_offset = 0;
}

static void delegation_push(delegation_stack *stack, const LexerInput *yy)
{
    stack->limits[stack->delegation_stack_offset++] = YYLIMIT;
}

static void delegation_pop(delegation_stack *stack, LexerInput *yy)
{
    YYTEXT = YYCURSOR;
    YYLIMIT = stack->limits[--stack->delegation_stack_offset];
}

#define FL_REWRITE_EOL_AS_CR (1<<0)
#define FL_REWRITE_EOL_AS_LF (1<<1)
// #define FL_REWRITE_EOL_AS_CRLF

/**
 * Highlight string according to given lexer and formatter
 *
 * @param lexer the lexer to tokenize the input string
 * @param fmt the formatter to generate output from tokens
 * @param src the input string
 * @param src_len its length
 * @param dst the output string
 * @param dst_len its length if not null
 *
 * @return zero if successfull
 */
// better to have: int highlight_string(const char *src, size_t src_len, char **dst, size_t *dst_len, Formatter *fmt, Lexer *lexer, .../* a list of additionnal and already initialized lexer? */)
SHALL_API int highlight_string(Lexer *lexer, Formatter *fmt, const char *src, size_t src_len, char **dst, size_t *dst_len/*, uint32_t flags*/)
{
    String *buffer;
    LexerData *ldata;
    LexerInput xx, *yy;
    delegation_stack ds;
    LexerReturnValue rv; // => ProcessingData?
    Lexer *current_lexer;
    int what, prev, token;
    YYCTYPE *prev_yycursor;
    size_t buffer_len, yycursor_unchanged;
    const char * const src_end = src + src_len;

    yy = &xx;
    ldata = NULL;
    delegation_init(&ds);
    current_lexer = lexer;
    yycursor_unchanged = 0;
//     rv = (LexerReturnValue){0};
#ifndef WITHOUT_DLIST
    dlist_init(&rv.lexer_stack, NULL);
#else
    rv.lexer_stack_offset = rv.current_lexer_offset = 0;
#endif
    hashtable_init(&rv.lexers, 0, value_hash, value_equal, NULL, NULL, (DtorFunc) NULL/*lexer_data_destroy*/);

    // skip UTF-8 BOM
    if (src_len >= STR_LEN(UTF8_BOM) && 0 == memcmp(src, UTF8_BOM, STR_LEN(UTF8_BOM))) {
        src += STR_LEN(UTF8_BOM);
        src_len -= STR_LEN(UTF8_BOM);
    }
    buffer = string_new();
    prev = -1;
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
            fmt->imp->start_token(prev = IGNORABLE, buffer, &fmt->optvals);
            fmt->imp->write_token(buffer, src, ++lf - src, &fmt->optvals);
            src_len -= lf - src;
            src = lf;
        }
    }
    YYLIMIT = (YYCTYPE *) src + src_len;
    prev_yycursor = YYCURSOR = (YYCTYPE *) src;
    // TODO: prendre le token avant de rencontrer la fin
    // YYFILL ne doit pas permettre de prendre en compte le type du token au début sur un token de plusieurs caractères
    if (NULL != fmt->imp->start_lexing) {
        fmt->imp->start_lexing(current_lexer->imp->name, buffer, &fmt->optvals);
    }
    stack_lexer(&rv, current_lexer);
#ifndef WITHOUT_DLIST
    ldata = ((LexerListElement *) rv.lexer_stack.head->data)->data;
    rv.current_lexer_offset = rv.lexer_stack.head;
#else
    ldata = rv.lexer_stack[rv.current_lexer_offset].data;
#endif
#if 0
    // TODO/temporary
    if (/*0 == strcmp(current_lexer->imp->name, "PHP") || */0 == strcmp(current_lexer->imp->name, "ERB")) {
        extern const LexerImplementation html_lexer;

        stack_lexer_implementation(&rv, &html_lexer);
    }
#endif
    while (1) {
// debug("DO");
// debug("YYLEX %s %p %p", current_lexer->imp->name, current_lexer, ldata);
        what = current_lexer->imp->yylex(yy, ldata, current_lexer->optvals, &rv);
        // trivial safety against infinite loop
        if (YYCURSOR == prev_yycursor) {
            if (++yycursor_unchanged >= RECURSION_LIMIT) {
                // TODO: return an error code and add a size_t * argument to set, if not NULL, the output string length
                fputs("RECURSION FOUND\n", stderr);
                goto abandon_or_done;
            }
        } else {
            prev_yycursor = YYCURSOR;
        }
        switch (what) {
            case DONE:
debug("[DONE] %s", current_lexer->imp->name);
                if (prev != -1/* && prev != IGNORABLE*/) {
                    fmt->imp->end_token(token, buffer, &fmt->optvals);
                    prev = IGNORABLE;
                }
                if (NULL != fmt->imp->end_lexing) {
                    fmt->imp->end_lexing(current_lexer->imp->name, buffer, &fmt->optvals);
                    prev = IGNORABLE;
                }
//                 if (YYCURSOR < YYLIMIT) {
// debug("rv.current_lexer_offset = %d, rv.lexer_stack_offset = %d", rv.current_lexer_offset, rv.lexer_stack_offset);
#ifndef WITHOUT_DLIST
//                     if (rv.lexer_stack.head != rv.lexer_stack.tail) {
                    if (NULL != rv.current_lexer_offset->prev) {
#else
                    if (rv.current_lexer_offset > 0 && rv.current_lexer_offset <= rv.lexer_stack_offset) {
#endif
                        const LexerImplementation *imp_before_pop = current_lexer->imp;
#ifndef WITHOUT_DLIST
                        LexerListElement *lle;

                        rv.current_lexer_offset = rv.current_lexer_offset->prev;
                        lle = (LexerListElement *) rv.current_lexer_offset->data;
                        current_lexer = lle->lexer;
                        ldata = lle->data;
#else
                        --rv.current_lexer_offset;
                        current_lexer = rv.lexer_stack[rv.current_lexer_offset].lexer;
                        ldata = rv.lexer_stack[rv.current_lexer_offset].data;
#endif
debug("POP LEXER (%s => %s)", imp_before_pop->name, current_lexer->imp->name);
                        if (NULL != fmt->imp->start_lexing) {
                            fmt->imp->start_lexing(current_lexer->imp->name, buffer, &fmt->optvals);
                            prev = IGNORABLE;
                        }
                        YYCTYPE *yylimit_before_pop = YYLIMIT;
                        delegation_pop(&ds, yy);
debug("POP YYLIMIT (%zu => %zu)", SIZE_T(((const char *) yylimit_before_pop) - src), SIZE_T(((const char *) YYLIMIT) - src));
                        what = 1;
                        continue;
                    } else {
                        goto abandon_or_done;
                    }
//                 }
                break;
            case DELEGATE_FULL:  // child/sub lexer "decides" on its own where to stop
            case DELEGATE_UNTIL: // parent lexer defined where child/sub lexer have to stop
#ifndef WITHOUT_DLIST
                if (NULL != rv.current_lexer_offset->next) {
#else
debug("rv.current_lexer_offset = %d, rv.lexer_stack_offset = %d", rv.current_lexer_offset, rv.lexer_stack_offset);
                if (rv.current_lexer_offset >= 0 && rv.current_lexer_offset < rv.lexer_stack_offset) {
#endif
                    const LexerImplementation *imp_before_push = current_lexer->imp;
#ifndef WITHOUT_DLIST
                    LexerListElement *lle;

                    rv.current_lexer_offset = rv.current_lexer_offset->next;
                    lle = (LexerListElement *) rv.current_lexer_offset->data;
                    current_lexer = lle->lexer;
                    ldata = lle->data;
#else
                    ++rv.current_lexer_offset;
                    current_lexer = rv.lexer_stack[rv.current_lexer_offset].lexer;
                    ldata = rv.lexer_stack[rv.current_lexer_offset].data;
#endif
                    // TODO: pourquoi on n'a le même lexer ?!?
debug("PUSH LEXER (%s => %s) (%s:%s:%d)", imp_before_push->name, current_lexer->imp->name, rv.return_file, rv.return_func, rv.return_line);
                    if (NULL != fmt->imp->start_lexing) {
                        fmt->imp->start_lexing(current_lexer->imp->name, buffer, &fmt->optvals);
                        prev = IGNORABLE;
                    }
                    delegation_push(&ds, yy);
                    if (DELEGATE_UNTIL == what) {
                        YYCURSOR = YYTEXT; // ok?
debug("PUSH YYLIMIT (%zu => %zu)", SIZE_T(((const char *) YYLIMIT) - src), SIZE_T(((const char *) rv.child_limit) - src));
                        YYLIMIT = rv.child_limit;
                    }
                    break;
                } else {
debug("lexer stack is empty");
                    YYCURSOR = rv.child_limit;
                    /* no break here */
                }
            case TOKEN:
            {
                int yyleng;

                token = rv.token_default_type;
                yyleng = YYCURSOR - YYTEXT;
// debug("[TOKEN] %s: >%.*s< (%s %s)", current_lexer->imp->name, yyleng, YYTEXT, tokens[token].name, -1 == prev ? "\xe2\x88\x85" /* U+2205 */ : tokens[prev].name);
                if (prev != token) {
                    if (prev != -1/* && prev != IGNORABLE*/) {
                        fmt->imp->end_token(prev, buffer, &fmt->optvals);
                    }
//                     if (token != IGNORABLE) {
                        fmt->imp->start_token(token, buffer, &fmt->optvals);
//                     }
                }
                fmt->imp->write_token(buffer, (char *) YYTEXT, yyleng, &fmt->optvals);
                prev = token;
                break;
            }
            default:
                assert(0);
                break;
        }
    }
abandon_or_done:
    // TODO: while (rv.current_lexer_offset-- > 0): end_lexing?
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }
    hashtable_destroy(&rv.lexers);

    // set result string
    buffer_len = buffer->len;
    *dst = string_orphan(buffer);
    if (NULL != dst_len) {
        *dst_len = buffer_len;
    }

    return 0;
}
