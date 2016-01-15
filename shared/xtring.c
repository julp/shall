#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "xtring.h"
#include "nearest_power.h"

#ifdef DEBUG
# define STRING_INITIAL_LENGTH 1 /* Voluntarily small for development/test */
#else
# define STRING_INITIAL_LENGTH 4096
#endif /* DEBUG */

#define STRING_INIT_LEN(/*String **/ str, /*size_t*/ length) \
    do {                                                     \
        str = mem_new(*str);                                 \
        str->ptr = NULL;                                     \
        str->allocated = str->len = 0;                       \
        _string_maybe_expand_to(str, (length));              \
    } while (0);

/* ==================== private helpers for growing up ==================== */

static HOT void _string_maybe_expand_of(String *str, size_t additional_length)
{
    assert(NULL != str);

    if (UNEXPECTED(str->len + additional_length >= str->allocated)) {
        str->allocated = nearest_power(str->len + additional_length, STRING_INITIAL_LENGTH);
        str->ptr = mem_renew(str->ptr, *str->ptr, str->allocated + 1);
    }
}

static HOT void _string_maybe_expand_to(String *str, size_t total_length)
{
    assert(NULL != str);

    if (UNEXPECTED(total_length >= str->allocated)) {
        str->allocated = nearest_power(total_length, STRING_INITIAL_LENGTH);
        str->ptr = mem_renew(str->ptr, *str->ptr, str->allocated + 1);
    }
}

/* ==================== creation and cloning ==================== */

String *string_new(void) /* WARN_UNUSED_RESULT */
{
    return string_sized_new(STRING_INITIAL_LENGTH);
}

String *string_dup(const String *str) /* WARN_UNUSED_RESULT */
{
    String *copy;

    assert(NULL != str);

    copy = mem_new(*copy);
    copy->allocated = copy->len = str->len;
    copy->ptr = mem_new_n(*copy->ptr, copy->allocated + 1);
    memcpy(copy->ptr, str->ptr, str->len);
    copy->ptr[copy->len] = 0;

    return copy;
}

String *string_sized_new(size_t requested) /* WARN_UNUSED_RESULT */
{
    String *str;

    STRING_INIT_LEN(str, requested);
    *str->ptr = '\0';

    return str;
}

String *string_dup_string_len(const char *from, size_t length)
{
    String *str;

    assert(NULL != from);

    STRING_INIT_LEN(str, length);
    str->len = length;
    memcpy(str->ptr, from, str->len);
    str->ptr[str->len] = '\0';

    return str;
}

String *string_dup_string(const char *from)
{
    assert(NULL != from);

    return string_dup_string_len(from, strlen(from));
}

String *string_adopt_string_len(char *from, size_t len)
{
    String *str;

    assert(NULL != from);

    str = mem_new(*str);
    str->len = len;
    str->allocated = len + 1;
    str->ptr = from;

    return str;
}

String *string_adopt_string(char *from)
{
    assert(NULL != from);

    return string_adopt_string_len(from, strlen(from));
}

/* ==================== prefix/suffix ==================== */

int string_startswith(String *str, const char *prefix, size_t length)
{
    assert(NULL != str);

    return str->len >= length && 0 == memcmp(str->ptr, prefix, length);
}

int string_endswith(String *str, const char *suffix, size_t length)
{
    assert(NULL != str);

    return str->len >= length && 0 == memcmp(str->ptr + str->len - length, suffix, length);
}

/* ==================== destruction ==================== */

void string_destroy(String *str)
{
    assert(NULL != str);

    free(str->ptr);
    free(str);
}

char *string_orphan(String *str)
{
    char *ret;

    assert(NULL != str);

    ret = str->ptr;
    free(str);

    return ret;
}

/* ==================== basic operations (insert, append, replace, delete) ==================== */

int32_t string_subreplace_len(String *str, const char *replacement, size_t replacement_length, size_t position, size_t length)
{
    int32_t diff_len;

    assert(NULL != str);
    assert(position <= str->len);

    diff_len = replacement_length - length;
    if (diff_len > 0) {
        _string_maybe_expand_of(str, (size_t) diff_len);
    }
    if (replacement_length != length) {
        // TODO: assume str->len - position - length > 0?
        memmove(str->ptr + position + length + diff_len, str->ptr + position + length, str->len - position - length);
    }
    if (replacement_length > 0) {
        if (replacement >= str->ptr && replacement <= str->ptr + str->len) {
            size_t offset = replacement - str->ptr;
            size_t precount = 0;

            replacement = str->ptr + offset;
            if (offset < position) {
                precount = MIN(replacement_length, position - offset);
                memcpy(str->ptr + position, replacement, precount);
            }
            if (length > precount) {
                memcpy(str->ptr + position + precount, replacement + precount + replacement_length, replacement_length - precount);
            }
        } else {
            memcpy(str->ptr + position, replacement, replacement_length);
        }
    }
    str->len += diff_len;
    str->ptr[str->len] = '\0';

    return diff_len;
}

int32_t string_delete_len(String *str, size_t position, size_t length)
{
    return string_subreplace_len(str, NULL, 0, position, length);
}

int32_t string_insert_len(String *str, size_t position, const char *c, size_t length)
{
    return string_subreplace_len(str, c, length, position, 0);
}

void string_append_char(String *str, char c)
{
    string_insert_len(str, str->len, &c, 1);
}

void string_append_string(String *str, const char *suffix)
{
    string_insert_len(str, str->len, suffix, strlen(suffix));
}

void string_append_string_len(String *str, const char *suffix, size_t len)
{
    string_insert_len(str, str->len, suffix, len);
}

void string_prepend_char(String *str, char c)
{
    string_insert_len(str, 0, &c, 1);
}

void string_prepend_string(String *str, const char *prefix)
{
    string_insert_len(str, 0, prefix, strlen(prefix));
}

void string_prepend_string_len(String *str, const char *prefix, size_t len)
{
    string_insert_len(str, 0, prefix, len);
}

/* ==================== trimming (WS and/or EOL) ==================== */

void string_chomp(String *str)
{
    assert(NULL != str);

    if (str->len > 0) {
        switch (str->ptr[str->len - 1]) {
            case '\r':
            case '\v':
            case '\f':
#if 0
            case 0x85: /* ISO-8859-1: next line character */
#endif
                str->ptr[--str->len] = 0;
                break;
            case '\n':
                str->ptr[--str->len] = 0;
                if (str->len > 0 && '\r' == str->ptr[str->len - 1]) {
                    str->ptr[--str->len] = 0;
                }
                break;
        }
    }
}

enum {
    TRIM_LEFT  = 1,
    TRIM_RIGHT = 2,
    TRIM_BOTH  = 3
};

static size_t _trim(
    char *string, size_t string_length,
    const char *what, int32_t what_length,
    int mode
) {
    size_t i;
    size_t start = 0, end;

    if (NULL != what) {
        if ('\0' == *what) {
            what = NULL;
            what_length = 0;
        } else if (what_length < 0) {
            what_length = strlen(what);
        }
    }
    end = string_length;
    if (HAS_FLAG(mode, TRIM_LEFT)) {
        for (i = 0; i < end; i++) {
            if (NULL != what) {
                if (NULL == memchr(what, string[i], what_length)) {
                    break;
                }
            } else {
                if (!isspace(string[i])) {
                    break;
                }
            }
        }
        start = i;
    }
    if (HAS_FLAG(mode, TRIM_RIGHT)) {
        for (i = end; i > start; i--) {
            if (NULL != what) {
                if (NULL == memchr(what, string[i], what_length)) {
                    break;
                }
            } else {
                if (!isspace(string[i])) {
                    break;
                }
            }
        }
        end = i;
    }
    if (start < string_length) {
        memmove(string, string + start, end - start);
        *(string + end - start) = 0;
    } else {
        *string = 0;
    }

    return end - start;
}

static size_t trim(char *s, size_t s_length, const char *what, size_t what_length)
{
    return _trim(s, s_length, what, what_length, TRIM_BOTH);
}

static size_t ltrim(char *s, size_t s_length, const char *what, size_t what_length)
{
    return _trim(s, s_length, what, what_length, TRIM_LEFT);
}

static size_t rtrim(char *s, size_t s_length, const char *what, size_t what_length)
{
    return _trim(s, s_length, what, what_length, TRIM_RIGHT);
}

void string_trim(String *str)
{
    assert(NULL != str);

    str->len = trim(str->ptr, str->len, NULL, -1);
}

void string_ltrim(String *str)
{
    assert(NULL != str);

    str->len = ltrim(str->ptr, str->len, NULL, -1);
}

void string_rtrim(String *str)
{
    assert(NULL != str);

    str->len = rtrim(str->ptr, str->len, NULL, -1);
}

/* ==================== misc/others ==================== */

void string_sync(const String *ref, String *buffer, double ratio)
{
    assert(NULL != ref);
    assert(buffer != ref);
    assert(0 != ratio);

    if (buffer->allocated <= ref->allocated * ratio) {
        buffer->allocated = ref->allocated * ratio;
        buffer->ptr = mem_renew(buffer->ptr, *buffer->ptr, buffer->allocated + 1);
    }
}

int string_empty(const String *str)
{
    assert(NULL != str);

    return (str->len < 1);
}

void string_truncate(String *str)
{
    assert(NULL != str);

    *str->ptr = '\0';
    str->len = 0;
}

void string_dump(String *str)
{
    char *p;
    size_t i, len;
    const int replacement_len = STR_SIZE("0x00");
    const char replacement[] = "0x%02X";

    assert(NULL != str);

    len = 0;
    for (i = 0; i < str->len; i++) {
        switch (str->ptr[i]) {
            case '\t':
            case '\r':
                len++;
                break;
            default:
                if (!isprint(str->ptr[i])) {
                    len += replacement_len - STR_LEN("X");
                }
        }
    }
    if (len > 0) {
        _string_maybe_expand_of(str, len);
        str->len += len;
        str->ptr[str->len] = 0;
        p = str->ptr + str->len;
        for (/* NOP */; i > 0; i--) {
            switch (str->ptr[i]) {
                case '\t':
                    *--p = 't';
                    *--p = '\\';
                    break;
                case '\r':
                    *--p = 'r';
                    *--p = '\\';
                    break;
                default:
                {
                    if (!isprint(str->ptr[i])) {
                        p -= replacement_len;
                        snprintf(p, replacement_len, replacement, str->ptr[i]);
                    }
                }
            }
        }
    }
}

static const char replacement[] = "0x%02hhx";
static const int replacement_len = STR_LEN("0x00");
// static const int replacement_size = STR_SIZE("0x00");

void string_append_string_len_dump(String *str, const char *src, size_t src_len)
{
    char *q;
    size_t len;
    const char *p;
    const char * const end = src + src_len;

    assert(NULL != str);
    assert(NULL != src);

    len = 0;
    for (p = src; p < end; p++) {
        switch (*p) {
            case '\n':
            case '\t':
            case '\r':
                len++;
                break;
            default:
                if (!isprint(*p)) {
                    len += replacement_len - sizeof(*p);
                }
        }
    }
    _string_maybe_expand_of(str, src_len + len);
    if (len > 0) {
        q = str->ptr + str->len;
        for (p = src; p < end; p++) {
            switch (*p) {
                case '\n':
                    *q++ = '\\';
                    *q++ = 'n';
                    break;
                case '\t':
                    *q++ = '\\';
                    *q++ = 't';
                    break;
                case '\r':
                    *q++ = '\\';
                    *q++ = 'r';
                    break;
                default:
                {
                    if (!isprint(*p)) { // TODO: UTF-8 compliant
                        sprintf(q, replacement, *p);
//                         snprintf(q, replacement_size, replacement, *p);
                        q += replacement_len;
                    } else {
                        *q++ = *p;
                    }
                }
            }
        }
    } else {
        memcpy(str->ptr + str->len, src, src_len);
    }
    str->len += src_len + len;
    str->ptr[str->len] = 0;
}

void string_formatted(String *str, const char *format, ...) /* PRINTF(2, 3) */
{
    int ret;
    va_list args;

    assert(NULL != str);
    assert(NULL != format);

    va_start(args, format);
    if ((ret = vsnprintf(str->ptr, str->allocated, format, args)) > (int) str->allocated) {
        _string_maybe_expand_of(str, ret);
        va_end(args);
        va_start(args, format);
        ret = vsnprintf(str->ptr, str->allocated, format, args);
        assert(ret > 0 && ((size_t) ret) <= str->allocated);
        va_end(args);
    } else {
        va_end(args);
    }
    str->len = ret;
}

void string_append_formatted(String *str, const char *format, ...) /* PRINTF(2, 3) */
{
    int ret;
    va_list args;

    assert(NULL != str);
    assert(NULL != format);

    va_start(args, format);
    if ((ret = vsnprintf(str->ptr + str->len, str->allocated - str->len, format, args)) >= (int) (str->allocated - str->len)) {
        _string_maybe_expand_of(str, ret);
        va_end(args);
        va_start(args, format);
        ret = vsnprintf(str->ptr + str->len, str->allocated - str->len, format, args);
        assert(ret <= (int) (str->allocated - str->len));
        va_end(args);
    } else {
        va_end(args);
    }
    str->len += ret;
}

void string_append_n_times(String *str, const char *suffix, size_t suffix_len, size_t times)
{
    size_t i;

    assert(NULL != str);
    assert(NULL != suffix);

    _string_maybe_expand_of(str, suffix_len * times);
    for (i = 0; i < times; i++) {
        memcpy(str->ptr + str->len, suffix, suffix_len);
        str->len += suffix_len;
    }
    str->ptr[str->len] = '\0';
}

void string_append_xml_len(String *str, const char *suffix, size_t suffix_len)
{
    const char *p;
    size_t required_len;
    const char * const end = suffix + suffix_len;

    required_len = 0;
    for (p = suffix; p < end; p++) {
        switch (*p) {
            case '&':
                required_len += STR_LEN("&amp;");
                break;
            case '<':
            case '>':
                required_len += STR_LEN("&Xt;");
                break;
            default:
                ++required_len;
        }
    }
    _string_maybe_expand_of(str, required_len);
    for (p = suffix; p < end; p++) {
        switch (*p) {
            case '&':
                memcpy(str->ptr + str->len, "&amp;", STR_LEN("&amp;"));
                str->len += STR_LEN("&amp;");
                break;
            case '<':
                memcpy(str->ptr + str->len, "&lt;", STR_LEN("&lt;"));
                str->len += STR_LEN("&lt;");
                break;
            case '>':
                memcpy(str->ptr + str->len, "&gt;", STR_LEN("&gt;"));
                str->len += STR_LEN("&gt;");
                break;
            default:
                str->ptr[str->len++] = *p;
        }
    }
    str->ptr[str->len] = '\0';
}

#define ESC_DECL(s) \
    { STR_LEN(s), s }

static struct json_escape_t {
    size_t sequence_len;
    const char * const sequence;
} json_escape_table[] = {
    /* 00 - 07 */ ESC_DECL("\\u0000"), ESC_DECL("\\u0001"), ESC_DECL("\\u0002"), ESC_DECL("\\u0003"), ESC_DECL("\\u0004"), ESC_DECL("\\u0005"), ESC_DECL("\\u0006"), ESC_DECL("\\u0007"),
    /* 08 - 0F */ ESC_DECL("\\b"),     ESC_DECL("\\t"),     ESC_DECL("\\n"),     ESC_DECL("\\u000B"), ESC_DECL("\\f"),     ESC_DECL("\\r"),     ESC_DECL("\\u000E"), ESC_DECL("\\u000F"),
    /* 10 - 17 */ ESC_DECL("\\u0010"), ESC_DECL("\\u0011"), ESC_DECL("\\u0012"), ESC_DECL("\\u0013"), ESC_DECL("\\u0014"), ESC_DECL("\\u0015"), ESC_DECL("\\u0016"), ESC_DECL("\\u0017"),
    /* 18 - 1F */ ESC_DECL("\\u0018"), ESC_DECL("\\u0019"), ESC_DECL("\\u001A"), ESC_DECL("\\u001B"), ESC_DECL("\\u001C"), ESC_DECL("\\u001D"), ESC_DECL("\\u001E"), ESC_DECL("\\u001F"),
    /* 10 - 27 */ ESC_DECL(" "),       ESC_DECL("!"),       ESC_DECL("\\\""),    ESC_DECL("#"),       ESC_DECL("$"),       ESC_DECL("%"),       ESC_DECL("&"),       ESC_DECL("'"),
    /* 18 - 2F */ ESC_DECL("("),       ESC_DECL(")"),       ESC_DECL("*"),       ESC_DECL("+"),       ESC_DECL(","),       ESC_DECL("-"),       ESC_DECL("."),       ESC_DECL("\\/")
};

void string_append_json_string(String *str, const char *string)
{
    const char *p;
    size_t required_len;

    required_len = 2; /* for " */
    for (p = string; '\0' != *p; p++) {
        if (((unsigned char) *p) < ARRAY_SIZE(json_escape_table)) {
            required_len += json_escape_table[(unsigned char) *p].sequence_len;
        } else {
            ++required_len;
        }
    }
    _string_maybe_expand_of(str, required_len);
    str->ptr[str->len++] = '"';
    for (p = string; '\0' != *p; p++) {
        if (((unsigned char) *p) < ARRAY_SIZE(json_escape_table)) {
            struct json_escape_t e = json_escape_table[(unsigned char) *p];

            memcpy(str->ptr + str->len, e.sequence, e.sequence_len);
            str->len += e.sequence_len;
        } else {
            str->ptr[str->len++] = *p;
        }
    }
    str->ptr[str->len++] = '"';
    str->ptr[str->len] = '\0';
}
