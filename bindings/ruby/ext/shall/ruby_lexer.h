#pragma once

VALUE cBaseLexer;

typedef struct {
    Lexer *lexer;
} rb_lexer_object;

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct lexer_type;

# define UNWRAP_LEXER(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_lexer_object, &lexer_type, output)
#else
# define UNWRAP_LEXER(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_lexer_object, output)
#endif /* WITH_TYPED_DATA */

Lexer *ruby_lexer_unwrap(void *);

void rb_shall_init_lexer(void);
