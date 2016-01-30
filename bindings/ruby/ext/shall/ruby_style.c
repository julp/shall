#include "ruby_shall.h"

#include <shall/themes.h>

static VALUE cStyle;

static VALUE rb_style_fg(VALUE self)
{
    return Qnil;
}

static VALUE rb_style_bg(VALUE self)
{
    return Qnil;
}

static VALUE rb_style_bold_p(VALUE self)
{
    return Qfalse;
}

static VALUE rb_style_italic_p(VALUE self)
{
    return Qfalse;
}

static VALUE rb_style_underline_p(VALUE self)
{
    return Qfalse;
}

void rb_shall_init_style(void)
{
    // Shall::Style
    cStyle = rb_define_class_under(mShall, "Style", rb_cObject);

    // fg?, bg?, fg, bg, fg=, bg=, bold=, italic=, underline=
    rb_define_method(cStyle, "bold?", rb_style_bold_p, 0);
    rb_define_method(cStyle, "italic?", rb_style_italic_p, 0);
    rb_define_method(cStyle, "underline?", rb_style_underline_p, 0);
}
