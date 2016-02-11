#include <shall/tokens.h>

#include "common.h"
#include "options.h"
#include "lexer_class.h"
#include "formatter_class.h"

#if 0
PyObject* PyImport_AddModuleObject(PyObject *name);
PyObject* PyImport_AddModule(const char *name);

https://docs.python.org/3/c-api/arg.html
https://docs.python.org/2/c-api/dict.html
https://docs.python.org/2/c-api/string.html
#endif

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
#if 0
    if (PyArg_ParseTuple(args, "s#O!O!", &string, &string_len, &ShallLexerBaseType, &lexer, &ShallFormatterBaseType, &fmt)/* && 1 == PyObject_IsInstance(lexer, (PyObject *) &ShallLexerBaseType)*/) {
        char *dest;
        size_t dest_len;

// print issubclass(shall.PHP, shall.Base)
// printf("PyObject_IsInstance = %d\n", PyObject_IsInstance(lexer, (PyObject *) &ShallLexerBaseType));
        highlight_string(string, (size_t) string_len, &dest, &dest_len, ((ShallFormatterObject *) fmt)->fmt, 1, &((ShallLexerObject *) lexer)->lexer);
        ret = PyUnicode_FromStringAndSize(dest, dest_len);
    }
#else
    if (PyArg_ParseTuple(args, "s#O!O!", &string, &string_len, &PyList_Type, &lexer, &ShallFormatterBaseType, &fmt)) {
        bool ok;
        char *dest;
        size_t dest_len;
        /*Py_s*/size_t i, lexerc;

        lexerc = (size_t) PyList_Size(lexer);
        ok = lexerc > 0;
        Lexer *lexerv[lexerc];
        for (i = 0; ok && i < lexerc; i++) {
            PyObject *item;

            item = PyList_GET_ITEM(lexer, i);
            ok &= /*NULL != item && */1 == PyObject_IsInstance(item, (PyObject *) &ShallLexerBaseType);
//             ok &= 1 == PyObject_TypeCheck(item, &ShallLexerBaseType);
            if (ok) {
                lexerv[i] = ((ShallLexerObject *) item)->lexer;
            }
        }
        if (ok) {
            highlight_string(string, (size_t) string_len, &dest, &dest_len, ((ShallFormatterObject *) fmt)->fmt, lexerc, lexerv);
            ret = PyUnicode_FromStringAndSize(dest, dest_len);
        } else {
            ret = NULL;
            PyErr_SetString(PyExc_TypeError, "lexers");
        }
    }
#endif
    Py_XINCREF(ret);

    return ret;
}

static PyObject *shall_sample(PyObject *self, PyObject *args)
{
    PyObject *ret, *fmt;

    ret = Py_None;
    if (PyArg_ParseTuple(args, "O!", &ShallFormatterBaseType, &fmt)) {
        char *dest;
        size_t dest_len;

        highlight_sample(&dest, &dest_len, ((ShallFormatterObject *) fmt)->fmt);
        ret = PyUnicode_FromStringAndSize(dest, dest_len);
    }
    Py_INCREF(ret);

    return ret;
}

static PyMethodDef ShallMethods[] = {
    { "sample",             (PyCFunction) shall_sample,             METH_VARARGS, "TODO" },
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

#if PY_MAJOR_VERSION < 3
void init_shall_token(void)
{
    PyObject *shalltokenmodulep = Py_InitModule("shall.token", NULL);
    PyModule_AddObject(shallmodulep, "token", shalltokenmodulep);

    debug("init_shall_token called\n");
# define TOKEN(constant, parent, description, cssclass) \
    PyModule_AddIntConstant(shalltokenmodulep, #constant, constant);
//     PyDict_SetItemString(tokens, #constant, PyLong_FromLong(constant));
# include <shall/keywords.h>
# undef TOKEN
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

    register_lexer_class(shallmodulep);
    register_formatter_class(shallmodulep);

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
