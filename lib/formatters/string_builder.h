#pragma once

#define STRING_BUILDER_DECL(maxsize) \
    typedef struct { \
        char *w; /* offset to write */ \
        char buffer[maxsize]; \
    } string_builder_t;

#define STRING_BUILDER_INIT(sb) \
    do { \
        *(sb).buffer = '\0'; \
        (sb).w = (sb).buffer; \
    } while (0);

#define STRING_BUILDER_APPEND(sb, string) \
    (sb).w = stpcpy((sb).w, string)

#define STRING_BUILDER_APPEND_FORMATTED(sb, fmt, ...) \
    (sb).w += sprintf((sb).w, fmt, ## __VA_ARGS__)

#define STRING_BUILDER_DUP_INTO(sb, out) \
    do { \
        out = strdup((sb).buffer); \
        out##_len = (sb).w - (sb).buffer; \
    } while(0);
