#pragma once

typedef struct {
    PyObject_HEAD
//     Lexer *lexer;
} ShallLexerBaseObject;

typedef struct {
    ShallLexerBaseObject base;
    Lexer *lexer;
} ShallLexerObject;

PyTypeObject ShallLexerBaseType;

Lexer *python_lexer_unwrap(void *);
void register_lexer_class(PyObject *);
void _create_lexer(const LexerImplementation *, PyObject *, PyObject **);
