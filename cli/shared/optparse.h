#pragma once

enum {
    LEXER,
    FORMATTER,
    COUNT
};

typedef struct {
    const char *name; // TODO: remove const as we own it and it mey be convenient to allow modification
    size_t name_len;
    const char *value; // TODO: same, remove const
    size_t value_len;
} Option;

typedef struct {
    size_t options_len;
    size_t options_size;
    Option *options; // TODO: options[]?
} Options;

void option_parse(const char *, Option *);

void options_init(Options *);
void options_add(Options *, const char *);
void options_free(Options *);
void options_clear(Options *);
