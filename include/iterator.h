#pragma once

#include <stdbool.h>

// TODO
#define SHALL_API

typedef void (*iterator_first_t)(const void *, void **);
typedef void (*iterator_last_t)(const void *, void **);
typedef void (*iterator_current_t)(const void *, void **, void **);
typedef void (*iterator_next_t)(const void *, void **);
typedef void (*iterator_previous_t)(const void *, void **);
typedef bool (*iterator_is_valid_t)(const void *, void **);
typedef void (*iterator_close_t)(void *);

typedef struct _Iterator Iterator;

struct _Iterator {
    void *state;
    const void *collection;
    iterator_first_t first;
    iterator_last_t last;
    iterator_current_t current;
    iterator_next_t next;
    iterator_previous_t previous;
    iterator_is_valid_t valid;
    iterator_close_t close;
};

SHALL_API void iterator_init(
    Iterator *,
    const void *,
    void *,
    iterator_first_t,
    iterator_last_t,
    iterator_current_t,
    iterator_next_t,
    iterator_previous_t,
    iterator_is_valid_t,
    iterator_close_t
);
SHALL_API void iterator_first(Iterator *);
SHALL_API void iterator_last(Iterator *);
SHALL_API void *iterator_current(Iterator *);
SHALL_API void iterator_next(Iterator *);
SHALL_API void iterator_previous(Iterator *);
SHALL_API bool iterator_is_valid(Iterator *);
SHALL_API void iterator_close(Iterator *);

SHALL_API void null_terminated_ptr_array_to_iterator(Iterator *, void **);
