#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifndef DTOR_FUNC
# define DTOR_FUNC
typedef void (*DtorFunc)(void *);
#endif /* !DOTR_FUNC */
typedef int (*CmpFunc)(const void *, const void *);

typedef struct DListElement
{
    struct DListElement *next;
    struct DListElement *prev;
    void *data;
} DListElement;

typedef struct
{
    size_t length;
    DtorFunc dtor;
    DListElement *head;
    DListElement *tail;
} DList;

bool dlist_append(DList *, void *);
void dlist_clear(DList *);
bool dlist_empty(DList *);
DListElement *dlist_find_first(DList *, CmpFunc, void *);
DListElement *dlist_find_last(DList *, CmpFunc, void *);
void dlist_init(DList *, DtorFunc);
bool dlist_insert_after(DList *, DListElement *, void *);
bool dlist_insert_before(DList *, DListElement *, void *);
size_t dlist_length(DList *);
bool dlist_prepend(DList *, void *);
void dlist_remove_head(DList *);
void dlist_remove_link(DList *, DListElement *);
void dlist_remove_tail(DList *);

#ifndef WITHOUT_ITERATOR
# include "iterator.h"

void dlist_to_iterator(Iterator *, DList *);
#endif /* !WITHOUT_ITERATOR */
