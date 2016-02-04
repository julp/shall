#include "ruby_shall.h"

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

static VALUE rb_style_fg(VALUE self)
{
    return Qnil;
}

static VALUE rb_style_bg(VALUE self)
{
    return Qnil;
}

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

    // fg, bg, fg=, bg=, bold=, italic=, underline=
    rb_define_method(cStyle, "bg?", rb_style_bg_set_p, 0);
    rb_define_method(cStyle, "fg?", rb_style_fg_set_p, 0);
    rb_define_method(cStyle, "bold?", rb_style_bold_p, 0);
    rb_define_method(cStyle, "italic?", rb_style_italic_p, 0);
    rb_define_method(cStyle, "underline?", rb_style_underline_p, 0);
}
