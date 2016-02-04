#pragma once

#include <shall/themes.h>

VALUE cBaseTheme;

typedef struct {
    Theme theme;
} rb_theme_object;

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct theme_type;

# define UNWRAP_THEME(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_theme_object, &theme_type, output)
#else
# define UNWRAP_THEME(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_theme_object, output)
#endif /* WITH_TYPED_DATA */

Theme *fetch_theme_instance(VALUE);

void rb_shall_init_theme(void);
