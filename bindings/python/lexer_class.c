#include "common.h"
#include "options.h"
#include "helpers.h"
#include "lexer_class.h"
#include "lexer_methods.h"

static PyObject *lexers;

static PyMethodDef Shall_Lexer_Methods[] = {
    { "get_option",    (PyCFunction) shall_lexer_get_option,    METH_VARARGS, "TODO" },
    { "set_option",    (PyCFunction) shall_lexer_set_option,    METH_VARARGS, "TODO" },
    { "get_name",      (PyCFunction) shall_lexer_get_name,      METH_CLASS | METH_NOARGS, "TODO" },
    { "get_aliases",   (PyCFunction) shall_lexer_get_aliases,   METH_CLASS | METH_NOARGS, "TODO" },
    { "get_mimetypes", (PyCFunction) shall_lexer_get_mimetypes, METH_CLASS | METH_NOARGS, "TODO" },
    { NULL, NULL, 0, NULL }
};

static PyObject *Shall_Lexer_new(PyTypeObject *, PyObject *, PyObject *);
static void shall_lexer_base_dealloc(ShallLexerBaseObject *);
static int shall_lexer_base_init(ShallLexerBaseObject *, PyObject *, PyObject *);

PyTypeObject ShallLexerBaseType = {
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

/* helpers */

Lexer *python_lexer_unwrap(void *object)
{
    ShallLexerObject *o;

    o = (ShallLexerObject *) object;

    return o->lexer;
}

void _create_lexer(const LexerImplementation *imp, PyObject *options, PyObject **ret)
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

static void create_lexer_class_cb(const LexerImplementation *imp, void *data)
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
    PyModule_AddObject((PyObject *) data, imp_name, (PyObject *) type);
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

void register_lexer_class(PyObject *shallmodulep)
{
    Py_INCREF(&ShallLexerBaseType);
    PyModule_AddObject(shallmodulep, "BaseLexer", (PyObject *) &ShallLexerBaseType);

    lexers = PyDict_New();
    Py_INCREF(lexers);

    lexer_implementation_each(create_lexer_class_cb, shallmodulep);
}
