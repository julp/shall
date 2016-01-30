#include "ruby_shall.h"
#include "ruby_lexer.h"

VALUE cBaseLexer;
static VALUE lexers;
static VALUE mLexer;

/* ruby internal */

#ifdef WITH_TYPED_DATA
static void rb_lexer_free(void *ptr)
{
    rb_lexer_object *o;

    o = (rb_lexer_object *) ptr;
#else
static void rb_lexer_free(rb_lexer_object *o)
{
#endif /* WITH_TYPED_DATA */
    lexer_destroy(o->lexer, NULL);
    xfree(o);
}

#ifdef WITH_TYPED_DATA
static void rb_lexer_mark(void *ptr)
{
    rb_lexer_object *o;

    o = (rb_lexer_object *) ptr;
#else
static void rb_lexer_mark(rb_lexer_object *o)
{
#endif /* WITH_TYPED_DATA */
    lexer_each_sublexers(o->lexer, (on_lexer_destroy_cb_t) rb_gc_mark);
}

#ifdef WITH_TYPED_DATA
#if 0
static size_t rb_lexer_size(void *ptr)
{
    return sizeof(rb_lexer_object) /*+ lexer->imp->data_size*/;
}
#endif

const struct rb_data_type_struct lexer_type = {
    "shall lexer",
    { rb_lexer_mark, rb_lexer_free, 0/*, { 0, 0 }*/ },
    NULL,
    NULL,
    0 | RUBY_TYPED_FREE_IMMEDIATELY
};
#endif /* WITH_TYPED_DATA */

/* helpers */

static const LexerImplementation *klass_to_lexer_implementation(VALUE klass)
{
    const LexerImplementation *imp;

    if (Qtrue != rb_ary_includes(lexers, klass)) {
        rb_raise(rb_eRuntimeError, "'%s' is not a valid predefined shall lexer", rb_class2name(klass));
        return NULL;
    }
    imp = lexer_implementation_by_name(rb_class2name(klass) + STR_LEN("Shall::Lexer::"));

    return imp;
}

/*
static VALUE lexer_implementation_to_klass(const LexerImplementation *imp)
{
    return rb_const_get_at(mLexer, rb_intern(lexer_implementation_name(imp)));
}
*/

static VALUE rb_lexer_alloc(VALUE klass)
{
    VALUE o;
    rb_lexer_object *l;
    const LexerImplementation *imp;

#ifdef WITH_TYPED_DATA
    o = TypedData_Make_Struct(klass, rb_lexer_object, &lexer_type, l);
#else
    o = Data_Make_Struct(klass, rb_lexer_object, rb_lexer_mark, rb_lexer_free, l);
#endif /* WITH_TYPED_DATA */
    imp = klass_to_lexer_implementation(klass);
    // TODO: do something if NULL == imp
    l->lexer = lexer_create(imp);

    return o;
}

Lexer *ruby_lexer_unwrap(void *object)
{
    rb_lexer_object *o;

    UNWRAP_LEXER((VALUE) object, o);

    return o->lexer;
}

static VALUE rb_lexer_initialize(int, VALUE *, VALUE);

/* :nodoc: */
static void create_lexer_class_cb(const LexerImplementation *imp, void *data)
{
    VALUE cXLexer, ary;
    const char *imp_name;

    imp_name = lexer_implementation_name(imp);
    cXLexer = rb_define_class_under(mLexer, imp_name, cBaseLexer);
    rb_define_alloc_func(cXLexer, rb_lexer_alloc);
    rb_define_method(cXLexer, "initialize", rb_lexer_initialize, -1);
//     cXLexer = rb_obj_freeze(cXLexer);
    rb_ary_push((VALUE) data, cXLexer);

#ifndef WITHOUT_LEXER_CONST
    rb_define_const(cXLexer, "NAME", rb_usascii_str_new_cstr(imp_name));
    ary = rb_ary_new();
    lexer_implementation_each_alias(imp, ary_push_string_cb, (void *) ary);
    rb_define_const(cXLexer, "ALIASES", ary);
    ary = rb_ary_new();
    lexer_implementation_each_mimetype(imp, ary_push_string_cb, (void *) ary);
    rb_define_const(cXLexer, "MIMETYPES", ary);
#endif
}

/* ========== class methods ========== */

/*
 * call-seq:
 *   Shall::Lexer::Foo.name -> string
 *
 * Returns the official name of a lexer
 *
 *   Shall::Lexer::Foo.name # => "Foo"
 */
static VALUE rb_lexer_name(VALUE klass)
{
    const LexerImplementation *imp;

    imp = klass_to_lexer_implementation(klass);

    return rb_usascii_str_new_cstr(lexer_implementation_name(imp));
}

/*
 * call-seq:
 *   Shall::Lexer::Foo.mimetypes -> array
 *
 * Returns the MIME types associated to a lexer
 *
 *   Shall::Lexer::Text.mimetypes # => [ "text/plain" ]
 */
static VALUE rb_lexer_mimetypes(VALUE klass)
{
    VALUE ret;
    const LexerImplementation *imp;

    ret = rb_ary_new();
    imp = klass_to_lexer_implementation(klass);
    lexer_implementation_each_mimetype(imp, ary_push_string_cb, (void *) ret);

    return ret;
}

/*
 * call-seq:
 *   Shall::Lexer::Foo.aliases -> array
 *
 * Returns alternative names of a lexer
 *
 *   Shall::Lexer::PHP.aliases # => [ "php4", "php5", ... ]
 */
static VALUE rb_lexer_aliases(VALUE klass)
{
    VALUE ret;
    const LexerImplementation *imp;

    ret = rb_ary_new();
    imp = klass_to_lexer_implementation(klass);
    lexer_implementation_each_alias(imp, ary_push_string_cb, (void *) ret);

    return ret;
}

/* instance methods */

/*
 * call-seq:
 *   Shall::Lexer::Foo.new(options = {}) -> Shall::Lexer::Foo
 *
 * Creates a new lexer with optionnal +options+
 *
 *   Shall::Lexer::PHP.new(start_inline: true, short_tags: false) # => Shall::Lexer::PHP object
 */
static VALUE rb_lexer_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE options;

    options = Qnil;
    rb_scan_args(argc, argv, "01", &options);
    rb_set_options(self, options);

    return self;
}

/*
 * call-seq:
 *   lexer.get_option(string) -> (nil|true|false|num|string|Shall::Lexer::Base)
 *
 * Returns the current value of the given option, by its <i>name</i>, of a lexer.
 * nil is returned if the option is unknown to the lexer.
 *
 *   lexer.get_option('start_inline') # => false
 */
static VALUE rb_lexer_get_option(VALUE self, VALUE name)
{
    rb_lexer_object *o;
    OptionType type;
    OptionValue *optvalptr;

    if (T_SYMBOL == TYPE(name)) {
        name = rb_sym2str(name);
    }
    Check_Type(name, T_STRING);
    UNWRAP_LEXER(self, o);
    type = lexer_get_option(o->lexer, StringValueCStr(name), &optvalptr);

    return rb_get_option(type, optvalptr);
}

/*
 * call-seq:
 *   lexer.set_option(string, value) -> nil
 *
 * Sets an option
 *
 *   lexer.set_option('start_inline', true) # => nil
 */
static VALUE rb_lexer_set_option(VALUE self, VALUE name, VALUE value)
{
    rb_set_option(name, value, self);

    return Qnil;
}

/* ========== Shall module functions ========== */

static VALUE _rb_create_lexer(const LexerImplementation *imp, VALUE options)
{
    int ret;
    VALUE self;
    char buffer[1024];

    self = Qnil;
    ret = snprintf(buffer, STR_SIZE(buffer), "%s::%s", rb_class2name(mLexer), lexer_implementation_name(imp));
    if (ret == -1 || ((size_t) ret) >= STR_SIZE(buffer)) {
        // overflow
    } else {
        VALUE klass;

        if (Qnil != (klass = rb_path2class(buffer))) {
            rb_lexer_object *o;

#ifdef WITH_TYPED_DATA
            self = TypedData_Make_Struct(klass, rb_lexer_object, &lexer_type, o);
#else
            self = Data_Make_Struct(klass, rb_lexer_object, rb_lexer_mark, rb_lexer_free, o);
#endif /* WITH_TYPED_DATA */
            o->lexer = lexer_create(imp);
            rb_set_options(self, options);
        }
    }

    return self;
}

/*
 * call-seq:
 *   Shall::lexer_guess(string, options = {}) -> (nil|Shall::Lexer::Base)
 *
 * Attempts to find out an appropriate lexer by analysing +string+.
 * Returns +nil+ if the operation fails else a lexer instance that
 * may be initialized with extra +options+.
 *
 *   Shall::lexer_guess('<?php echo "Hello world!";') # => Shall::Lexer::PHP
 *   Shall::lexer_guess('print "Hello world!";')      # => nil
 */
static VALUE rb_lexer_guess(int argc, VALUE *argv, VALUE module)
{
    VALUE self, src, options;
    const LexerImplementation *imp;

    self = options = Qnil;
    rb_scan_args(argc, argv, "11", &src, &options);
    Check_Type(src, T_STRING);
    if (NULL != (imp = lexer_implementation_guess(RSTRING_PTR(src), RSTRING_LEN(src)))) {
        self = _rb_create_lexer(imp, options);
    }
    /*if (Qnil == self) {
        rb_raise(rb_eNameError, "%s is not a known lexer", name);
    }*/

    return self;
}

/*
 * call-seq:
 *   Shall::lexer_by_name(string, options = {}) -> (nil|Shall::Lexer::Base)
 *
 * Attempts to find out a lexer through its name or one of its aliases.
 * Returns +nil+ if there is no match else a lexer instance that
 * may be initialized with extra +options+.
 *
 *   Shall::lexer_by_name('php5') # => Shall::Lexer::PHP
 *   Shall::lexer_by_name('foo')  # => nil
 */
static VALUE rb_lexer_by_name(int argc, VALUE *argv, VALUE module)
{
    VALUE self, name, options;
    const LexerImplementation *imp;

    self = options = Qnil;
    rb_scan_args(argc, argv, "11", &name, &options);
    Check_Type(name, T_STRING);
    if (NULL != (imp = lexer_implementation_by_name(StringValueCStr(name)))) {
        self = _rb_create_lexer(imp, options);
    }
    /*if (Qnil == self) {
        rb_raise(rb_eNameError, "%s is not a known lexer", name);
    }*/

    return self;
}

/*
 * call-seq:
 *   Shall::lexer_for_filename(string, options = {}) -> (nil|Shall::Lexer::Base)
 *
 * Attempts to find out an appropriate lexer from a filename.
 * Returns +nil+ if none matches.
 *
 *   Shall::lexer_for_filename('httpd.conf') # => Shall::Lexer::Apache
 *   Shall::lexer_for_filename('bar.foo')    # => nil
 */
static VALUE rb_lexer_for_filename(int argc, VALUE *argv, VALUE module)
{
    VALUE self, filename, options;
    const LexerImplementation *imp;

    self = options = Qnil;
    rb_scan_args(argc, argv, "11", &filename, &options);
    Check_Type(filename, T_STRING);
    if (NULL != (imp = lexer_implementation_for_filename(StringValueCStr(filename)))) {
        self = _rb_create_lexer(imp, options);
    }
    /*if (Qnil == self) {
        rb_raise(rb_eNameError, "%s is not a known lexer", name);
    }*/

    return self;
}

/* ========== Initialisation ========== */

void rb_shall_init_lexer(void)
{
    mLexer = rb_define_module_under(mShall, "Lexer");

    // Shall module functions
    rb_define_module_function(mShall, "lexer_guess", rb_lexer_guess, -1);
    rb_define_module_function(mShall, "lexer_by_name", rb_lexer_by_name, -1);
    rb_define_module_function(mShall, "lexer_for_filename", rb_lexer_for_filename, -1);

    // Shall::Lexer::Base, lexer superclass
    cBaseLexer = rb_define_class_under(mLexer, "Base", rb_cObject);
    // class methods
    rb_define_singleton_method(cBaseLexer, "name", rb_lexer_name, 0);
    rb_define_singleton_method(cBaseLexer, "aliases", rb_lexer_aliases, 0);
    rb_define_singleton_method(cBaseLexer, "mimetypes", rb_lexer_mimetypes, 0);
    // instance methods
    rb_define_method(cBaseLexer, "get_option", rb_lexer_get_option, 1);
    rb_define_method(cBaseLexer, "set_option", rb_lexer_set_option, 2);
//     rb_undef_method(CLASS_OF(cBaseLexer), "new");
    rb_undef_alloc_func(cBaseLexer);

    // expose available lexers as frozen Shall::LEXERS constant, for user and us (we use it internally to avoid crash on inherited Lexer class)
    lexers = rb_ary_new();
    lexer_implementation_each(create_lexer_class_cb, (void *) lexers);
    lexers = rb_ary_freeze(lexers);
    rb_define_const(mShall, "LEXERS", lexers);
}
