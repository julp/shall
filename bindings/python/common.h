#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <shall/shall.h>

#define DEBUG

#ifdef DEBUG
# include <stdio.h>
# include <stdarg.h>
# define debug(format, ...) \
    fprintf(stderr, format "\n", ## __VA_ARGS__)
#else
# define debug(format, ...)
#endif /* DEBUG */
