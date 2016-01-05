#ifndef OPTPARSE_H

# define OPTPARSE_H

enum {
    LEXER,
    FORMATTER,
    COUNT
};

typedef struct {
    const char *name;
    size_t name_len;
    const char *value;
    size_t value_len;
} Option;

typedef struct {
    size_t options_len;
    size_t options_size;
    Option *options;
} Options;

void options_init(Options *);
void options_add(Options *, const char *);
void options_free(Options *);

#endif /* !OPTPARSE_H */
