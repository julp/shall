#pragma once

#include <shall/formatter.h>

typedef struct {
    PyObject_HEAD
    Formatter *fmt;
} ShallFormatterObject;

typedef struct {
    PyObject *self, *options;
} PythonFormatterData;

PyTypeObject ShallFormatterBaseType;

void register_formatter_class(PyObject *);
