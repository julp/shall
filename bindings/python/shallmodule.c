#include <Python.h>

#include <shall/cpp.h>
#include "../../formatter.h" // TODO: temporary
#include <shall/shall.h>
#include <shall/tokens.h>
#include <shall/option.h>

#if 0
PyObject* PyImport_AddModuleObject(PyObject *name);
PyObject* PyImport_AddModule(const char *name);

https://docs.python.org/3/c-api/arg.html
https://docs.python.org/2/c-api/dict.html
https://docs.python.org/2/c-api/string.html
#endif

#define DEBUG

#ifdef DEBUG
# include <stdio.h>
# include <stdarg.h>
# define debug(format, ...) \
    fprintf(stderr, format "\n", ## __VA_ARGS__)
#else
# define debug(format, ...)
#endif

static PyObject *lexers, *formatters;

typedef struct {
    PyObject_HEAD
//     Lexer *lexer;
} ShallLexerBaseObject;

typedef struct {
    ShallLexerBaseObject base;
    Lexer *lexer;
} ShallLexerObject;

typedef struct {
    PyObject_HEAD
    Formatter *fmt;
} ShallFormatterObject;

static PyObject *Shall_Lexer_new(PyTypeObject *, PyObject *, PyObject *);
static void shall_lexer_base_dealloc(ShallLexerBaseObject *);
static int shall_lexer_base_init(ShallLexerBaseObject *, PyObject *, PyObject *);
static PyObject *shall_lexer_get_option(PyObject *, PyObject *);
static PyObject *shall_lexer_set_option(PyObject *, PyObject *);
static PyObject *shall_lexer_get_name(PyObject *);
static PyObject *shall_lexer_get_aliases(PyObject *);
static PyObject *shall_lexer_get_mimetypes(PyObject *);

static PyMethodDef Shall_Lexer_Methods[] = {
    { "get_option",    (PyCFunction) shall_lexer_get_option,    METH_VARARGS, "TODO" },
    { "set_option",    (PyCFunction) shall_lexer_set_option,    METH_VARARGS, "TODO" },
    { "get_name",      (PyCFunction) shall_lexer_get_name,      METH_CLASS | METH_NOARGS, "TODO" },
    { "get_aliases",   (PyCFunction) shall_lexer_get_aliases,   METH_CLASS | METH_NOARGS, "TODO" },
    { "get_mimetypes", (PyCFunction) shall_lexer_get_mimetypes, METH_CLASS | METH_NOARGS, "TODO" },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject ShallLexerBaseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "shall.BaseLexer",              /* tp_name */
    sizeof(ShallLexerBaseObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor) shall_lexer_base_dealloc, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_reserved */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash  */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Shall Lexer Base objects", /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    Shall_Lexer_Methods,        /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc) shall_lexer_base_init, /* tp_init */
    0,                          /* tp_alloc */
    Shall_Lexer_new,            /* tp_new */
};

/* ========== helpers ========== */

static void list_append_string_cb(const char *string, void *data)
{
    PyList_Append((PyObject *) data, PyUnicode_FromString(string));
}

static void python_get_option(int type, OptionValue *optvalptr, PyObject **ret)
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

static inline Lexer *python_lexer_unwrap(void *object)
{
    ShallLexerObject *o;

    o = (ShallLexerObject *) object;

    return o->lexer;
}

static int formatter_set_option_compat_cb(void *object, const char *name, OptionType type, OptionValue optval, void **UNUSED(ptr))
{
    return formatter_set_option((Formatter *) object, name, type, optval);
}

static int python_set_option(void *object, const char *name, PyObject *value, int reject_lexer, int (*cb)(void *, const char *, OptionType, OptionValue, void **))
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

typedef int (*set_option_t)(void *, const char *, OptionType, OptionValue, void **);

static void python_set_options(void *lexer_or_formatter, PyObject *options, int reject_lexer, set_option_t cb)
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

/* ========== lexers ========== */

/* helpers */

static void _create_lexer(const LexerImplementation *imp, PyObject *options, PyObject **ret)
{
    PyObject *klass;
    ShallLexerObject *l;

//     l = (ShallLexerObject *) Py_TYPE(klass)->tp_alloc(Py_TYPE(klass), 0);
    klass = PyDict_GetItemString(lexers, lexer_implementation_name(imp));
//     l = PyObject_New(ShallLexerObject, &ShallLexerBaseType);
    l = PyObject_New(ShallLexerObject, (PyTypeObject *) klass);
    l->lexer = lexer_create(imp);
    if (NULL != options) {
        python_set_options((void *) l->lexer, options, 0, (set_option_t) lexer_set_option);
    }
    *ret = (PyObject *) l;
}

/* class methods */

static PyObject *shall_lexer_get_name(PyObject *cls)
{
    PyObject *ret;
    const LexerImplementation *imp;

    imp = lexer_implementation_by_name(((PyTypeObject *) cls)->tp_name);
    ret = PyUnicode_FromString(lexer_implementation_name(imp));
    Py_INCREF(ret);

    return ret;
}

static PyObject *shall_lexer_get_aliases(PyObject *cls)
{
    PyObject *ret;
    const LexerImplementation *imp;

    imp = lexer_implementation_by_name(((PyTypeObject *) cls)->tp_name);
    ret = PyList_New(0);
    lexer_implementation_each_alias(imp, list_append_string_cb, (void *) ret);
    Py_INCREF(ret);

    return ret;
}

static PyObject *shall_lexer_get_mimetypes(PyObject *cls)
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

static PyObject *shall_lexer_get_option(PyObject *self, PyObject *args)
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

static PyObject *shall_lexer_set_option(PyObject *self, PyObject *args)
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

/* python internals */

/* ... base lexer class */

static PyObject *Shall_Lexer_new(PyTypeObject *type, PyObject *args, PyObject *UNUSED(kwds))
{
//     PyObject *options;
    ShallLexerObject *self;

//     options = NULL;
    self = (ShallLexerObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        //
    }
//     PyArg_ParseTuple(args, "|O!", &PyDict_Type, &options);
//     if (NULL != options) {
//         python_set_options((void *) self->lexer, options, 0, (set_option_t) lexer_set_option);
//     }

    return (PyObject *) self;
}

static void shall_lexer_base_dealloc(ShallLexerBaseObject *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int shall_lexer_base_init(ShallLexerBaseObject *self, PyObject *args, PyObject *UNUSED(kwds))
{
    return 0;
}

/* ... lexer class */

static void Shall_Lexer_dealloc(ShallLexerObject *self)
{
    lexer_destroy(self->lexer, (on_lexer_destroy_cb_t) Py_DecRef);
#if PY_MAJOR_VERSION >= 3
    Py_TYPE((PyObject *) &self->base)
#else
    self->base.ob_type
#endif /* PY_MAJOR_VERSION >= 3 */
    ->tp_free((PyObject *) self);
}

static int Shall_Lexer_init(ShallLexerObject *lo, PyObject *args, PyObject *kwds)
{
    PyObject *self, *options;
    const LexerImplementation *imp;

    options = NULL;
    self = (PyObject *) lo;
    if (ShallLexerBaseType.tp_init(self, args, kwds) < 0) {
        return -1;
    }
    PyArg_ParseTuple(args, "|O!", &PyDict_Type, &options);
    imp = lexer_implementation_by_name(Py_TYPE(self)->tp_name);
    lo->lexer = lexer_create(imp);
    if (NULL != options) {
        python_set_options((void *) lo->lexer, options, 0, (set_option_t) lexer_set_option);
    }

    return 0;
}

static PyTypeObject ShallLexerXType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "shall.lexer",               /* tp_name */
    sizeof(ShallLexerObject),    /* tp_basicsize */
    0,                           /* tp_itemsize */
    (destructor) Shall_Lexer_dealloc, /* tp_dealloc */
    0,                           /* tp_print */
    0,                           /* tp_getattr */
    0,                           /* tp_setattr */
    0,                           /* tp_reserved */
    0,                           /* tp_repr */
    0,                           /* tp_as_number */
    0,                           /* tp_as_sequence */
    0,                           /* tp_as_mapping */
    0,                           /* tp_hash  */
    0,                           /* tp_call */
    0,                           /* tp_str */
    0,                           /* tp_getattro */
    0,                           /* tp_setattro */
    0,                           /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Shall Lexer objects",       /* tp_doc */
    0,                           /* tp_traverse */
    0,                           /* tp_clear */
    0,                           /* tp_richcompare */
    0,                           /* tp_weaklistoffset */
    0,                           /* tp_iter */
    0,                           /* tp_iternext */
    0,                           /* tp_methods */
    0,                           /* tp_members */
    0,                           /* tp_getset */
    &ShallLexerBaseType,         /* tp_base */
    0,                           /* tp_dict */
    0,                           /* tp_descr_get */
    0,                           /* tp_descr_set */
    0,                           /* tp_dictoffset */
    (initproc) Shall_Lexer_init, /* tp_init */
    0,                           /* tp_alloc */
    0,                           /* tp_new */
};

/* ========== formatters ========== */

/* helpers */

/* class methods */

/* instance methods */

static PyObject *shall_formatter_get_option(PyObject *self, PyObject *args)
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

static PyObject *shall_formatter_set_option(PyObject *self, PyObject *args)
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

static PyObject *shall_formatter_start_document(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *shall_formatter_end_document(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *shall_formatter_start_token(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *shall_formatter_end_token(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *shall_formatter_write_token(PyObject *self, PyObject *args)
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

static PyObject *shall_formatter_start_lexing(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *shall_formatter_end_lexing(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

/* python internals */

typedef struct {
    PyObject *self, *options;
} PythonFormatterData;

#define PYTHON_CALLBACK(/*const char **/ method, /*const char **/ argfmt, ...) \
    do { \
        PyObject *res; \
        PythonFormatterData *mydata; \
 \
        mydata = (PythonFormatterData *) data; \
        res = PyObject_CallMethod(mydata->self, method, argfmt, ## __VA_ARGS__); \
        if (Py_None != res) { \
            /*if (PyUnicode_Check(res)) {*/ \
            if (PyBytes_Check(res)) { \
                /*PyObject *utf8_as_bytes;*/ \
 \
                /*if (NULL != (utf8_as_bytes = PyUnicode_AsUTF8String(res))) {*/ \
                    /*string_append_string_len(out, PyBytes_AS_STRING(utf8_as_bytes), PyBytes_GET_SIZE(utf8_as_bytes));*/ \
                /*}*/ \
                string_append_string_len(out, PyBytes_AS_STRING(res), PyBytes_GET_SIZE(res)); \
            } \
        } \
    } while (0);

static int python_start_document(String *out, FormatterData *data)
{
    PYTHON_CALLBACK("start_document", "");

    return 0;
}

static int python_end_document(String *out, FormatterData *data)
{
    PYTHON_CALLBACK("end_document", "");

    return 0;
}

static int python_start_token(int token, String *out, FormatterData *data)
{
    PYTHON_CALLBACK("start_token", "i", token);

    return 0;
}

static int python_end_token(int token, String *out, FormatterData *data)
{
    PYTHON_CALLBACK("end_token", "i", token);

    return 0;
}

static int python_write_token(String *out, const char *token, size_t token_len, FormatterData *data)
{
#if 0
#if PY_MAJOR_VERSION >= 3
    PYTHON_CALLBACK("write_token", "U#", token, token_len);
#else
    PYTHON_CALLBACK("write_token", "N", PyUnicode_FromStringAndSize(token, token_len));
#endif /* PY_MAJOR_VERSION >= 3 */
#else
    PYTHON_CALLBACK("write_token", "s#", token, token_len);
#endif

    return 0;
}

static int python_start_lexing(const char *lexname, String *out, FormatterData *data)
{
    PYTHON_CALLBACK("start_lexing", "s", lexname);

    return 0;
}

static int python_end_lexing(const char *lexname, String *out, FormatterData *data)
{
    PYTHON_CALLBACK("end_lexing", "s", lexname);

    return 0;
}

static OptionValue *python_get_option_ptr(Formatter *fmt, int define, size_t UNUSED(offset), const char *name, size_t name_len)
{
    OptionValue *optvalptr;
    PythonFormatterData *mydata;

    optvalptr = NULL;
    mydata = (PythonFormatterData *) &fmt->optvals;
    if (define) {
        optvalptr = malloc(sizeof(*optvalptr));
//         st_add_direct(mydata->options, (st_data_t) name, (st_data_t) optvalptr);
    } else {
//         st_lookup(mydata->options, (st_data_t) name, (st_data_t *) &optvalptr);
    }

    return optvalptr;
}

static const FormatterImplementation pythonfmt = {
    "Pyhton", // unused
    "", // unused
    python_get_option_ptr,
    python_start_document,
    python_end_document,
    python_start_token,
    python_end_token,
    python_write_token,
    python_start_lexing,
    python_end_lexing,
    sizeof(PythonFormatterData),
    NULL
};

/* ... base formatter class */

static PyObject *Shall_Formatter_new(PyTypeObject *type, PyObject *args, PyObject *UNUSED(kwds))
{
    ShallFormatterObject *self;

    self = (ShallFormatterObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        const char *imp_name;
        const FormatterImplementation *imp;

        imp_name = type->tp_name;
        if (NULL == PyDict_GetItemString(formatters, imp_name)) {
            imp = &pythonfmt;
        } else {
            imp = formatter_implementation_by_name(imp_name);
        }
        self->fmt = formatter_create(imp);
        if (&pythonfmt == imp) {
            PythonFormatterData *mydata;

            mydata = (PythonFormatterData *) &self->fmt->optvals;
            mydata->self = (PyObject *) self;
            mydata->options = PyDict_New();
        }
    }

    return (PyObject *) self;
}

static void Shall_Formatter_dealloc(ShallFormatterObject *self)
{
    formatter_destroy(self->fmt);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int Shall_Formatter_init(ShallFormatterObject *self, PyObject *args, PyObject *UNUSED(kwds))
{
    PyObject *options;
//     const char *imp_name;
//     const FormatterImplementation *imp;

    options = NULL;
    PyArg_ParseTuple(args, "|O!", &PyDict_Type, &options);
    if (NULL != options) {
        python_set_options(self->fmt, options, 1, (set_option_t) formatter_set_option);
    }

    return 0;
}

static PyMethodDef Shall_Formatter_Methods[] = {
    { "get_option",     (PyCFunction) shall_formatter_get_option,     METH_VARARGS, "TODO" },
    { "set_option",     (PyCFunction) shall_formatter_set_option,     METH_VARARGS, "TODO" },
    { "start_document", (PyCFunction) shall_formatter_start_document, METH_VARARGS, "TODO" },
    { "end_document",   (PyCFunction) shall_formatter_end_document,   METH_VARARGS, "TODO" },
    { "start_token",    (PyCFunction) shall_formatter_start_token,    METH_VARARGS, "TODO" },
    { "end_token",      (PyCFunction) shall_formatter_end_token,      METH_VARARGS, "TODO" },
    { "write_token",    (PyCFunction) shall_formatter_write_token,    METH_VARARGS, "TODO" },
    { "start_lexing",   (PyCFunction) shall_formatter_start_lexing,   METH_VARARGS, "TODO" },
    { "end_lexing",     (PyCFunction) shall_formatter_end_lexing,     METH_VARARGS, "TODO" },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject ShallFormatterBaseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "shall.formatter.Base",              /* tp_name */
    sizeof(ShallFormatterObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor) Shall_Formatter_dealloc, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_reserved */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash  */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Shall Formatter Base objects", /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    Shall_Formatter_Methods,        /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc) Shall_Formatter_init, /* tp_init */
    0,                          /* tp_alloc */
    Shall_Formatter_new,            /* tp_new */
};

/* ... formatter class */

static PyTypeObject ShallFormatterXType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "shall.formatter",               /* tp_name */
    sizeof(ShallFormatterObject),    /* tp_basicsize */
    0,                           /* tp_itemsize */
    0,                           /* tp_dealloc */
    0,                           /* tp_print */
    0,                           /* tp_getattr */
    0,                           /* tp_setattr */
    0,                           /* tp_reserved */
    0,                           /* tp_repr */
    0,                           /* tp_as_number */
    0,                           /* tp_as_sequence */
    0,                           /* tp_as_mapping */
    0,                           /* tp_hash  */
    0,                           /* tp_call */
    0,                           /* tp_str */
    0,                           /* tp_getattro */
    0,                           /* tp_setattro */
    0,                           /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Shall Formatter objects",       /* tp_doc */
    0,                           /* tp_traverse */
    0,                           /* tp_clear */
    0,                           /* tp_richcompare */
    0,                           /* tp_weaklistoffset */
    0,                           /* tp_iter */
    0,                           /* tp_iternext */
    0,                           /* tp_methods */
    0,                           /* tp_members */
    0,                           /* tp_getset */
    &ShallFormatterBaseType,         /* tp_base */
    0,                           /* tp_dict */
    0,                           /* tp_descr_get */
    0,                           /* tp_descr_set */
    0,                           /* tp_dictoffset */
    0,                           /* tp_init */
    0,                           /* tp_alloc */
    0,                           /* tp_new */
};

/* ========== module functions ========== */

static PyObject *shall_lexer_guess(PyObject *UNUSED(self), PyObject *args)
{
    const char *src;
    Py_ssize_t src_len;
    PyObject *ret, *options;

    ret = Py_None;
    options = NULL;
    if (PyArg_ParseTuple(args, "s#|O!", &src, &src_len, &PyDict_Type, &options)) {
        const LexerImplementation *imp;

        if (NULL != (imp = lexer_implementation_guess(src, src_len))) {
            _create_lexer(imp, options, &ret);
        }
    }
    Py_INCREF(ret);

    return ret;
}

static PyObject *shall_lexer_by_name(PyObject *UNUSED(self), PyObject *args)
{
    const char *name;
    PyObject *ret, *options;

    ret = Py_None;
    options = NULL;
    if (PyArg_ParseTuple(args, "s|O!", &name, &PyDict_Type, &options)) {
        const LexerImplementation *imp;

        if (NULL != (imp = lexer_implementation_by_name(name))) {
            _create_lexer(imp, options, &ret);
        }
    }
    Py_INCREF(ret);

    return ret;
}

static PyObject *shall_lexer_for_filename(PyObject *UNUSED(self), PyObject *args)
{
    const char *filename;
    PyObject *ret, *options;

    ret = Py_None;
    options = NULL;
    if (PyArg_ParseTuple(args, "s|O!", &filename, &PyDict_Type, &options)) {
        const LexerImplementation *imp;

        if (NULL != (imp = lexer_implementation_for_filename(filename))) {
            _create_lexer(imp, options, &ret);
        }
    }
    Py_INCREF(ret);

    return ret;
}

static PyObject *shall_highlight(PyObject *self, PyObject *args)
{
    const char *string;
    Py_ssize_t string_len;
    PyObject *ret, *lexer, *fmt;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "s#O!O!", &string, &string_len, &ShallLexerBaseType, &lexer, &ShallFormatterBaseType, &fmt)/* && 1 == PyObject_IsInstance(lexer, (PyObject *) &ShallLexerBaseType)*/) {
        char *dest;
        size_t dest_len;

// print issubclass(shall.PHP, shall.Base)
// printf("PyObject_IsInstance = %d\n", PyObject_IsInstance(lexer, (PyObject *) &ShallLexerBaseType));
        highlight_string(string, string_len, &dest, &dest_len, ((ShallFormatterObject *) fmt)->fmt, 1, &((ShallLexerObject *) lexer)->lexer);
        ret = PyUnicode_FromStringAndSize(dest, dest_len);
    }
    Py_INCREF(ret);

    return ret;
}

static PyMethodDef ShallMethods[] = {
    { "highlight",          (PyCFunction) shall_highlight,          METH_VARARGS, "TODO" },
    { "lexer_guess",        (PyCFunction) shall_lexer_guess,        METH_VARARGS, "TODO" },
    { "lexer_by_name",      (PyCFunction) shall_lexer_by_name,      METH_VARARGS, "TODO" },
    { "lexer_for_filename", (PyCFunction) shall_lexer_for_filename, METH_VARARGS, "TODO" },
    { NULL, NULL, 0, NULL }
};

/* ========== module initialization ========== */

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef shallmodule = {
    PyModuleDef_HEAD_INIT,
    "shall",
    "TODO",
    -1,
    ShallMethods,
    NULL, NULL, NULL, NULL
};

// static struct PyModuleDef shalltokenmodule = {
//     PyModuleDef_HEAD_INIT,
//     "shall.token",
//     NULL,
//     -1,
//     NULL
// };
#endif /* PY_MAJOR_VERSION >= 3 */

static PyObject *shallmodulep;

static void create_lexer_class_cb(const LexerImplementation *imp, void *UNUSED(data))
{
    PyTypeObject *type;
//     char *tp_name;
    const char *imp_name;

    imp_name = lexer_implementation_name(imp);
//     asprintf(&tp_name, "shall.%s", imp_name);
    type = PyMem_Malloc(sizeof(*type));
    memcpy(type, &ShallLexerXType, sizeof(*type));
    type->tp_name = /*tp_name*/imp_name;
//     type->tp_flags = Py_TPFLAGS_DEFAULT;
    type->tp_base = &ShallLexerBaseType;
//     type->tp_init = (initproc) Shall_Lexer_init;
    PyType_Ready(type);
    Py_INCREF(type);
    PyModule_AddObject(shallmodulep, imp_name, (PyObject *) type);
//     PyDict_SetItem(lexers, PyUnicode_FromString(imp_name), (PyObject *) type);
    PyDict_SetItemString(lexers, imp_name, (PyObject *) type);
#if 1
    {
        PyObject *dict;

        dict = PyDict_New();
        PyDict_SetItemString(dict, "name", PyUnicode_FromString(imp_name));
//         PyObject_SetAttrString((PyObject *) type, "name", PyUnicode_FromString(imp_name));
        {
            PyObject *list;

            list = PyList_New(0);
            lexer_implementation_each_mimetype(imp, list_append_string_cb, (void *) list);
//             Py_INCREF(list);
            PyDict_SetItemString(dict, "mimetypes", list);
//             PyObject_SetAttrString((PyObject *) type, "mimetypes", list);
        }
        {
            PyObject *list;

            list = PyList_New(0);
            lexer_implementation_each_alias(imp, list_append_string_cb, (void *) list);
//             Py_INCREF(list);
            PyDict_SetItemString(dict, "aliases", list);
//             PyObject_SetAttrString((PyObject *) type, "aliases", list);
        }
        Py_INCREF(dict);
        type->tp_dict = dict;
    }
#endif
}

static void create_formatter_class_cb(const FormatterImplementation *imp, void *UNUSED(data))
{
    PyTypeObject *type;
    const char *imp_name;

    imp_name = formatter_implementation_name(imp);
    type = PyMem_Malloc(sizeof(*type));
    memcpy(type, &ShallFormatterXType, sizeof(*type));
    type->tp_name = imp_name;
    PyType_Ready(type);
    Py_INCREF(type);
    PyModule_AddObject(shallmodulep, imp_name, (PyObject *) type);
    PyDict_SetItemString(formatters, imp_name, (PyObject *) type);
}

#if PY_MAJOR_VERSION < 3
void init_shall_token(void)
{
    PyObject *shalltokenmodulep = Py_InitModule("shall.token", NULL);
    PyModule_AddObject(shallmodulep, "token", shalltokenmodulep);

    debug("init_shall_token called\n");
#define TOKEN(constant, parent, description, cssclass) \
    PyModule_AddIntConstant(shalltokenmodulep, #constant, constant);
//     PyDict_SetItemString(tokens, #constant, PyLong_FromLong(constant));
#include <shall/keywords.h>
#undef TOKEN
}
#endif /* PY_MAJOR_VERSION < 3 */

/*
Submodules?
http://computer-programming-forum.com/56-python/2336abf191d170aa.htm
http://mdqinc.com/blog/2011/08/statically-linking-python-with-cython-generated-modules-and-packages/
*/

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_shall
#else
initshall
#endif /* PY_MAJOR_VERSION >= 3 */
(void)
{
    PyObject /**shallmodulep, */*shalltokenmodulep, *tokens;

    if (PyType_Ready(&ShallLexerBaseType) < 0) {
        return
#if PY_MAJOR_VERSION >= 3
            NULL;
#endif /* PY_MAJOR_VERSION >= 3 */
        ;
    }
#if PY_MAJOR_VERSION >= 3
    shallmodulep = PyModule_Create(&shallmodule);
//     shalltokenmodulep = PyModule_Create(&shalltokenmodule);
#else
//     ShallLexerBaseType.tp_new = PyType_GenericNew;
    shallmodulep = Py_InitModule("shall", ShallMethods);
# if 0
    shalltokenmodulep = Py_InitModule("shall.token", NULL);
# else
//     inittoken();
//     shalltokenmodulep = PyImport_AddModule("token");
//     PyModule_AddObject(shallmodulep, "Token", shalltokenmodulep);
//     Py_INCREF(shalltokenmodulep);
    PyDict_SetItemString(PyModule_GetDict(shallmodulep), "__path__", PyUnicode_FromString("shall"));
    PyImport_AppendInittab("shall.token", init_shall_token);
# endif
#endif /* PY_MAJOR_VERSION >= 3 */

    Py_INCREF(&ShallLexerBaseType);
    PyModule_AddObject(shallmodulep, "BaseLexer", (PyObject *) &ShallLexerBaseType);

    lexers = PyDict_New();
    Py_INCREF(lexers);

    lexer_implementation_each(create_lexer_class_cb, NULL);

    formatters = PyDict_New();
    Py_INCREF(formatters);

    Py_INCREF(&ShallFormatterBaseType);
    PyModule_AddObject(shallmodulep, "BaseFormatter", (PyObject *) &ShallFormatterBaseType);

    formatter_implementation_each(create_formatter_class_cb, NULL);

#if 0
    tokens = PyDict_New();
    Py_INCREF(tokens);
    PyModule_AddObject(shallmodulep, "Token", tokens);
#define TOKEN(constant, parent, description, cssclass) \
    PyModule_AddIntConstant(shalltokenmodulep, #constant, constant);
//     PyDict_SetItemString(tokens, #constant, PyLong_FromLong(constant));
#include <shall/keywords.h>
#undef TOKEN
#endif

#if PY_MAJOR_VERSION >= 3
    return shallmodulep;
#endif /* PY_MAJOR_VERSION >= 3 */
}
