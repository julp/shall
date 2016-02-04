#include "ruby_shall.h"
#include "ruby_lexer.h"
#include "ruby_theme.h"
#include "ruby_formatter.h"

void ary_push_string_cb(const char *string, void *data)
{
    rb_ary_push((VALUE) data, rb_usascii_str_new_cstr(string));
}

VALUE rb_get_option(int type, OptionValue *optvalptr)
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
        case OPT_TYPE_THEME:
            // TODO (without doing a copy would be nice)
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

void rb_set_options(VALUE self, VALUE options)
{
    if (!NIL_P(options)) {
        Check_Type(options, T_HASH);
        rb_hash_foreach(options, rb_set_option, self);
    }
}

int rb_set_option(VALUE key, VALUE val, VALUE wl)
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
                case T_CLASS:
                    if (!islexer) { // only a formatter can set a theme
                        if (Qtrue == rb_class_inherited_p(val, cBaseTheme)) {
                            Theme *theme;

                            theme = fetch_theme_instance(val);
                            type = OPT_TYPE_THEME;
                            OPT_THEMPTR(optval) = theme;
                            break;
                        }
                    }
                    /* no break if false */ // TODO: a little bit ugly here
                case T_DATA:
                    if (islexer) { // only a lexer can set a sublexer
                        if (Qtrue == rb_class_inherited_p(CLASS_OF(val), cBaseLexer)) {
//                             rb_lexer_object *l;

//                             UNWRAP_LEXER(val, l);
                            type = OPT_TYPE_LEXER;
//                             OPT_LEXPTR(optval) = l;
                            OPT_LEXPTR(optval) = (void *) val;
                            OPT_LEXUWF(optval) = ruby_lexer_unwrap;
                            break;
                        }
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
