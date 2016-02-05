#include "ruby_shall.h"
#include "ruby_color.h"

#include <shall/themes.h>

static VALUE cStyle;

typedef struct {
    Style *style; // TODO: should not be a pointer?
} rb_style_object;

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct style_type;

# define UNWRAP_STYLE(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_style_object, &style_type, output)
#else
# define UNWRAP_STYLE(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_style_object, output)
#endif /* WITH_TYPED_DATA */

#ifdef WITH_TYPED_DATA
static void rb_style_free(void *ptr)
{
    rb_style_object *o;

    o = (rb_style_object *) ptr;
#else
static void rb_style_free(rb_style_object *o)
{
#endif /* WITH_TYPED_DATA */
    xfree(o);
}

#ifdef WITH_TYPED_DATA
static void rb_style_mark(void *ptr)
{
    rb_style_object *o;

    o = (rb_style_object *) ptr;
#else
static void rb_style_mark(rb_style_object *o)
{
#endif /* WITH_TYPED_DATA */
    // NOP for now
}

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct style_type = {
    "shall style",
    { rb_style_mark, rb_style_free, 0/*, { 0, 0 }*/ },
    NULL,
    NULL,
    0 | RUBY_TYPED_FREE_IMMEDIATELY
};
#endif /* WITH_TYPED_DATA */

#define HAS_ATTRIBUTE(name) \
    static VALUE rb_style_##name##_p(VALUE self) \
    { \
        rb_style_object *s; \
 \
        UNWRAP_STYLE(self, s); \
 \
        return s->style->name ? Qtrue : Qfalse; \
    }

HAS_ATTRIBUTE(bg_set);
HAS_ATTRIBUTE(fg_set);
HAS_ATTRIBUTE(bold);
HAS_ATTRIBUTE(italic);
HAS_ATTRIBUTE(underline);

#define COLOR_ATTRIBUTE(attr) \
    static VALUE rb_style_##attr##_get(VALUE self) \
    { \
        rb_style_object *s; \
 \
        UNWRAP_STYLE(self, s); \
 \
        return ruby_color_from_color(&s->style->attr); \
    } \
 \
    static VALUE rb_style_##attr##_set(VALUE self, VALUE value) \
    { \
        rb_style_object *s; \
 \
        UNWRAP_STYLE(self, s); \
        if (NIL_P(value)) { \
            s->style->attr##_set = false; \
            /* bzero ? */ \
        } else { \
            Check_Type(value, T_STRING); \
            color_parse_hexstring(StringValueCStr(value), RSTRING_LEN(value), &s->style->attr); \
        } \
 \
        return Qnil; \
    }

COLOR_ATTRIBUTE(fg);
COLOR_ATTRIBUTE(bg);

#define SET_BOOL_ATTRIBUTE(name) \
    static VALUE rb_style_##name##_set(VALUE self, VALUE value) \
    { \
        rb_style_object *s; \
 \
        UNWRAP_STYLE(self, s); \
        s->style->name = RTEST(value); \
 \
        return Qnil; \
    }

SET_BOOL_ATTRIBUTE(bold);
SET_BOOL_ATTRIBUTE(italic);
SET_BOOL_ATTRIBUTE(underline);

VALUE ruby_create_style(Style *style)
{
    VALUE o;
    rb_style_object *s;

#ifdef WITH_TYPED_DATA
    o = TypedData_Make_Struct(cStyle, rb_style_object, &style_type, s);
#else
    o = Data_Make_Struct(cStyle, rb_style_object, rb_style_mark, rb_style_free, s);
#endif /* WITH_TYPED_DATA */
    s->style = style;
//     memcpy(&s->style, style, sizeof(s->style));

    return o;
}

void rb_shall_init_style(void)
{
    // Shall::Style
    cStyle = rb_define_class_under(mShall, "Style", rb_cObject);
    rb_undef_method(CLASS_OF(cStyle), "new");

    rb_define_method(cStyle, "bg", rb_style_bg_get, 0);
    rb_define_method(cStyle, "fg", rb_style_fg_get, 0);

    rb_define_method(cStyle, "bg=", rb_style_bg_set, 1);
    rb_define_method(cStyle, "fg=", rb_style_fg_set, 1);
    rb_define_method(cStyle, "bold=", rb_style_bold_set, 1);
    rb_define_method(cStyle, "italic=", rb_style_italic_set, 1);
    rb_define_method(cStyle, "underline=", rb_style_underline_set, 1);

    rb_define_method(cStyle, "bg?", rb_style_bg_set_p, 0);
    rb_define_method(cStyle, "fg?", rb_style_fg_set_p, 0);
    rb_define_method(cStyle, "bold?", rb_style_bold_p, 0);
    rb_define_method(cStyle, "italic?", rb_style_italic_p, 0);
    rb_define_method(cStyle, "underline?", rb_style_underline_p, 0);
}
