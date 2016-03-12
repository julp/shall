#include <stdlib.h>
#include <string.h>

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

#define U_IS_TRAIL(c) \
    (0xDC00 == ((c) & 0xFFFFFC00))

#define IS_XDIGIT(c) \
    (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

/**
 * Check if a code point is valid
 *
 * @param start position where the token starts (ie should be YYTEXT)
 * @param limit the end of the text (ie should be YYLIMIT)
 * @param cursor the cursor (YYCURSOR) to move after the trail surrogate when the pair is valid
 * @param prefix the string which starts an Unicode escape sequence (eg "\\u{" for PHP)
 * @param prefix_len its length
 * @param suffix the string which ends an Unicode escape sequence (eg "}" for PHP), NULL if unused
 * @param suffix_len its length (0 if unused)
 * @param min_len the minimum length defined by the language for an Unicode escape sequence because
 * some of them permit a variable length. If the length is fixed, define min_length = max_length.
 * @param max_len the maximum length of an Unicode escape sequence
 * @param flags a bit mask of:
 * - 16 to allow surrogates
 * - 32 to forbid surrogates
 *
 * @return false on an invalid code point
 */
bool check_codepoint(const YYCTYPE *start, const YYCTYPE * const limit, const YYCTYPE **cursor, const char *prefix, size_t prefix_len, const char *suffix, size_t suffix_len, size_t min_len, size_t max_len, uint8_t flags)
{
    bool ok;
    size_t i;
    uint32_t cp;
    const YYCTYPE *p;

    cp = 0;
    ok = true;
    p = start + prefix_len;
    assert(0 == memcmp(prefix, start, prefix_len));
    for (i = 0; IS_XDIGIT(*p) && i < max_len; i++) {
        cp *= 16;
        cp += ((*p >= '0' && *p <= '9') ? (*p - '0') : 10 + ((*p >= 'a' && *p <= 'f') ? (*p - 'a') : (*p - 'A')));
        ++p;
    }
    if (0 != suffix_len) {
        assert(0 == memcmp(suffix, p, suffix_len));
    }
    p += suffix_len;
    if (U_IS_SURROGATE(cp)) {
        if (!HAS_FLAG(flags, 16) || U_IS_SURROGATE_TRAIL(cp)) {
            ok = false;
        } else {
            if ((limit - p) < (prefix_len + min_len) || 0 != memcmp(prefix, p, prefix_len)) {
                ok = false;
            } else {
                cp = 0;
                p += prefix_len;
                for (i = 0; IS_XDIGIT(*p) && i < max_len; i++) {
                    cp *= 16;
                    cp += ((*p >= '0' && *p <= '9') ? (*p - '0') : 10 + ((*p >= 'a' && *p <= 'f') ? (*p - 'a') : (*p - 'A')));
                    ++p;
                }
                ok = U_IS_TRAIL(cp);
                if (0 != suffix_len) {
                    assert(0 == memcmp(suffix, p, suffix_len));
                }
                p += suffix_len;
                if (ok) {
                    *cursor = p; // extend YYCURSOR to next sequence if present
                }
            }
        }
    }

    return ok;
}
