#include "common.h"
#include "options.h"
#include "formatter_class.h"

/* instance methods */

PyObject *shall_formatter_get_option(PyObject *self, PyObject *args)
{
    PyObject *ret;
    const char *name;
    OptionType type;
    OptionValue *optvalptr;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "s", &name)) {
        type = formatter_get_option(((ShallFormatterObject *) self)->fmt, name, &optvalptr);
        python_get_option(type, optvalptr, &ret);
    }
    Py_INCREF(ret);

    return ret;
}

PyObject *shall_formatter_set_option(PyObject *self, PyObject *args)
{
    const char *name;
    PyObject *ret, *value;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "sO", &name, &value)) {
        ret = python_set_option(((ShallFormatterObject *) self)->fmt, name, value, 1, formatter_set_option_compat_cb) ? Py_True : Py_False;
    }
    Py_INCREF(ret);

    return ret;
}

PyObject *shall_formatter_start_document(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

PyObject *shall_formatter_end_document(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

PyObject *shall_formatter_start_token(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

PyObject *shall_formatter_end_token(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

PyObject *shall_formatter_write_token(PyObject *self, PyObject *args)
{
#if 1
    PyObject *ret;

    ret = Py_None;
    PyArg_ParseTuple(args, "S", &ret);
    Py_INCREF(ret);

    return ret;
#else
    Py_RETURN_NONE;
#endif
}

PyObject *shall_formatter_start_lexing(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

PyObject *shall_formatter_end_lexing(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}
