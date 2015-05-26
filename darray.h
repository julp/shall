#ifndef DARRAY_H

# define DARRAY_H

# include "bool.h"

typedef struct {
    uint8_t *data;
    size_t length;
    size_t allocated;
    size_t element_size;
} DArray;

# define darray_prepend(/*DArray **/ da, ptr) \
    darray_prepend((da), (ptr), 1)

# define darray_push(/*DArray **/ da, ptr) \
    darray_append((da), (ptr))

# define darray_append(/*DArray **/ da, ptr) \
    darray_append_all((da), (ptr), 1)

# define darray_insert(/*DArray **/ da, /*unsigned int*/ offset, ptr) \
    darray_insert((da), (offset), (ptr), 1)

# define darray_at_unsafe(/*DArray **/ da, /*unsigned int*/ offset, T) \
    ((T *) ((void *) (da)->data))[(offset)]

# define darray_top_unsafe(/*DArray **/ da, T) \
    darray_at_unsafe(da, (da)->length - 1, T)

void darray_append_all(DArray *, const void * const, size_t);
bool darray_at(DArray *, unsigned int, void *);
void darray_clear(DArray *);
void darray_destroy(DArray *);
void darray_init(DArray *, size_t, size_t);
void darray_insert_all(DArray *, unsigned int, const void * const, size_t);
size_t darray_length(DArray *);
bool darray_pop(DArray *, void *);
void darray_prepend_all(DArray *, const void * const, size_t);
bool darray_remove_at(DArray *, unsigned int);
void darray_remove_range(DArray *, unsigned int, unsigned int);
void darray_set_size(DArray *, size_t);
bool darray_shift(DArray *, void *);
void darray_swap(DArray *, unsigned int, unsigned int);

#endif /* !DARRAY_H */
