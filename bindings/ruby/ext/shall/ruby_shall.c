#include "ruby_shall.h"
#include "ruby_color.h"
#include "ruby_style.h"
#include "ruby_theme.h"
#include "ruby_lexer.h"
#include "ruby_formatter.h"

static VALUE mToken;

/* ========== Shall module functions ========== */

/*
 * call-seq:
 *   Shall::highlight(string, lexer, formatter) -> string
 *
 * Highlights the given +string+, based on +lexer+ (a Shall::Lexer::Base object)
 * and +formatter+ (a Shall::Formatter::Base object)
 */
static VALUE rb_shall_highlight(VALUE module, VALUE string, VALUE lexer, VALUE formatter)
{
    char *dest;
    size_t dest_len;
    rb_lexer_object *l;
    rb_formatter_object *f;

    Check_Type(string, T_STRING);
    if (Qtrue != rb_obj_is_kind_of(lexer, cBaseLexer)) {
        return Qnil;
    }
    if (Qtrue != rb_obj_is_kind_of(formatter, cBaseFormatter)) {
        return Qnil;
    }
    UNWRAP_LEXER(lexer, l);
    UNWRAP_FORMATTER(formatter, f);
    highlight_string(StringValueCStr(string), RSTRING_LEN(string), &dest, &dest_len, f->formatter, 1, &l->lexer);

    return rb_utf8_str_new(dest, dest_len);
}

/*
 * call-seq:
 *   Shall::sample(formatter) -> string
 *
 * Generates a sample output for +formatter+ (a Shall::Formatter::Base object)
 */
static VALUE rb_shall_sample(VALUE module, VALUE formatter)
{
    char *dest;
    size_t dest_len;
    rb_formatter_object *f;

    if (Qtrue != rb_obj_is_kind_of(formatter, cBaseFormatter)) {
        return Qnil;
    }
    UNWRAP_FORMATTER(formatter, f);
    highlight_sample(&dest, &dest_len, f->formatter);

    return rb_utf8_str_new(dest, dest_len);
}

/* ========== Initialisation ========== */

/*
 * X
 */
void Init_shall(void)
{
    mShall = rb_define_module("Shall");
    mToken = rb_define_module_under(mShall, "Token");

#define TOKEN(constant, parent, description, cssclass) \
    rb_define_const(mToken, #constant, INT2FIX(constant));
#include <shall/keywords.h>
#undef TOKEN

    // Shall module functions
    rb_define_module_function(mShall, "sample", rb_shall_sample, 1);
    rb_define_module_function(mShall, "highlight", rb_shall_highlight, 3);

    {
#include <shall/version.h>
        Version v;
        char tmp[VERSION_STRING_MAX_LENGTH];

        version_get(v);
        version_to_string(v, tmp, STR_SIZE(tmp));
        rb_define_const(mShall, "VERSION", rb_usascii_str_new_cstr(tmp));
    }

    rb_shall_init_lexer();
    rb_shall_init_formatter();
    rb_shall_init_theme();
    rb_shall_init_style();
    rb_shall_init_color();
}
