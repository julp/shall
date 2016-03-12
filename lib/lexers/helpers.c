#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

int named_elements_cmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return strcmp_l(na->name, na->name_len, nb->name, nb->name_len);
}

int named_elements_casecmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return ascii_strcasecmp_l(na->name, na->name_len, nb->name, nb->name_len);
}

#define U_IS_SURROGATE(c) \
    (0xD800 == ((c) & 0xFFFFF800))

#define U_IS_SURROGATE_LEAD(c) \
    (0 == ((c) & 0x400))

#define U_IS_SURROGATE_TRAIL(c) \
    (0 != ((c) & 0x400))

/**
 * Check if a code point is valid
 *
 * @param start
 * @param end
 * @param flags a bit mask of:
 * - 16 to allow surrogates
 * - 32 to forbid surrogates
 *
 * @return false on an invalid code point
 */
bool check_codepoint(const uint8_t *start, const uint8_t * const end, uint8_t flags)
{
    bool ok;
    uint32_t cp;
    const uint8_t *p;

    cp = 0;
    ret = true;
    for (p = start; start < end; p++) {
        cp = cp << 4 + ((*p >= '0' && *p <= '9') ? (*p - '0') : ((*p >= 'a' && *p <= 'f') ? (*p - 'a') : (*p - 'A'));
    }
    if (U_IS_SURROGATE(cp)) {
        if (flags & 32) {
            ret = false;
        }
    }

    return ret;
}
