#include "common.h"
#include "options.h"
#include "helpers.h"
#include "formatter_class.h"
#include "formatter_methods.h"

static PyObject *formatters;

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
    NULL,
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

PyTypeObject ShallFormatterBaseType = {
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

static void create_formatter_class_cb(const FormatterImplementation *imp, void *data)
{
    PyTypeObject *type;
    const char *imp_name;

    imp_name = formatter_implementation_name(imp);
    type = PyMem_Malloc(sizeof(*type));
    memcpy(type, &ShallFormatterXType, sizeof(*type));
// #if 1
    type->tp_name = imp_name;
// #else
    char *p;
    size_t imp_name_len;

    imp_name_len = strlen(imp_name);
    p = PyMem_New(char, imp_name_len + STR_SIZE("Formatter"));
    memcpy(p, imp_name, imp_name_len);
    memcpy(p + imp_name_len, "Formatter", STR_SIZE("Formatter"));
//     type->tp_name = p;
// #endif
    PyType_Ready(type);
    Py_INCREF(type);
    PyModule_AddObject((PyObject *) data, p, (PyObject *) type);
    PyDict_SetItemString(formatters, imp_name, (PyObject *) type);
}

void register_formatter_class(PyObject *shallmodulep)
{
    formatters = PyDict_New();
    Py_INCREF(formatters);

    Py_INCREF(&ShallFormatterBaseType);
    PyModule_AddObject(shallmodulep, "BaseFormatter", (PyObject *) &ShallFormatterBaseType);

    formatter_implementation_each(create_formatter_class_cb, shallmodulep);
}
