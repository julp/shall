#include "ruby_shall.h"
#include "ruby_theme.h"
#include "ruby_style.h"

VALUE cBaseTheme;
static ID sInstance;
static VALUE themes;
static VALUE mTheme;

struct attr_desc {
    ID id;
    const char *name;
};

static union {
    struct {
        struct attr_desc fg;
        struct attr_desc bg;
        struct attr_desc bold;
        struct attr_desc italic;
        struct attr_desc underline;
    };
    struct attr_desc attrs[5];
} defined_attributes = {
    .fg = { .name = "fg" },
    .bg = { .name = "bg" },
    .bold = { .name = "bold" },
    .italic = { .name = "italic" },
    .underline = { .name = "underline" },
};

/* ruby internal */

#if 0
Possible approaches:
* Shall::BaseTheme inherit from Hash and at instanciation from a builtin theme, we populate the inner Hash
    + pro: easier to manage (gc/mark)?
    + cons: more work and implies a callback in shall library to translate ruby objects/values to C
    + unknown: how to manage sub Style objects?
* we work on Theme *, so, for Ruby themes, we malloc a Theme and for builtin instanciation, we make a copy? (const Theme * => Theme * to be modifiable)
    + pro: no modification to do in shall library
    + cons: more greedy? (maybe not)
    + cons: how to manage sub Style objects?
* separate builtins themes from ruby ones
    + cons: twice more work to do
    + cons: implies a callback in shall library to translate ruby objects/values to C
    + cons: have to translate builtins in ruby object

TODO:
* proxy (st_table)
    + colors
    + styles
* add a VALUE for name to rb_theme_object (and mark it)?
#endif

#ifdef WITH_TYPED_DATA
static void rb_theme_free(void *ptr)
{
    rb_theme_object *o;

    o = (rb_theme_object *) ptr;
#else
static void rb_theme_free(rb_theme_object *o)
{
#endif /* WITH_TYPED_DATA */
//     free(o->theme.name);
    xfree(o);
}

#ifdef WITH_TYPED_DATA
static void rb_theme_mark(void *ptr)
{
    rb_theme_object *o;

    o = (rb_theme_object *) ptr;
#else
static void rb_theme_mark(rb_theme_object *o)
{
#endif /* WITH_TYPED_DATA */
    // NOP for now
}

#ifdef WITH_TYPED_DATA
const struct rb_data_type_struct theme_type = {
    "shall theme",
    { rb_theme_mark, rb_theme_free, 0/*, { 0, 0 }*/ },
    NULL,
    NULL,
    0 | RUBY_TYPED_FREE_IMMEDIATELY
};
#endif /* WITH_TYPED_DATA */

/* :nodoc: */
static void create_theme_class_cb(const Theme *theme, void *data)
{
    VALUE cXTheme;

    cXTheme = rb_define_class_under(mTheme, theme_name(theme), cBaseTheme);
    rb_ary_push((VALUE) data, cXTheme);
}

static VALUE wrap_builtin_theme(VALUE klass, const Theme *theme)
{
    VALUE o;
    rb_theme_object *t;

#ifdef WITH_TYPED_DATA
    o = TypedData_Make_Struct(klass, rb_theme_object, &theme_type, t);
#else
    o = Data_Make_Struct(klass, rb_theme_object, rb_theme_mark, rb_theme_free, t);
#endif /* WITH_TYPED_DATA */
    if (NULL == theme) {
        bzero(&t->theme, sizeof(t->theme));
        t->theme.name = NULL;
    } else {
        memcpy(&t->theme, theme, sizeof(t->theme));
        t->theme.name = strdup(theme->name);
    }

    return o;
}

static VALUE rb_theme_alloc(VALUE klass)
{
    VALUE o;

    o = Qnil;
// debug("%s: %s", __func__, rb_class2name(klass));
    if (Qtrue == rb_ary_includes(themes, klass)) {
        const Theme *theme;

        if (NULL != (theme = theme_by_name(rb_class2name(klass) + STR_LEN("Shall::Theme::")))) {
            o = wrap_builtin_theme(klass, theme);
        }
    }
    if (NIL_P(o)) {
        o = wrap_builtin_theme(klass, NULL);
    }

    return o;
}

Theme *fetch_theme_instance(VALUE klass)
{
    VALUE self;
    rb_theme_object *t;

    self = rb_funcall(klass, sInstance, 0);
    UNWRAP_THEME(self, t);

    return &t->theme;
}

/* ========== instance methods ========== */

// name + export_as_css + ...

/*
 * call-seq:
 *   TODO
 */
static VALUE rb_theme_export_export_as_css(int argc, VALUE *argv, VALUE self)
{
    VALUE ret;
    char *css;
    VALUE scope;
    rb_theme_object *t;

    rb_scan_args(argc, argv, "01", &scope);
    if (!NIL_P(scope)) {
        Check_Type(scope, T_STRING);
    }
    UNWRAP_THEME(self, t);
    css = theme_export_as_css(&t->theme, Qnil == scope ? NULL : StringValueCStr(scope), true);

    ret = rb_usascii_str_new_cstr(css);
    free(css);

    return ret;
}

/*
 * call-seq:
 *   TODO
 */
static VALUE rb_theme_get_style(VALUE klass, VALUE type)
{
    int index;
    VALUE ret;
    Theme *theme;

    ret = Qnil;
    theme = fetch_theme_instance(klass);
    Check_Type(type, T_FIXNUM);
    index = NUM2INT(type);
    if (index >= 0 && index < _TOKEN_COUNT) {
        ret = ruby_create_style(&theme->styles[index]);
    }

    return ret;
}

static VALUE rb_theme_get_name(VALUE klass)
{
    Theme *theme;

    theme = fetch_theme_instance(klass);

    return rb_str_new_cstr(theme->name);
}

static VALUE rb_theme_set_name(VALUE klass, VALUE name)
{
    Theme *theme;

debug("%s", __func__);
    theme = fetch_theme_instance(klass);
    if (NULL != theme->name) {
        free(theme->name);
    }
    theme->name = strdup(StringValueCStr(name));

    return Qnil;
}

/* ========== class methods ========== */

/*
 * call-seq:
 *   Shall::MyTheme.style(Shall::Token, ..., attributes) -> nil
 *
 * Define style of given token type(s).
 *
 *   Shall::MyTheme.style(
 *     Shall::Token::Comment, Shall::Token::Comment::Multiline,
 *     fg: '#cccccc', underline: true
 *   ) # => nil
 */
static VALUE rb_theme_set_style(int argc, VALUE *argv, VALUE klass)
{
    size_t i;
    Theme *theme;
    Style style = { 0 };
    VALUE types, attributes;
    ID keywords[ARRAY_SIZE(defined_attributes.attrs)];
    VALUE values[ARRAY_SIZE(defined_attributes.attrs)];

//     types = attributes = Qnil; // according to extension.rdoc, rb_scan_args take care of this for unused arguments
    rb_scan_args(argc, argv, "*:", &types, &attributes);
    Check_Type(types, T_ARRAY);
    for (i = 0; i < ARRAY_SIZE(values); i++) {
        keywords[i] = defined_attributes.attrs[i].id;
    }
    // NOTE: rb_get_kwargs internally initializes each item of values to Qundef
    rb_get_kwargs(attributes, keywords, 0, ARRAY_SIZE(values), values);
//     bzero(&style, sizeof(style));
    for (i = 0; i < ARRAY_SIZE(defined_attributes.attrs); i++) {
        if (Qundef != values[i]/* && !NIL_P(values[i])*/) {
            // TODO: arguments checking?
            if (defined_attributes.attrs[i].id == defined_attributes.fg.id) {
                style.fg_set = true;
//                 style.fg = ???;
            } else if (defined_attributes.attrs[i].id == defined_attributes.bg.id) {
                style.bg_set = true;
//                 style.bg = ???;
            } else if (defined_attributes.attrs[i].id == defined_attributes.bold.id) {
                style.bold = true;
            } else if (defined_attributes.attrs[i].id == defined_attributes.italic.id) {
                style.italic = true;
            } else if (defined_attributes.attrs[i].id == defined_attributes.underline.id) {
                style.underline = true;
            }
        }
    }
    theme = fetch_theme_instance(klass);
    {
        long i;

        for (i = 0; i < RARRAY_LEN(types); i++) {
            VALUE v;
            int index;

            v = RARRAY_AREF(types, i);
            Check_Type(v, T_FIXNUM);
            index = NUM2INT(v);
            memcpy(&theme->styles[index], &style, sizeof(style));
        }
    }

    return Qnil;
}

/* ========== Shall module functions ========== */

/*
 * call-seq:
 *   Shall::theme_by_name(string) -> (nil|Shall::Theme)
 *
 * Attempts to find out a (builtin) theme from its name.
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
    if (T_SYMBOL == TYPE(name)) {
        name = rb_sym2str(name);
    }
    Check_Type(name, T_STRING);
    if (NULL != (theme = theme_by_name(StringValueCStr(name)))) {
        self = wrap_builtin_theme(rb_const_get_at(mTheme, rb_intern(theme_name(theme))), theme);
    }
    /*if (Qnil == self) {
        rb_raise(rb_eNameError, "%s is not a known theme", name);
    }*/

    return self;
}

/* ========== Initialisation ========== */

void rb_shall_init_theme(void)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(defined_attributes.attrs); i++) {
        defined_attributes.attrs[i].id = rb_intern(defined_attributes.attrs[i].name);
    }

    mTheme = rb_define_module_under(mShall, "Theme");

    // Shall module functions
    rb_define_module_function(mShall, "theme_by_name", rb_theme_by_name, 1);

    // Shall::Theme::Base, theme superclass
    cBaseTheme = rb_define_class_under(mTheme, "Base", rb_cObject); // inherit Hash?
#if 1
    {
        VALUE mSingleton;

        rb_require("singleton");
        mSingleton = rb_const_get(rb_cObject, rb_intern("Singleton"));
        rb_include_module(cBaseTheme, mSingleton);
        rb_funcall(mSingleton, rb_intern("included"), 1, cBaseTheme);

        sInstance = rb_intern("instance");
    }
#endif
    rb_define_alloc_func(cBaseTheme, rb_theme_alloc);
    rb_define_method(cBaseTheme, "export_as_css", rb_theme_export_export_as_css, -1); // class or instance?

    rb_define_singleton_method(cBaseTheme, "name", rb_theme_get_name, 0);
    rb_define_singleton_method(cBaseTheme, "set_name", rb_theme_set_name, 1);
    rb_define_singleton_method(cBaseTheme, "style", rb_theme_set_style, -1);
    rb_define_singleton_method(cBaseTheme, "get_style", rb_theme_get_style, 1);

    // expose available themes as frozen Shall::THEMES constant
    themes = rb_ary_new();
    theme_each(create_theme_class_cb, (void *) themes);
    themes = rb_ary_freeze(themes);
    rb_define_const(mShall, "THEMES", themes);
}
