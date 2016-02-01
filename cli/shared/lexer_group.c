#include <stdlib.h>

#include "lexer_group.h"

#define INIT_GROUP_COUNT 8

LexerGroup *group_new(Lexer *lexer)
{
    LexerGroup *g;

    g = malloc(sizeof(*g) + INIT_GROUP_COUNT * sizeof(Lexer *));
    g->count = 0;
    g->allocated = INIT_GROUP_COUNT;
    g->lexers[g->count++] = lexer;

    return g;
}

void group_append(LexerGroup **g, Lexer *lexer)
{
    if (NULL == *g) {
        *g = group_new(lexer);
    } else {
        if ((*g)->count == (*g)->allocated) {
            *g = realloc(*g, sizeof(**g) + ((*g)->allocated <<= 1) * sizeof(Lexer *));
        }
        (*g)->lexers[(*g)->count++] = lexer;
    }
}

void group_destroy(void *data)
{
    size_t i;
    LexerGroup *g;

    g = (LexerGroup *) data;
    for (i = 0; i < g->count; i++) {
        lexer_destroy(g->lexers[i], (on_lexer_destroy_cb_t) lexer_destroy);
    }
    free(g);
}

void group_set_top(LexerGroup *g, Lexer *lexer)
{
    g->lexers[0] = lexer;
}
