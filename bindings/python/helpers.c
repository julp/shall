#include "common.h"

void list_append_string_cb(const char *string, void *data)
{
    PyList_Append((PyObject *) data, PyUnicode_FromString(string));
}
