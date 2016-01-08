#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lexer.h"
#include "formatter.h"
#include "hashtable.h"
#include "xtring.h"
#include "shall.h"
#undef TOKEN // TODO: conflict with "# define TOKEN(type)" of lexer.h
#include "tokens.h"

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

#define LEXER_FLAG_KEEP (1<<0)

static void destroy_nonuser_lexer_data(void *rawdata)
{
    LexerData *data;

    data = (LexerData *) rawdata;
    if (!HAS_FLAG(data->flags, LEXER_FLAG_KEEP)) {
        lexer_data_destroy(data);
        free(data);
    }
}

#if 0
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

            known = FALSE;
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

            known = FALSE;
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

void stack_lexer(LexerReturnValue *pdata, const LexerImplementation *limp, LexerData *ldata)
{
    bool known;

    known = FALSE;
    if (NULL == ldata) {
        if (!(known = hashtable_direct_get(&pdata->lexers, limp, &ldata))) {
            ldata = malloc(limp->data_size);
            lexer_data_init(ldata, limp->data_size);
        }
    } else {
//         reset_lexer(data);
        SET_FLAG(ldata->flags, LEXER_FLAG_KEEP);
    }
    if (!known) {
        hashtable_direct_put(&pdata->lexers, 0, limp, ldata, NULL);
    }
    pdata->lexer_stack[pdata->lexer_stack_offset].imp = limp;
    *(void **) &pdata->lexer_stack[pdata->lexer_stack_offset].optvals = (void **) &ldata;
    pdata->lexer_stack_offset++;
}

void unstack_lexer(LexerReturnValue *pdata, const LexerImplementation *limp)
{
    // reset_lexer(data);?
    pdata->lexer_stack_offset--;
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
    YYLIMIT = stack->limits[stack->delegation_stack_offset--];
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
 * @param dest the output string
 *
 * @return the length of *dest
 */
SHALL_API size_t highlight_string(Lexer *lexer, Formatter *fmt, const char *src, char **dest/* uint32_t flags*/)
{
    String *buffer;
    LexerInput xx, *yy;
    delegation_stack ds;
    LexerReturnValue rv; // => ProcessingData?
    int what, prev, token;
    size_t src_len, buffer_len;

    yy = &xx;
    delegation_init(&ds);
    rv.lexer_stack_offset = rv.current_lexer_offset = 0;
    hashtable_init(&rv.lexers, 0, value_hash, value_equal, NULL, NULL, destroy_nonuser_lexer_data);

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
            // TODO: highlight it?
            fmt->imp->start_token(prev = IGNORABLE, buffer, &fmt->optvals);
            fmt->imp->write_token(buffer, src, ++lf - src, &fmt->optvals);
            src_len -= lf - src;
            src = lf;
        }
    }
    YYCURSOR = (YYCTYPE *) src;
    YYLIMIT = (YYCTYPE *) src + src_len;
    // TODO: prendre le token avant de rencontrer la fin
    // YYFILL ne doit pas permettre de prendre en compte le type du token au début sur un token de plusieurs caractères
    if (NULL != fmt->imp->start_lexing) {
        fmt->imp->start_lexing(lexer->imp->name, buffer, &fmt->optvals);
    }
    do {
        // TODO: s'assurer qu'on ne fait pas plus de X itérations avec un YYCURSOR à la même position (boucle infinie)
        what = lexer->imp->yylex(yy, (LexerData *) lexer->optvals, &rv);
        switch (what) {
            case TOKEN:
            {
                int yyleng;

treat_as_token:
                token = rv.token_default_type;
                yyleng = YYCURSOR - YYTEXT;
// printf("[TOKEN] %s: >%.*s<\n", lexer->imp->name, yyleng, YYTEXT);
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
            case DONE:
printf("[DONE] %s\n", lexer->imp->name);
                if (prev != -1/* && prev != IGNORABLE*/) {
                    fmt->imp->end_token(token, buffer, &fmt->optvals);
                }
                if (NULL != fmt->imp->end_lexing) {
                    fmt->imp->end_lexing(lexer->imp->name, buffer, &fmt->optvals);
                }
                if (YYCURSOR < YYLIMIT) {
                    if (rv.current_lexer_offset > 0 && rv.current_lexer_offset <= rv.lexer_stack_offset) {
                        lexer = &rv.lexer_stack[--rv.current_lexer_offset];
                        if (NULL != fmt->imp->start_lexing) {
                            fmt->imp->start_lexing(lexer->imp->name, buffer, &fmt->optvals);
                        }
                        delegation_pop(&ds, yy);
                        continue;
                    }
                }
                break;
            case DELEGATE_FULL:  // child/sub lexer "decides" on its own where to stop
            case DELEGATE_UNTIL: // parent lexer defined where child/sub lexer have to stop
printf("[DELEGATE] FROM %s\n", lexer->imp->name);
//                 if (YYCURSOR < YYLIMIT) {
                    if (rv.current_lexer_offset >/*=*/ 0 && rv.current_lexer_offset < rv.lexer_stack_offset) {
                        lexer = &rv.lexer_stack[++rv.current_lexer_offset];
printf("[DELEGATE] TO %s\n", lexer->imp->name);
                        if (NULL != fmt->imp->start_lexing) {
                            fmt->imp->start_lexing(lexer->imp->name, buffer, &fmt->optvals);
                        }
                        delegation_push(&ds, yy);
                        if (DELEGATE_UNTIL == what) {
printf("YYLIMIT changed\n");
                            YYLIMIT = rv.child_limit;
                        }
                        continue;
                    } else {
printf("lexer stack is empty\n");
                        YYCURSOR = rv.child_limit;
                        goto treat_as_token;
                    }
//                 }
                break;
        }
    } while (what > 0);
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }
    hashtable_destroy(&rv.lexers);

    buffer_len = buffer->len;
    *dest = string_orphan(buffer);

    return buffer_len;
}
