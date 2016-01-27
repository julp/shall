#include "common.h"

void array_push_string_cb(const char *string, void *data)
{
    add_next_index_string_copy((zval *) data, string);
}
