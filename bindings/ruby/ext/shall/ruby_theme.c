#include "ruby_shall.h"

#include <shall/themes.h>

static VALUE themes;
static VALUE mTheme;
static VALUE cBaseTheme;

/* ruby internal */

/* :nodoc: */
static void create_theme_class_cb(const Theme *theme, void *data)
{
    VALUE cXTheme;

    cXTheme = rb_define_class_under(mTheme, theme_name(theme), cBaseTheme);
    rb_ary_push((VALUE) data, cXTheme);
}

/* class methods */

/* instance methods */

/* ========== Shall module functions ========== */

/*
 * call-seq:
 *   Shall::theme_by_name(string) -> (nil|Shall::Theme)
 *
 * Attempts to find out a theme from its name.
 * Returns +nil+ if there is no match else the theme
 * respoding to the given name.
 *
 *   Shall::theme_by_name('monokai') # => Shall::Theme::Monokai
 *   Shall::theme_by_name('foo')  # => nil
 */
static VALUE rb_theme_by_name(VALUE module, VALUE name)
{
    VALUE self;
    const Theme *theme;

    self = Qnil;
    Check_Type(name, T_STRING);
    if (NULL != (theme = theme_by_name(StringValueCStr(name)))) {
//         self = XXX;
    }
    /*if (Qnil == self) {
        rb_raise(rb_eNameError, "%s is not a known theme", name);
    }*/

    return self;
}

/* ========== Initialisation ========== */

void rb_shall_init_theme(void)
{
    mTheme = rb_define_module_under(mShall, "Theme");

    // Shall module functions
    rb_define_module_function(mShall, "theme_by_name", rb_theme_by_name, 1);

    // Shall::Theme::Base, theme superclass
    cBaseTheme = rb_define_class_under(mTheme, "Base", rb_cObject);

    // expose available themes as frozen Shall::THEMES constant
    themes = rb_ary_new();
    theme_each(create_theme_class_cb, (void *) themes);
    themes = rb_ary_freeze(themes);
    rb_define_const(mShall, "THEMES", themes);
}
