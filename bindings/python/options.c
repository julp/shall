#include "common.h"
#include "options.h"
#include "lexer_class.h"

void python_get_option(int type, OptionValue *optvalptr, PyObject **ret)
{
    switch (type) {
        case OPT_TYPE_BOOL:
            *ret = OPT_GET_BOOL(*optvalptr) ? Py_True : Py_False;
            break;
        case OPT_TYPE_INT:
            *ret = PyLong_FromLong(OPT_GET_INT(*optvalptr));
            break;
        case OPT_TYPE_STRING:
            *ret = PyUnicode_FromStringAndSize(OPT_STRVAL(*optvalptr), OPT_STRLEN(*optvalptr));
            break;
        case OPT_TYPE_LEXER:
            if (NULL != OPT_LEXPTR(*optvalptr)) {
                *ret = (PyObject *) OPT_LEXPTR(*optvalptr);
//                 Py_IncRef(*ret);
            }
            break;
    }
}

int formatter_set_option_compat_cb(void *object, const char *name, OptionType type, OptionValue optval, void **UNUSED(ptr))
{
    return formatter_set_option((Formatter *) object, name, type, optval);
}

int python_set_option(void *object, const char *name, PyObject *value, int reject_lexer, int (*cb)(void *, const char *, OptionType, OptionValue, void **))
{
    int type;
    PyObject *ptr;
    OptionValue optval;

    ptr = NULL;
    /*if (Py_None == value) {
        // TODO: NULL to "remove" sublexers
    } else*/ if (PyBool_Check(value)) {
        type = OPT_TYPE_BOOL;
        OPT_SET_BOOL(optval, ((Py_True) == value));
    } else if (PyLong_Check(value)) {
        type = OPT_TYPE_INT;
        OPT_SET_INT(optval, PyLong_AsLong(value));
    } else if (PyUnicode_Check(value)) {
        type = OPT_TYPE_STRING;
        OPT_STRVAL(optval) = PyUnicode_AS_DATA(value);
        OPT_STRLEN(optval) = PyUnicode_GET_SIZE(value);
    } else if (PyObject_IsInstance(value, (PyObject *) &ShallLexerBaseType)) {
        type = OPT_TYPE_LEXER;
        Py_IncRef(value);
        OPT_LEXPTR(optval) = value;
        OPT_LEXUWF(optval) = python_lexer_unwrap;
    } else {
        return 0;
    }
    cb(object, name, type, optval, (void **) &ptr);
    if (NULL != ptr) {
        Py_DecRef(ptr);
    }

    return 1;
}

void python_set_options(void *lexer_or_formatter, PyObject *options, int reject_lexer, set_option_t cb)
{
    Py_ssize_t pos = 0;
    PyObject *key, *value;

    while (PyDict_Next(options, &pos, &key, &value)) {
#if 0
        PyObject *utf8_as_bytes;

        if (NULL != (utf8_as_bytes = PyUnicode_AsUTF8String(key))) {
            python_set_option(lexer_or_formatter, PyBytes_AS_STRING(utf8_as_bytes), value, reject_lexer, cb);
        }
#else
        python_set_option(lexer_or_formatter, PyBytes_AS_STRING(key), value, reject_lexer, cb);
#endif
    }
}
