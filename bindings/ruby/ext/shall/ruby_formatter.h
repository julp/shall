#pragma once

VALUE cBaseFormatter;

typedef struct {
    Formatter *formatter;
} rb_formatter_object;

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct formatter_type;

# define UNWRAP_FORMATTER(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_formatter_object, &formatter_type, output)
#else
# define UNWRAP_FORMATTER(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_formatter_object, output)
#endif /* WITH_TYPED_DATA */

void rb_shall_init_formatter(void);
