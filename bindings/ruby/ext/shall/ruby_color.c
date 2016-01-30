#include "ruby_shall.h"

#include <shall/themes.h>

static VALUE cColor;

static VALUE rb_color_red(VALUE self)
{
    return Qnil;
}

static VALUE rb_color_green(VALUE self)
{
    return Qnil;
}

static VALUE rb_color_blue(VALUE self)
{
    return Qnil;
}

void rb_shall_init_color(void)
{
    // Shall::Color
    cColor = rb_define_class_under(mShall, "Color", rb_cObject);

    rb_define_method(cColor, "red", rb_color_red, 0);
    rb_define_method(cColor, "green", rb_color_green, 0);
    rb_define_method(cColor, "blue", rb_color_blue, 0);
}
