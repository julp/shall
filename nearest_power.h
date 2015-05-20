#ifndef NEAREST_POWER_H

# define NEAREST_POWER_H

# include <limits.h>

# define HALF_SIZE (1UL << (sizeof(size_t) * CHAR_BIT - 1))

static inline size_t nearest_power(size_t requested_length, size_t minimal)
{
    if (requested_length > HALF_SIZE) {
        return HALF_SIZE;
    } else {
        int i = 1;
        requested_length = MAX(requested_length, minimal);
        while ((1UL << i) < requested_length) {
            i++;
        }

        return (1UL << i);
    }
}

#endif /* !NEAREST_POWER_H */
