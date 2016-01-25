#include "ruby_shall.h"
#include "ruby_formatter.h"

VALUE cBaseFormatter;
static VALUE formatters;
static VALUE mFormatter;

/* ruby internal */

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
const struct rb_data_type_struct formatter_type = {
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

/* ========== Initialisation ========== */

void rb_shall_init_formatter(void)
{
    sStartDocument = rb_intern("start_document");
    sEndDocument = rb_intern("end_document");
    sStartToken = rb_intern("start_token");
    sEndToken = rb_intern("end_token");
    sWriteToken = rb_intern("write_token");
    sStartLexing = rb_intern("start_lexing");
    sEndLexing = rb_intern("end_lexing");

    mFormatter = rb_define_module_under(mShall, "Formatter");

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
