/**
 * @file lib/dlist.c
 * @brief double linked list
 */

#include <stdlib.h>

#include "dlist.h"

void dlist_init(DList *list, DtorFunc dtor)
{
    list->length = 0;
    list->head = list->tail = NULL;
    list->dtor = dtor;
}

size_t dlist_length(DList *list)
{
    return list->length;
}

void dlist_destroy(DList *list)
{
    DListElement *tmp, *last;

    tmp = list->head;
    while (tmp) {
        last = tmp;
        tmp = tmp->next;
        if (NULL != list->dtor) {
            list->dtor(last->data);
        }
        free(last);
    }
}

bool dlist_append(DList *list, void *data)
{
    DListElement *tmp;

    if (NULL == (tmp = malloc(sizeof(*tmp)))) {
        return false;
    }
    tmp->next = NULL;
    tmp->data = data;
    if (NULL != list->tail) {
        list->tail->next = tmp;
        tmp->prev = list->tail;
        list->tail = tmp;
    } else {
        list->head = list->tail = tmp;
        tmp->prev = NULL;
    }
    ++list->length;

    return true;
}

DListElement *dlist_find_first(DList *list, CmpFunc cmp, void *data)
{
    DListElement *el;

    for (el = list->head; NULL != el; el = el->next) {
        if (0 == cmp(el->data, data)) {
            return el;
        }
    }

    return NULL;
}

DListElement *dlist_find_last(DList *list, CmpFunc cmp, void *data)
{
    DListElement *el;

    for (el = list->tail; NULL != el; el = el->prev) {
        if (0 == cmp(el->data, data)) {
            return el;
        }
    }

    return NULL;
}

bool dlist_insert_before(DList *list, DListElement *sibling, void *data)
{
    return true;
}

bool dlist_insert_after(DList *list, DListElement *sibling, void *data)
{
    return true;
}

bool dlist_empty(DList *list)
{
    return NULL == list->head;
}

bool dlist_prepend(DList *list, void *data)
{
    DListElement *tmp;

    if (NULL == (tmp = malloc(sizeof(*tmp)))) {
        return false;
    }
    tmp->data = data;
    tmp->prev = NULL;
    if (NULL != list->head) {
        tmp->next = list->head;
    } else {
        tmp->next = NULL;
        list->tail = tmp;
    }
    list->head = tmp;
    ++list->length;

    return true;
}

void dlist_remove_head(DList *list)
{
    DListElement *tmp;

    if (NULL != list->head) {
        tmp = list->head;
        list->head = list->head->next;
        if (NULL != list->head) {
            list->head->prev = NULL;
        } else {
            list->head = list->tail = NULL;
        }
        if (NULL != list->dtor) {
            list->dtor(tmp->data);
        }
        free(tmp);
        --list->length;
    }
}

void dlist_remove_link(DList *list, DListElement *element)
{
    if (NULL != element->prev) {
        element->prev->next = element->next;
    }
    if (NULL != element->next) {
        element->next->prev = element->prev;
    }
    if (element == list->head) {
        list->head = list->head->next;
        if (list->head) {
            list->head->prev= NULL;
        }
    }
    if (element == list->tail) {
        list->tail = list->tail->prev;
        if (list->tail) {
            list->tail->next = NULL;
        }
    }
    free(element);
    --list->length;
}

void dlist_remove_tail(DList *list)
{
    DListElement *tmp;

    if (list->tail) {
        tmp = list->tail;
        list->tail = list->tail->prev;
        if (NULL != list->tail) {
            list->tail->next = NULL;
        } else {
            list->head = list->tail = NULL;
        }
        if (NULL != list->dtor) {
            list->dtor(tmp->data);
        }
        free(tmp);
        --list->length;
    }
}
