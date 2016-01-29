#include <stddef.h>
#include <stdlib.h>

#include "cpp.h"
#include "iterator.h"

static const Iterator NULL_ITERATOR = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

SHALL_API void iterator_init(
    Iterator *it,
    const void *collection,
    void *state,
    iterator_first_t first,
    iterator_last_t last,
    iterator_current_t current,
    iterator_next_t next,
    iterator_previous_t previous,
    iterator_is_valid_t valid,
    iterator_close_t close
)
{
    it->state = state;
    it->collection = collection;
    it->first = first;
    it->last = last;
    it->current = current;
    it->next = next;
    it->previous = previous;
    it->valid = valid;
    it->close = close;
}

SHALL_API void iterator_close(Iterator *it)
{
    if (NULL != it->close) {
        it->close(it->state);
    }
    *it = NULL_ITERATOR;
}

SHALL_API void *iterator_current(Iterator *it)
{
    void *value = NULL;

    if (NULL != it->current) {
        it->current(it->collection, &it->state, &value);
    }

    return value;
}

SHALL_API void iterator_first(Iterator *it)
{
    if (NULL != it->first) {
        it->first(it->collection, &it->state);
    }
}

SHALL_API void iterator_last(Iterator *it)
{
    if (NULL != it->last) {
        it->last(it->collection, &it->state);
    }
}

SHALL_API void iterator_next(Iterator *it)
{
    if (NULL != it->next) {
        it->next(it->collection, &it->state);
    }
}

SHALL_API void iterator_previous(Iterator *it)
{
    if (NULL != it->previous) {
        it->previous(it->collection, &it->state);
    }
}

SHALL_API bool iterator_is_valid(Iterator *it)
{
    return it->valid(it->collection, &it->state);
}

/* ========== NULL terminated (pointers) array ========== */

static void null_terminated_ptr_array_iterator_first(const void *collection, void **state)
{
    assert(NULL != collection);
    assert(NULL != state);

    *(void ***) state = (void **) collection;
}

static bool null_terminated_ptr_array_iterator_is_valid(const void *UNUSED(collection), void **state)
{
    assert(NULL != state);

    return NULL != **((void ***) state);
}

static void null_terminated_ptr_array_iterator_current(const void *collection, void **state, void **value)
{
    assert(NULL != collection);
    assert(NULL != state);
    assert(NULL != value);

    *value = **(void ***) state;
}

static void null_terminated_ptr_array_iterator_next(const void *UNUSED(collection), void **state)
{
    assert(NULL != state);

    ++*((void ***) state);
}

SHALL_API void null_terminated_ptr_array_to_iterator(Iterator *it, void **array)
{
    iterator_init(
        it, array, NULL,
        null_terminated_ptr_array_iterator_first, NULL,
        null_terminated_ptr_array_iterator_current,
        null_terminated_ptr_array_iterator_next, NULL,
        null_terminated_ptr_array_iterator_is_valid,
        NULL
    );
}

/* ========== NULL sentineled field terminated array of struct/union ========== */

typedef struct {
    const char *ptr;
    size_t element_size;
    size_t field_offset;
} nsftas_t /*null_sentineled_field_terminated_array_state*/;

static void null_sentineled_field_terminated_array_iterator_first(const void *collection, void **state)
{
    nsftas_t *s;

    assert(NULL != collection);
    assert(NULL != state);
    assert(NULL != *state);

    s = (nsftas_t *) *state;
    s->ptr = collection;
}

static bool null_sentineled_field_terminated_array_iterator_is_valid(const void *UNUSED(collection), void **state)
{
    nsftas_t *s;

    assert(NULL != state);
    assert(NULL != *state);

    s = (nsftas_t *) *state;

    return NULL != (const void *) *(s->ptr + s->field_offset);
}

static void null_sentineled_field_terminated_array_iterator_current(const void *UNUSED(collection), void **state, void **value)
{
    nsftas_t *s;

    assert(NULL != state);
    assert(NULL != value);
    assert(NULL != *state);

    s = (nsftas_t *) *state;
    *value = (void *) s->ptr;
}

static void null_sentineled_field_terminated_array_iterator_next(const void *UNUSED(collection), void **state)
{
    nsftas_t *s;

    assert(NULL != state);
    assert(NULL != *state);

    s = (nsftas_t *) *state;
    s->ptr += s->element_size;
}

SHALL_API void null_sentineled_field_terminated_array_to_iterator(Iterator *it, void *array, size_t element_size, size_t field_offset)
{
    nsftas_t *s;

    s = malloc(sizeof(*s));
    s->ptr = (const char *) array;
    s->element_size = element_size;
    s->field_offset = field_offset;

    iterator_init(
        it, array, s,
        null_sentineled_field_terminated_array_iterator_first, NULL,
        null_sentineled_field_terminated_array_iterator_current,
        null_sentineled_field_terminated_array_iterator_next, NULL,
        null_sentineled_field_terminated_array_iterator_is_valid,
        free
    );
}