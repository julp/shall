#include "ruby_shall.h"

#include <shall/themes.h>

static VALUE cColor;

typedef struct {
    Color *color;
} rb_color_object;

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct color_type;

# define UNWRAP_COLOR(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_color_object, &color_type, output)
#else
# define UNWRAP_COLOR(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_color_object, output)
#endif /* WITH_TYPED_DATA */

#ifdef WITH_TYPED_DATA
static void rb_color_free(void *ptr)
{
    rb_color_object *o;

    o = (rb_color_object *) ptr;
#else
static void rb_color_free(rb_color_object *o)
{
#endif /* WITH_TYPED_DATA */
    xfree(o);
}

#ifdef WITH_TYPED_DATA
static void rb_color_mark(void *ptr)
{
    rb_color_object *o;

    o = (rb_color_object *) ptr;
#else
static void rb_color_mark(rb_color_object *o)
{
#endif /* WITH_TYPED_DATA */
    // NOP for now
}

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct color_type = {
    "shall color",
    { rb_color_mark, rb_color_free, 0/*, { 0, 0 }*/ },
    NULL,
    NULL,
    0 | RUBY_TYPED_FREE_IMMEDIATELY
};
#endif /* WITH_TYPED_DATA */

#define UINT8_TO_FIXNUM(v) (UINT2NUM((unsigned int) v))
#define FIXNUM_TO_UINT8(v) ((uint8_t) NUM2UINT(v))

#define GET_COLOR(name, member) \
    static VALUE rb_color_##name(VALUE self) \
    { \
        rb_color_object *o; \
 \
        UNWRAP_COLOR(self, o); \
 \
        return UINT8_TO_FIXNUM(o->color->member); \
    }

GET_COLOR(red, r);
GET_COLOR(green, g);
GET_COLOR(blue, b);

#ifdef WITH_TYPED_DATA
# define BUILD_COLOR_OBJ(/*VALUE*/ o, /*rb_color_object **/c) \
    o = TypedData_Make_Struct(cColor, rb_color_object, &color_type, c)
#else
# define BUILD_COLOR_OBJ(/*VALUE*/ o, /*rb_color_object **/c) \
    o = Data_Make_Struct(cColor, rb_color_object, rb_color_mark, rb_color_free, c);
#endif /* WITH_TYPED_DATA */

VALUE ruby_color_from_color(Color *color)
{
    VALUE o;
    rb_color_object *c;

    BUILD_COLOR_OBJ(o, c);
//     memcpy(&c->color, color, sizeof(*color));
    c->color = color;

    return o;
}

#if 0
VALUE ruby_color_create_from_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    VALUE o;
    rb_color_object *c;

    BUILD_COLOR_OBJ(o, c);
    c->color = malloc(sizeof(Color));
    *c->color = { r, g, b };

    return o;
}
#endif

VALUE ruby_color_create_from_hexstring(VALUE string)
{
    VALUE o;
    rb_color_object *c;

    o = Qnil;

    return o;
}

void rb_shall_init_color(void)
{
    // Shall::Color
    cColor = rb_define_class_under(mShall, "Color", rb_cObject);
//     rb_undef_method(CLASS_OF(cColor), "new");

    rb_define_method(cColor, "red", rb_color_red, 0);
    rb_define_method(cColor, "green", rb_color_green, 0);
    rb_define_method(cColor, "blue", rb_color_blue, 0);

//     rb_define_singleton_method(cColor, "from_rgb", rb_color_create_from_rgb, 3);
//     rb_define_singleton_method(cColor, "from_hexstring", rb_color_create_from_hexstring, 1);
}
