#pragma once

#include "shall.h"

typedef struct {
    size_t count;
    size_t allocated;
    Lexer *lexers[];
} LexerGroup;

LexerGroup *group_new(Lexer *);
void group_append(LexerGroup **, Lexer *);
void group_destroy(void *);
