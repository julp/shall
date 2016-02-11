#include "common.h"
#include "options.h"
#include "helpers.h"
#include "lexer_class.h"

/* class methods */

PyObject *shall_lexer_get_name(PyObject *cls)
{
    PyObject *ret;
    const LexerImplementation *imp;

    imp = lexer_implementation_by_name(((PyTypeObject *) cls)->tp_name);
    ret = PyUnicode_FromString(lexer_implementation_name(imp));
    Py_INCREF(ret);

    return ret;
}

PyObject *shall_lexer_get_aliases(PyObject *cls)
{
    PyObject *ret;
    const LexerImplementation *imp;

    imp = lexer_implementation_by_name(((PyTypeObject *) cls)->tp_name);
    ret = PyList_New(0);
    lexer_implementation_each_alias(imp, list_append_string_cb, (void *) ret);
    Py_INCREF(ret);

    return ret;
}

PyObject *shall_lexer_get_mimetypes(PyObject *cls)
{
    PyObject *ret;
    const LexerImplementation *imp;

    imp = lexer_implementation_by_name(((PyTypeObject *) cls)->tp_name);
    ret = PyList_New(0);
    lexer_implementation_each_mimetype(imp, list_append_string_cb, (void *) ret);
    Py_INCREF(ret);

    return ret;
}

/* instance methods */

PyObject *shall_lexer_get_option(PyObject *self, PyObject *args)
{
    PyObject *ret;
    const char *name;
    OptionType type;
    OptionValue *optvalptr;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "s", &name)) {
        type = lexer_get_option(((ShallLexerObject *) self)->lexer, name, &optvalptr);
        python_get_option(type, optvalptr, &ret);
    }
    Py_INCREF(ret);

    return ret;
}

PyObject *shall_lexer_set_option(PyObject *self, PyObject *args)
{
    const char *name;
    PyObject *ret, *value;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "sO", &name, &value)) {
        ret = python_set_option(((ShallLexerObject *) self)->lexer, name, value, 0, (set_option_t) lexer_set_option) ? Py_True : Py_False;
    }
    Py_INCREF(ret);

    return ret;
}
