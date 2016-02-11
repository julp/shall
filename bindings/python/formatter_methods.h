#pragma once

PyObject *shall_formatter_get_option(PyObject *, PyObject *);
PyObject *shall_formatter_set_option(PyObject *, PyObject *);
PyObject *shall_formatter_start_document(PyObject *, PyObject *);
PyObject *shall_formatter_end_document(PyObject *, PyObject *);
PyObject *shall_formatter_start_token(PyObject *, PyObject *);
PyObject *shall_formatter_end_token(PyObject *, PyObject *);
PyObject *shall_formatter_write_token(PyObject *, PyObject *);
PyObject *shall_formatter_start_lexing(PyObject *, PyObject *);
PyObject *shall_formatter_end_lexing(PyObject *, PyObject *);
