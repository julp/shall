#include <ruby.h>

#define DEBUG
#include <shall/cpp.h>
#include <shall/formatter.h>
#include <shall/shall.h>
#include <shall/tokens.h>
#include <shall/types.h>

#define WITH_TYPED_DATA 1

static VALUE lexers, formatters;
static VALUE cBaseLexer, cBaseFormatter;
static VALUE mShall, mLexer, mFormatter, mToken;

static int rb_set_option(VALUE, VALUE, VALUE);

/* ========== Common ========== */

/* helpers */

static void ary_push_string_cb(const char *string, void *data)
{
    rb_ary_push((VALUE) data, rb_str_new_cstr(string));
}

/* ========== Lexer ========== */

/* ruby internal */

typedef struct {
    Lexer *lexer;
} rb_lexer_object;

#ifdef WITH_TYPED_DATA
# define UNWRAP_LEXER(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_lexer_object, &lexer_type, output)
#else
# define UNWRAP_LEXER(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_lexer_object, output)
#endif /* WITH_TYPED_DATA */

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

static const struct rb_data_type_struct lexer_type = {
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

static inline Lexer *ruby_lexer_unwrap(void *object)
{
    rb_lexer_object *o;

    UNWRAP_LEXER((VALUE) object, o);

    return o->lexer;
}

static VALUE rb_get_option(int type, OptionValue *optvalptr)
{
    VALUE ret;

    ret = Qnil;
    switch (type) {
        case OPT_TYPE_BOOL:
            ret = OPT_GET_BOOL(*optvalptr) ? Qtrue : Qfalse;
            break;
        case OPT_TYPE_INT:
            ret = INT2FIX(OPT_GET_INT(*optvalptr));
            break;
        case OPT_TYPE_STRING:
            ret = rb_str_new(OPT_STRVAL(*optvalptr), OPT_STRLEN(*optvalptr));
            break;
        case OPT_TYPE_LEXER:
            if (NULL != OPT_LEXPTR(*optvalptr)) {
                ret = (VALUE) OPT_LEXPTR(*optvalptr);
// #ifdef WITH_TYPED_DATA
//                 ret = TypedData_Make_Struct(lexer_implementation_to_klass(lexer_implementation(lexer)), rb_lexer_object, &lexer_type, l);
// #else
//                 ret = Data_Make_Struct(lexer_implementation_to_klass(lexer_implementation(lexer)), rb_lexer_object, rb_lexer_mark, rb_lexer_free, l);
// #endif /* WITH_TYPED_DATA */
            }
            break;
    }

    return ret;
}

static void rb_set_options(VALUE self, VALUE options)
{
    if (!NIL_P(options)) {
        Check_Type(options, T_HASH);
        rb_hash_foreach(options, rb_set_option, self);
    }
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
    rb_define_const(cXLexer, "NAME", rb_str_new_cstr(imp_name));
    ary = rb_ary_new();
    lexer_implementation_each_alias(imp, ary_push_string_cb, (void *) ary);
    rb_define_const(cXLexer, "ALIASES", ary);
    ary = rb_ary_new();
    lexer_implementation_each_mimetype(imp, ary_push_string_cb, (void *) ary);
    rb_define_const(cXLexer, "MIMETYPES", ary);
#endif
}

/* class methods */

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

    return rb_str_new_cstr(lexer_implementation_name(imp));
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

/* ========== Formatter ========== */

/* ruby internal */

typedef struct {
    Formatter *formatter;
} rb_formatter_object;

#ifdef WITH_TYPED_DATA
# define UNWRAP_FORMATTER(/*VALUE*/ input, /**/ output) \
    TypedData_Get_Struct(input, rb_formatter_object, &formatter_type, output)
#else
# define UNWRAP_FORMATTER(/*VALUE*/ input, /**/ output) \
    Data_Get_Struct(input, rb_formatter_object, output)
#endif /* WITH_TYPED_DATA */

static int free_optionvalue(st_data_t UNUSED(key), st_data_t value, st_data_t UNUSED(data))
{
    free((OptionValue *) value);

    return ST_CONTINUE;
}

#ifdef WITH_TYPED_DATA
static void rb_formatter_free(void *ptr);
#else
static void rb_formatter_free(rb_formatter_object *o);
#endif /* WITH_TYPED_DATA */

#ifdef WITH_TYPED_DATA
static void rb_formatter_mark(void *ptr)
{
    rb_formatter_object *o;

    o = (rb_formatter_object *) ptr;
    (void) o;
#else
static void rb_formatter_mark(rb_formatter_object *o)
{
#endif /* WITH_TYPED_DATA */
    // NOP (for now)
}

#ifdef WITH_TYPED_DATA
static const struct rb_data_type_struct formatter_type = {
    "shall formatter",
    { rb_formatter_mark, rb_formatter_free, 0/*, { 0, 0 }*/ },
    NULL,
    NULL,
    0 | RUBY_TYPED_FREE_IMMEDIATELY
};
#endif /* WITH_TYPED_DATA */

/* callbacks */

typedef struct {
    VALUE self;
    st_table *options;
} RubyFormatterData;

static ID sStartLexing, sEndLexing, sStartDocument, sEndDocument, sStartToken, sEndToken, sWriteToken;

#define RUBY_CALLBACK(/*ID*/ method, /*int*/ argc, ...) \
    do { \
        VALUE res; \
        RubyFormatterData *mydata; \
 \
        mydata = (RubyFormatterData *) data; \
        res = rb_funcall(mydata->self, method, argc, ## __VA_ARGS__); \
        if (!NIL_P(res)) { \
            Check_Type(res, T_STRING); \
            string_append_string_len(out, RSTRING_PTR(res), RSTRING_LEN(res)); \
        } \
    } while (0);

static int ruby_start_document(String *out, FormatterData *data)
{
    RUBY_CALLBACK(sStartDocument, 0);

    return 0;
}

static int ruby_end_document(String *out, FormatterData *data)
{
    RUBY_CALLBACK(sEndDocument, 0);

    return 0;
}

static int ruby_start_token(int token, String *out, FormatterData *data)
{
    RUBY_CALLBACK(sStartToken, 1, INT2FIX(token));

    return 0;
}

static int ruby_end_token(int token, String *out, FormatterData *data)
{
    RUBY_CALLBACK(sEndToken, 1, INT2FIX(token));

    return 0;
}

static int ruby_write_token(String *out, const char *token, size_t token_len, FormatterData *data)
{
    RUBY_CALLBACK(sWriteToken, 1, rb_str_new(token, token_len));

    return 0;
}

static int ruby_start_lexing(const char *lexname, String *out, FormatterData *data)
{
    RUBY_CALLBACK(sStartLexing, 1, rb_str_new_cstr(lexname));

    return 0;
}

static int ruby_end_lexing(const char *lexname, String *out, FormatterData *data)
{
    RUBY_CALLBACK(sEndLexing, 1, rb_str_new_cstr(lexname));

    return 0;
}

static OptionValue *ruby_get_option_ptr(Formatter *fmt, int define, size_t UNUSED(offset), const char *name, size_t name_len)
{
    OptionValue *optvalptr;
    RubyFormatterData *mydata;

    optvalptr = NULL;
    mydata = (RubyFormatterData *) &fmt->optvals;
    if (define) {
        optvalptr = malloc(sizeof(*optvalptr));
        st_add_direct(mydata->options, (st_data_t) name, (st_data_t) optvalptr);
    } else {
        st_lookup(mydata->options, (st_data_t) name, (st_data_t *) &optvalptr);
    }

    return optvalptr;
}

static const FormatterImplementation rubyfmt = {
    "Ruby", // unused
    "", // unused
    ruby_get_option_ptr,
    ruby_start_document,
    ruby_end_document,
    ruby_start_token,
    ruby_end_token,
    ruby_write_token,
    ruby_start_lexing,
    ruby_end_lexing,
    sizeof(RubyFormatterData),
    NULL
};

/* helpers */

static VALUE rb_formatter_alloc(VALUE klass)
{
    VALUE o;
    rb_formatter_object *f;
    const FormatterImplementation *imp;

#ifdef WITH_TYPED_DATA
    o = TypedData_Make_Struct(klass, rb_formatter_object, &formatter_type, f);
#else
    o = Data_Make_Struct(klass, rb_formatter_object, rb_formatter_mark, rb_formatter_free, f);
#endif /* WITH_TYPED_DATA */
    if (Qtrue == rb_ary_includes(formatters, klass)) {
        imp = formatter_implementation_by_name(rb_class2name(klass) + STR_LEN("Shall::Formatter::"));
    } else {
        imp = &rubyfmt;
    }
    f->formatter = formatter_create(imp);
    if (&rubyfmt == imp) {
        RubyFormatterData *mydata;

        mydata = (RubyFormatterData *) &f->formatter->optvals;
        mydata->self = o;
        mydata->options = st_init_strtable();
#if 1
        {
/*
#define CLASS_EQ(c1, c2) \
    (c1 == c2 || RCLASS_M_TBL(c1) == RCLASS_M_TBL(c2))
*/

            VALUE klass_before_base, k;

            for (k = klass_before_base = klass; cBaseFormatter != /*RCLASS_SUPER*/(k); klass_before_base = k, k = RCLASS_SUPER(k))
                ;
debug("klass_before_base = %s", rb_class2name(klass_before_base));
            if (klass_before_base != klass) {
                if (Qtrue == rb_ary_includes(formatters, klass_before_base)) {
                    const FormatterImplementation *imp;

                    imp = formatter_implementation_by_name(rb_class2name(klass_before_base) + STR_LEN("Shall::Formatter::"));
                    debug("[F] %s > %s (hérite d'un builtin formatter: %s)", rb_class2name(klass), rb_class2name(klass_before_base), formatter_implementation_name(imp));
                } else {
                    debug("[F] %s n'hérite pas d'un builtin formatter", rb_class2name(klass));
                }
            } else {
                debug("[F] %s hérite directement de Shall::Formatter::Base", rb_class2name(klass));
            }
        }
#endif
    }

    return o;
}

static VALUE rb_formatter_initialize(int, VALUE *, VALUE);

#if 0
static VALUE w_formatter_start_document(VALUE self)
{
    rb_formatter_object *f;

    UNWRAP_FORMATTER(self, f);

    return rb_str_new_cstr(f->imp->start_document(?, &f->data));
}
#endif

/* :nodoc: */
static void create_formatter_class_cb(const FormatterImplementation *imp, void *data)
{
    VALUE cXFormatter;
    const char *imp_name;

    imp_name = formatter_implementation_name(imp);
    cXFormatter = rb_define_class_under(mFormatter, imp_name, cBaseFormatter);
    rb_define_alloc_func(cXFormatter, rb_formatter_alloc);
    rb_define_method(cXFormatter, "initialize", rb_formatter_initialize, -1);
#if 0
    rb_define_method(cXFormatter, "start_document", w_formatter_start_document, 0);
    rb_define_method(cXFormatter, "end_document", w_formatter_end_document, 0);
    rb_define_method(cXFormatter, "start_token", w_formatter_start_token, 1);
    rb_define_method(cXFormatter, "end_token", w_formatter_end_token, 1);
    rb_define_method(cXFormatter, "write_token", w_formatter_write_token, 1);
#endif
    rb_ary_push((VALUE) data, cXFormatter);
}

/* instance methods */

static VALUE rb_formatter_get_option(VALUE self, VALUE name)
{
    rb_formatter_object *o;
    OptionType type;
    OptionValue *optvalptr;

    if (T_SYMBOL == TYPE(name)) {
        name = rb_sym2str(name);
    }
    Check_Type(name, T_STRING);
    UNWRAP_FORMATTER(self, o);
    type = formatter_get_option(o->formatter, StringValueCStr(name), &optvalptr);

    return rb_get_option(type, optvalptr);
}

static VALUE rb_formatter_set_option(VALUE self, VALUE name, VALUE value)
{
    rb_set_option(name, value, self);

    return Qnil;
}

/*
 * call-seq:
 *   Shall::Formatter::Foo.new(options = {}) -> Shall::Formatter::new
 *
 * Creates a new formatter with optionnal +options+
 *
 *    Shall::Formatter::HTML.new(cssclass: 'highlight')
 */
static VALUE rb_formatter_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE options;

    options = Qnil;
    rb_scan_args(argc, argv, "01", &options);
    rb_set_options(self, options);

    return self;
}

/*
 * call-seq:
 *   formatter.start_document -> (nil|string)
 *
 * Callback invoked before tokenization.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to prepend
 * a random string to the generated output.
 */
static VALUE rb_formatter_start_document(VALUE UNUSED(self))
{
    return Qnil;
}

/*
 * call-seq:
 *   formatter.end_document -> (nil|string)
 *
 * Callback invoked after tokenization.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to append
 * a random string to the generated output.
 */
static VALUE rb_formatter_end_document(VALUE UNUSED(self))
{
    return Qnil;
}

/*
 * call-seq:
 *   formatter.start_token(type) -> (nil|string)
 *
 * Callback invoked before a new type of token is encountered.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to prepend
 * a random string before a token.
 *
 * For example, if the input document were composed of the tokens:
 * "1", "2" of type "digit" and "f" of type "letter". The following
 * calls to formatter callbacks would be fired:
 *
 * - start_document
 * - start_token before '1'
 * - write_token on '1'
 * - write_token on '2'
 * - end_token after '2'
 * - start_token before 'f'
 * - write_token on 'f'
 * - end_token after 'f'
 * - end_document
 *
 * +type+ is one of the Shall::Token::* values/constants
 */
static VALUE rb_formatter_start_token(VALUE UNUSED(self), VALUE UNUSED(token))
{
    return Qnil;
}

/*
 * call-seq:
 *   formatter.end_token(type) -> (nil|string)
 *
 * Callback invoked after a new type of token is encountered.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to append
 * a random string after a token.
 *
 * +type+ is one of the Shall::Token::* values/constants
 */
static VALUE rb_formatter_end_token(VALUE UNUSED(self), VALUE UNUSED(token))
{
    return Qnil;
}

/*
 * call-seq:
 *   formatter.write_token(string) -> (nil|string)
 *
 * Callback invoked when a token is encountered.
 * Default behavior is to append the token as is.
 */
static VALUE rb_formatter_write_token(VALUE UNUSED(self), VALUE token)
{
    return token;
}

/*
 * call-seq:
 *   formatter.start_lexing(string) -> (nil|string)
 *
 * Callback invoked when a sublexer join the party.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to append
 * a random string on lexer switch.
 */
static VALUE rb_formatter_start_lexing(VALUE UNUSED(self), VALUE UNUSED(lexname))
{
    return Qnil;
}

/*
 * call-seq:
 *   formatter.end_lexing(string) -> (nil|string)
 *
 * Callback invoked when a sublexer leave the party.
 * Default behavior is to do nothing (nil returned). Override
 * it in your class to return a string if you want to append
 * a random string when a lexer return the control.
 */
static VALUE rb_formatter_end_lexing(VALUE UNUSED(self), VALUE UNUSED(lexname))
{
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
    dest_len = highlight_string(l->lexer, f->formatter, StringValueCStr(string), &dest);

    return rb_str_new(dest, dest_len);
}

static int rb_set_option(VALUE key, VALUE val, VALUE wl)
{
    int islexer;
    OptionType type;
    OptionValue optval;
    rb_lexer_object *o;
    rb_formatter_object *x;

    // quiet compiler (warning: [xo] may be used uninitialized)
    o = NULL;
    x = NULL;
    islexer = Qtrue == rb_class_inherited_p(CLASS_OF(wl), cBaseLexer);
    if (islexer) {
        UNWRAP_LEXER(wl, o);
    } else {
        UNWRAP_FORMATTER(wl, x);
    }
    switch (TYPE(key)) {
        case T_SYMBOL:
            key = rb_sym2str(key);
            /* no break */
        case T_STRING:
        {
            switch (TYPE(val)) {
                case T_SYMBOL:
                    val = rb_sym2str(val);
                    /* no break */
                case T_STRING:
                    type = OPT_TYPE_STRING;
                    OPT_STRVAL(optval) = StringValueCStr(val);
                    OPT_STRLEN(optval) = strlen(OPT_STRVAL(optval));
                    break;
                case T_FIXNUM:
                    type = OPT_TYPE_INT;
                    OPT_SET_INT(optval, FIX2INT(val));
                    break;
                case T_TRUE:
                case T_FALSE:
                    type = OPT_TYPE_INT;
                    OPT_SET_BOOL(optval, Qtrue == val);
                    break;
                case T_DATA:
                    if (islexer) { // only a lexer can set a sublexer
                        if (Qtrue == rb_class_inherited_p(CLASS_OF(val), cBaseLexer)) {
//                             rb_lexer_object *l;

//                             UNWRAP_LEXER(val, l);
                            type = OPT_TYPE_LEXER;
//                             OPT_LEXPTR(optval) = l;
                            OPT_LEXPTR(optval) = (void *) val;
                            OPT_LEXUWF(optval) = ruby_lexer_unwrap;
                        }
                        break;
                    }
                    /* no break if false */
                default:
                    return ST_CONTINUE;
            }
            break;
        }
        default:
            return ST_CONTINUE;
    }
    if (islexer) {
        lexer_set_option(o->lexer, StringValueCStr(key), type, optval, NULL);
    } else {
        formatter_set_option(x->formatter, StringValueCStr(key), type, optval);
    }

    return ST_CONTINUE;
}

#ifdef WITH_TYPED_DATA
static void rb_formatter_free(void *ptr)
{
    rb_formatter_object *o;

    o = (rb_formatter_object *) ptr;
#else
static void rb_formatter_free(rb_formatter_object *o)
{
#endif /* WITH_TYPED_DATA */
    if (&rubyfmt == o->formatter->imp) {
        RubyFormatterData *mydata;

        mydata = (RubyFormatterData *) &o->formatter->optvals;
        st_foreach(mydata->options, free_optionvalue, 0);
        st_free_table(mydata->options);
    }
    formatter_destroy(o->formatter);
    xfree(o);
}

/* ========== Initialisation ========== */

/*
 * X
 */
void Init_shall(void)
{
    sStartDocument = rb_intern("start_document");
    sEndDocument = rb_intern("end_document");
    sStartToken = rb_intern("start_token");
    sEndToken = rb_intern("end_token");
    sWriteToken = rb_intern("write_token");
    sStartLexing = rb_intern("start_lexing");
    sEndLexing = rb_intern("end_lexing");

    mShall = rb_define_module("Shall");
    mLexer = rb_define_module_under(mShall, "Lexer");
    mToken = rb_define_module_under(mShall, "Token");
    mFormatter = rb_define_module_under(mShall, "Formatter");

#define TOKEN(constant, description, cssclass) \
    rb_define_const(mToken, #constant, INT2FIX(constant));
#include <shall/keywords.h>
#undef TOKEN

    // Shall module functions
    rb_define_module_function(mShall, "lexer_guess", rb_lexer_guess, -1);
    rb_define_module_function(mShall, "lexer_by_name", rb_lexer_by_name, -1);
    rb_define_module_function(mShall, "lexer_for_filename", rb_lexer_for_filename, -1);
    rb_define_module_function(mShall, "highlight", rb_shall_highlight, 3);

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

    // Shall::Formatter::Base, formatter superclass
    cBaseFormatter = rb_define_class_under(mFormatter, "Base", rb_cObject);
    // instance mehods
    rb_define_method(cBaseFormatter, "get_option", rb_formatter_get_option, 1);
    rb_define_method(cBaseFormatter, "set_option", rb_formatter_set_option, 2);
    rb_define_method(cBaseFormatter, "start_document", rb_formatter_start_document, 0);
    rb_define_method(cBaseFormatter, "end_document", rb_formatter_end_document, 0);
    rb_define_method(cBaseFormatter, "start_token", rb_formatter_start_token, 1);
    rb_define_method(cBaseFormatter, "end_token", rb_formatter_end_token, 1);
    rb_define_method(cBaseFormatter, "write_token", rb_formatter_write_token, 1);
    rb_define_method(cBaseFormatter, "start_lexing", rb_formatter_start_lexing, 1);
    rb_define_method(cBaseFormatter, "end_lexing", rb_formatter_end_lexing, 1);
//     rb_undef_method(CLASS_OF(cBaseFormatter), "new");
//     rb_undef_alloc_func(cBaseFormatter);
    rb_define_alloc_func(cBaseFormatter, rb_formatter_alloc);

    // for internal use, regroup builtin formatters in frozen Shall::FORMATTERS constant
    formatters = rb_ary_new();
    formatter_implementation_each(create_formatter_class_cb, (void *) formatters);
    formatters = rb_ary_freeze(formatters);
    rb_define_const(mShall, "FORMATTERS", formatters);
}
