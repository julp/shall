/**
 * @file lib/darray.c
 * @brief dynamic array of elements
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "darray.h"

#define DARRAY_INCREMENT 8
#define DARRAY_MIN_LENGTH 16U

#define OFFSET_TO_ADDR(/*DArray **/ da, /*unsigned int*/ offset) \
    ((da)->data + (da)->element_size * (offset))

#define LENGTH(/*DArray **/ da, /*size_t*/ count) \
    ((da)->element_size * (count))

static inline void darray_maybe_resize_to(DArray *this, size_t total_length)
{
    assert(NULL != this);

    if (UNEXPECTED(total_length >= this->allocated)) {
        this->allocated = ((total_length / DARRAY_INCREMENT) + 1) * DARRAY_INCREMENT;
        this->data = realloc(this->data, this->element_size * this->allocated);
    }
}

static inline void darray_maybe_resize_of(DArray *da, size_t additional_length)
{
    darray_maybe_resize_to(da, da->length + additional_length);
}

void darray_init(DArray *da, size_t length, size_t element_size)
{
    da->data = NULL;
    da->length = da->allocated = 0;
    da->element_size = element_size;
    darray_maybe_resize_to(da, MIN(DARRAY_MIN_LENGTH, length));
}

void darray_destroy(DArray *da)
{
    free(da->data);
    da->data = NULL;
//     free(da); // caller have to do this
}

void darray_clear(DArray *da)
{
    da->length = 0;
    bzero(da->data, da->allocated);
}

bool darray_at(DArray *da, unsigned int offset, void *value)
{
    if (offset < da->length) {
        memcpy(value, OFFSET_TO_ADDR(da, offset), LENGTH(da, 1));
        return true;
    } else {
        return false;
    }
}

bool darray_shift(DArray *da, void *value)
{
    if (da->length > 0) {
        memcpy(value, OFFSET_TO_ADDR(da, 0), LENGTH(da, 1));
        if (--da->length > 0) {
            memmove(OFFSET_TO_ADDR(da, 0), OFFSET_TO_ADDR(da, 1), LENGTH(da, da->length));
        }
        return true;
    } else {
        return false;
    }
}

bool darray_pop(DArray *da, void *value)
{
    if (da->length > 0) {
        memcpy(value, OFFSET_TO_ADDR(da, --da->length), LENGTH(da, 1));
        return true;
    } else {
        return false;
    }
}

void darray_append_all(DArray *da, const void * const data, size_t data_length)
{
    darray_maybe_resize_of(da, data_length);
    memcpy(OFFSET_TO_ADDR(da, da->length), data, LENGTH(da, data_length));
    da->length += data_length;
}

void darray_prepend_all(DArray *da, const void * const data, size_t data_length)
{
    darray_maybe_resize_of(da, data_length);
    if (da->length > 0) {
        memmove(OFFSET_TO_ADDR(da, data_length), OFFSET_TO_ADDR(da, 0), LENGTH(da, da->length));
    }
    memcpy(OFFSET_TO_ADDR(da, 0), data, LENGTH(da, data_length));
    da->length += data_length;
}

void darray_insert_all(DArray *da, unsigned int offset, const void * const data, size_t data_length)
{
    assert(offset <= da->length);

    darray_maybe_resize_of(da, 1);
    if (offset != da->length) {
        memmove(OFFSET_TO_ADDR(da, offset + 1), OFFSET_TO_ADDR(da, offset), LENGTH(da, da->length - offset));
    }
    memcpy(OFFSET_TO_ADDR(da, offset), data, LENGTH(da, data_length));
    da->length += data_length;
}

bool darray_remove_at(DArray *da, unsigned int offset)
{
    if (offset < da->length) {
        memmove(OFFSET_TO_ADDR(da, offset), OFFSET_TO_ADDR(da, offset + 1), LENGTH(da, da->length - offset - 1));
        da->length--;
        return true;
    } else {
        return false;
    }
}

void darray_remove_range(DArray *da, unsigned int from, unsigned int to)
{
    size_t diff;

    assert(from < da->length);
    assert(to < da->length);
    assert(from <= to);

    diff = to - from + 1;
    memmove(OFFSET_TO_ADDR(da, to + 1), OFFSET_TO_ADDR(da, from), LENGTH(da, da->length - diff));
    da->length -= diff;
}

void darray_swap(DArray *da, unsigned int offset1, unsigned int offset2)
{
    int i;
    uint8_t tmp, *fp, *tp;

    assert(offset1 != offset2);
    assert(offset1 < da->length);
    assert(offset2 < da->length);

    fp = OFFSET_TO_ADDR(da, offset1);
    tp = OFFSET_TO_ADDR(da, offset2);
    for (i = da->element_size; i > 0; i--) {
        tmp = *fp;
        *fp++ = *tp;
        *tp++ = tmp;
    }
}

void darray_set_size(DArray *da, size_t length)
{
    if (length < da->length) {
        da->length = length;
    } else {
        darray_maybe_resize_to(da, length);
    }
}

size_t darray_length(DArray *da)
{
    return da->length;
}

void darray_sort(DArray *da, CmpFuncArg cmpfn, void *arg)
{
    assert(NULL != da);
    assert(NULL != cmpfn);

    qsort_r(da->data, da->length, da->element_size,
#ifdef BSD
        arg, cmpfn
#else
        cmpfn, arg
#endif
    );
}

static void darray_iterator_first(const void *collection, void **state)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;
    *(uint8_t **) state = ary->data;
}

static void darray_iterator_last(const void *collection, void **state)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;
    *(uint8_t **) state = ary->data + LENGTH(ary, ary->length - 1);
}

static bool darray_iterator_is_valid(const void *collection, void **state)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;

    return *((uint8_t **) state) >= ary->data && *((uint8_t **) state) < (ary->data + LENGTH(ary, ary->length));
}

static void darray_iterator_current(const void *collection, void **state, void **key, void **value)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;
    if (NULL != value) {
        *value = *(uint8_t **) state;
    }
    if (NULL != key) {
        *((uint64_t *) key) = *((uint8_t **) state) - ary->data;
    }
}

static void darray_iterator_next(const void *collection, void **state)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;
    *((uint8_t **) state) += ary->element_size;
}

static void darray_iterator_previous(const void *collection, void **state)
{
    DArray *ary;

    assert(NULL != collection);
    assert(NULL != state);

    ary = (DArray *) collection;
    *((uint8_t **) state) -= ary->element_size;
}

void darray_to_iterator(Iterator *it, DArray *da)
{
    iterator_init(
        it, da, NULL,
        darray_iterator_first, darray_iterator_last,
        darray_iterator_current,
        darray_iterator_next, darray_iterator_previous,
        darray_iterator_is_valid,
        NULL
    );
}
