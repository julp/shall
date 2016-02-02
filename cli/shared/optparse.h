#pragma once

enum {
    LEXER,
    FORMATTER,
    COUNT
};

typedef struct {
    char *name;
    size_t name_len;
    char *value;
    size_t value_len;
} Option;

typedef struct {
    size_t options_len;
    size_t options_size;
    Option *options;
} OptionsStore;

void option_parse(const char *, Option *);

void options_store_init(OptionsStore *);
void options_store_add(OptionsStore *, const char *);
void options_store_free(OptionsStore *);
void options_store_clear(OptionsStore *);
