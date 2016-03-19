/**
 * @file lib/iterator.c
 * @brief An iterator is an abstraction layer to easily traverse a collection of (read only) items.
 *
 * This is simple as this:
 * \code
 *   Iterator it;
 *
 *   // initialize the iterator
 *   my_collection_to_iterator(&it);
 *   // traverse it (forward order here)
 *   for (iterator_first(&it); iterator_is_valid(&it); iterator_next(&it)) {
 *       const MyItem *item;
 *
 *       item = (const MyItem *) iterator_current(&it);
 *       // use item
 *   }
 *   // we're done, "close" the iterator
 *   iterator_close(&it);
 * \endcode
 *
 * You can even known if your collection is empty by writing:
 * \code
 *   Iterator it;
 *
 *   // initialize the iterator
 *   my_collection_to_iterator(&it);
 *   iterator_first(&it);
 *   if (iterator_is_valid(&it)) {
 *       // collection is not empty, "normal" traversal
 *       do {
 *           const MyItem *item;
 *
 *           item = (const MyItem *) iterator_current(&it);
 *           // use item
 *           iterator_next(&it); // have to be the last instruction of the loop
 *       } while (iterator_is_valid(&it));
 *   } else {
 *       // collection is empty
 *   }
 *   iterator_close(&it);
 * \endcode
 */

#include <stddef.h>
#include <stdlib.h>

#include "cpp.h"
#include "iterator.h"

static const Iterator NULL_ITERATOR = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/**
 * Initialize an iterator
 *
 * @param collection the collection to iterate on
 * @param state a data passed to the callbacks to mark the current position in the collection
 * @param first a callback to (re)initialize the current position on the first item of the
 * collection (may be NULL the collection is not intended to be traversed in forward order)
 * @param last a callback to (re)initialize the current position on the last item of the
 * collection (may be NULL the collection is not intended to be traversed in reverse order)
 * @param current the callback to get the current element
 * @param next the callback to move the internal position to the next element
 * (may be NULL the collection is not intended to be traversed in forward order)
 * @param previous the callback to move the internal position to the previous element
 * (may be NULL the collection is not intended to be traversed in reverse order)
 * @param valid the callback to known if the internal position is still valid after
 * moving it to its potential next or previous element
 * @param close a callback to free internal state (NULL if you have nothing to free)
 */
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
) {
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

/**
 * End the iterator
 * (it shouldn't no longer be used)
 *
 * @param it the iterator
 */
SHALL_API void iterator_close(Iterator *it)
{
    if (NULL != it->close) {
        it->close(it->state);
    }
    *it = NULL_ITERATOR;
}

/**
 * Get the current value
 *
 * @param it the iterator
 */
SHALL_API void *iterator_current(Iterator *it)
{
    void *value = NULL;

    if (NULL != it->current) {
        it->current(it->collection, &it->state, &value);
    }

    return value;
}

/**
 * (Re)set internal position to the first element (if supported)
 *
 * @param it the iterator to (re)set
 */
SHALL_API void iterator_first(Iterator *it)
{
    if (NULL != it->first) {
        it->first(it->collection, &it->state);
    }
}

/**
 * (Re)set internal position to the last element (if supported)
 *
 * @param it the iterator to (re)set
 */
SHALL_API void iterator_last(Iterator *it)
{
    if (NULL != it->last) {
        it->last(it->collection, &it->state);
    }
}

/**
 * Move internal position to the next element (if supported)
 *
 * @param it the iterator to move forward
 */
SHALL_API void iterator_next(Iterator *it)
{
    if (NULL != it->next) {
        it->next(it->collection, &it->state);
    }
}

/**
 * Move internal position to the previous element (if supported)
 *
 * @param it the iterator to move backward
 */
SHALL_API void iterator_previous(Iterator *it)
{
    if (NULL != it->previous) {
        it->previous(it->collection, &it->state);
    }
}

/**
 * Determine if the current position of the iterator is still valid
 * If not, the only valid operations are a reset of its internal
 * cursor to its first or last element (if any) or to close it if
 * you are done
 *
 * @param it the iterator
 */
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

/**
 * Iterate on a NULL terminated array of pointers
 *
 * @note the avantage of this iterator is that it doesn't need (allocate)
 * any additionnal memory for its use. (but keep calling iterator_close for
 * one of this kind)
 *
 * @param it the iterator to initialize
 * @param array the array to iterate on
 */
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

    return NULL != *((char **) (s->ptr + s->field_offset));
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

/**
 * Iterate on an array of struct (or union) where one of its field is sentineled
 * by a NULL pointer.
 *
 * @param it the iterator to initialize
 * @param array the array to iterate on
 * @param element_size the size of an element of this array
 * @param field_offset the offset of the member to test against NULL
 * (use offsetof to define it)
 */
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
