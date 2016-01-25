#include <unistd.h>
#include <locale.h>
#include <stdlib.h>

#include "config.h"
#include "cpp.h"
#include "shall.h"

#ifdef WITH_ICU

# include <unicode/ucnv.h>
# include <unicode/ucsdet.h>

# define MIN_CONFIDENCE 39

/**
 * Attempts to guess encoding of a string
 *
 * First, we look for a signature (also called BOM). If there is
 * none, we try a charset detection. Note that the second method
 * may return a wrong result!
 *
 * @param string the buffer to analyse
 * @param string_len its length
 * @param signature_len the length of the BOM if any was found (caller should initialize it to 0)
 *
 * @return NULL or the name of the encoding
 */
SHALL_API const char *encoding_guess(const char *string, size_t string_len, size_t *signature_len)
{
    int32_t length;
    UErrorCode status;
    const char *encoding;

    length = 0;
    status = U_ZERO_ERROR;
    encoding = ucnv_detectUnicodeSignature(string, string_len, &length, &status);
    if (U_SUCCESS(status)) {
        if (NULL != signature_len) {
            *signature_len = (size_t) length;
        }
        if (NULL == encoding) {
            int32_t confidence;
            UCharsetDetector *csd;
            const char *tmpencoding;
            const UCharsetMatch *ucm;

            csd = ucsdet_open(&status);
            if (U_FAILURE(status)) {
                goto end;
            }
            ucsdet_setText(csd, string, string_len, &status);
            if (U_FAILURE(status)) {
                goto end;
            }
            ucm = ucsdet_detect(csd, &status);
            if (U_FAILURE(status)) {
                goto end;
            }
            confidence = ucsdet_getConfidence(ucm, &status);
            tmpencoding = ucsdet_getName(ucm, &status);
            if (U_FAILURE(status)) {
                ucsdet_close(csd);
                goto end;
            }
            if (confidence > MIN_CONFIDENCE) {
                encoding = tmpencoding;
            }
            ucsdet_close(csd);
        }
    }

end:
    return encoding;
}

/**
 * Convert a string from one charset to another
 * @param from the input encoding's name (src's charset)
 * @param to the output encoding's name (dst's charset)
 * @param in the string to convert
 * @param in_len its length
 * @param out the result string
 * @param out_len_arg_p its length (use NULL if you don't want this information)
 *
 * @return false on failure else true
 */
static bool encoding_convert(const char *from, const char *to, const char *in, size_t in_len, char **out, size_t *out_len_arg_p)
{
    UErrorCode status;
    int32_t out_size, out_len;

    *out = NULL;
    out_len = 0;
    status = U_ZERO_ERROR;
    out_size = ucnv_convert(to, from, NULL, 0, in, in_len, &status);
    if (U_BUFFER_OVERFLOW_ERROR == status) {
        status = U_ZERO_ERROR;
        *out = mem_new_n(**out, out_size + 1);
        out_len = ucnv_convert(to, from, *out, out_size + 1, in, in_len, &status);
        if (U_ZERO_ERROR != status) {
            out_len = 0;
            free(*out);
            *out = NULL;
        }
    }
    if (NULL != out_len_arg_p) {
        *out_len_arg_p = (size_t) out_len;
    }

    return NULL != *out;
}

#else

# ifdef WITH_ICONV

#  include <errno.h>
#  include <iconv.h>
#  include <string.h>

#  define INVALID_SIZE_T ((size_t) -1)
#  define INVALID_ICONV_T ((iconv_t) -1)

#  define ICONV_SET_ERROR(msg, ...) \
    fprintf(stderr, msg, ## __VA_ARGS__)

#  define ICONV_ERR_CONVERTER(error) \
    ICONV_SET_ERROR("Cannot open converter")
#  define ICONV_ERR_WRONG_CHARSET(error, to, from) \
    ICONV_SET_ERROR("Wrong charset, conversion from '%s' to '%s' is not allowed", to, from)
#  define ICONV_ERR_ILLEGAL_CHAR(error) \
    ICONV_SET_ERROR("Incomplete multibyte character detected in input string")
#  define ICONV_ERR_ILLEGAL_SEQ(error) \
    ICONV_SET_ERROR("Illegal character detected in input string")
#  define ICONV_ERR_TOO_BIG(error) \
    ICONV_SET_ERROR("Buffer length exceeded")
#  define ICONV_ERR_UNKNOWN(error, errno) \
    ICONV_SET_ERROR("Unknown error (%d)", errno)

/**
 * Convert a string from one charset to another
 * @param from the input encoding's name (src's charset)
 * @param to the output encoding's name (dst's charset)
 * @param in the string to convert
 * @param in_len its length
 * @param out the result string
 * @param out_len_arg_p its length (use NULL if you don't want this information)
 *
 * @return false on failure else true
 */
static bool encoding_convert(const char *from, const char *to, const char *in, size_t in_len, char **out, size_t *out_len_arg_p)
{
    iconv_t cd;
    bool retval;
    char *in_p, *out_p;
    size_t in_left, out_left, out_size, result, out_len, *out_len_p;

    *out = NULL;
    if (NULL == out_len_arg_p) {
        out_len_p = &out_len;
    } else {
        out_len_p = out_len_arg_p;
    }
    *out_len_p = 0;
    retval = true;
    result = INVALID_SIZE_T;
#  if 0
    if ('\0' == *in) {
        *out_len_p = 0;
        *out = mem_new_n(**out, *out_len_p + 1);
        **out = '\0';
        return retval;
    }
#  else
    if (0 == strcmp(from, to) || '\0' == *in) {
        *out = (char *) in;
        *out_len_p = in_len;
        return retval;
    }
#  endif

    errno = 0;
    if (INVALID_ICONV_T == (cd = iconv_open(to, from))) {
        if (EINVAL == errno) {
            ICONV_ERR_WRONG_CHARSET(error, to, from);
            return false;
        } else {
            ICONV_ERR_CONVERTER(error);
            return false;
        }
    }
# ifdef HAVE_ICONVCTL
    {
        int one;

        one = 1;
        iconvctl(cd, ICONV_SET_ILSEQ_INVALID, &one);
    }
# endif /* HAVE_ICONVCTL */
    out_size = 1; /* out_size: capacity allocated to *out */
    *out = mem_new_n(**out, out_size + 1); /* + 1 for \0 */
    in_p = (char *) in;
    out_p = *out;
    in_left = in_len;
    out_left = out_size; /* out_left: free space left in *out */
    while (in_left > 0) {
// printf("iconv(%d, %d)\n", out_size, out_left);
        result = iconv(cd, (ICONV_CONST char **) &in_p, &in_left, &out_p, &out_left);
        *out_len_p = out_p - *out;
        out_left = out_size - *out_len_p;
        if (INVALID_SIZE_T == result) {
            if (E2BIG == errno && in_left > 0) {
                char *tmp;

                tmp = mem_renew(*out, **out, ++out_size + 1); /* *out is no longer valid */
                *out = tmp;
                out_p = *out + *out_len_p;
                out_left = out_size - *out_len_p;
                continue;
            }
        }
        break;
    }
    if (INVALID_SIZE_T != result) {
        while (1) {
            result = iconv(cd, NULL, NULL, &out_p, &out_left);
            *out_len_p = out_p - *out;
            out_left = out_size - *out_len_p;
            if (INVALID_SIZE_T != result) {
                break;
            }
            if (E2BIG == errno) {
                char *tmp;

                tmp = mem_renew(*out, **out, ++out_size + 1); /* *out is no longer valid */
                *out = tmp;
                out_p = *out + *out_len_p;
                out_left = out_size - *out_len_p;
            } else {
                break;
            }
        }
    }
    iconv_close(cd);
    if (INVALID_SIZE_T == result) {
        retval = false;
        free(*out);
        *out = NULL;
        switch (errno) {
            case EINVAL:
                ICONV_ERR_ILLEGAL_CHAR(error);
                break;
            case EILSEQ:
                ICONV_ERR_ILLEGAL_SEQ(error);
                break;
            case E2BIG:
                ICONV_ERR_TOO_BIG(error);
                break;
            default:
                ICONV_ERR_UNKNOWN(error, errno);
        }
    } else {
        *out_p = '\0';
        *out_len_p = out_p - *out;
    }

    return retval;
}

# endif /* WITH_ICONV */

# define S(s) s, STR_LEN(s)

# include <string.h>

static struct {
    const char *encoding;
    const char *signature;
    size_t signature_len;
} signatures[] = {
    { "UTF-8",    S("\xEF\xBB\xBF") },
    { "UTF-32LE", S("\xFF\xFE\x00\x00") },
    { "UTF-32BE", S("\x00\x00\xFE\xFF") },
    { "UTF-16LE", S("\xFF\xFE") },
    { "UTF-16BE", S("\xFE\xFF") },
#if 0
    { "SCSU",       S("\x0E\xFE\xFF") },
    { "BOCU-1",     S("\xFB\xEE\x28") },
    { "UTF-7",      S("\x2B\x2F\x76\x38") },
    { "UTF-7",      S("\x2B\x2F\x76\x39") },
    { "UTF-7",      S("\x2B\x2F\x76\x2B") },
    { "UTF-7",      S("\x2B\x2F\x76\x2F") },
    { "UTF-EBCDIC", S("\xDD\x73\x66\x73") },
#endif
};

/**
 * Attempts to find a signature in front of the buffer
 *
 * @param string the buffer to analyse
 * @param string_len its length
 * @param signature_len the length of the BOM if any was found (caller should initialize it to 0)
 *
 * @return NULL or the name of the encoding
 */
static const char *detect_signature(const char *string, size_t string_len, size_t *signature_len)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(signatures); i++) {
        if (string_len >= signatures[i].signature_len && 0 == memcmp(string, signatures[i].signature, signatures[i].signature_len)) {
            if (NULL != signature_len) {
                *signature_len = signatures[i].signature_len;
            }
            return signatures[i].encoding;
        }
    }

    return NULL;
}

/**
 * Attempts to guess encoding of a string
 *
 * First, we look for a signature (also called BOM). If there is
 * none, we try a limited charset detection. Note that the second
 * method may return a wrong result!
 *
 * @note detection is limited to Unicode encodings (UTF-(?:8|((?:16|32)(?:[LB]E))))
 *
 * @param string the buffer to analyse
 * @param string_len its length
 * @param signature_len the length of the BOM if any was found (caller should initialize it to 0)
 *
 * @return NULL or the name of the encoding
 */
SHALL_API const char *encoding_guess(const char *string, size_t string_len, size_t *signature_len)
{
    const char *encoding;

    if (NULL == (encoding = detect_signature(string, string_len, signature_len))) {
        if (string_len >= 4) {
            if (0x00 == string[1]) {
                if (0x00 == string[2]) {
                    if (0x00 == string[0] && 0x00 != string[3]) {
                        return "UTF-32BE";
                    } else if (0x00 != string[0] && 0x00 == string[3]) {
                        return "UTF-32LE";
                    } else {
                        return NULL;
                    }
                } else if (/*0x00 != string[2] && */0x00 != string[0] && 0x00 == string[3]) {
                    return "UTF-16LE";
                }
            } else if (/*0x00 != string[1] && */0x00 == string[0] && 0x00 == string[2] && 0x00 != string[3]) {
                return "UTF-16BE";
            }
        }
    }

    return encoding;
}

#endif /* WITH_ICU */

#undef S
#define S(state) STATE_##state

enum {
    S(__),       // error/invalid, have to be 0
    S(OK),       // accept
    S(FB),       // last byte (always in range [0x80;0xBF) for a code point encoded on more than a single byte
    S(32),       // normal case for the 2nd byte of a 3 bytes code point ([0x80;0xBF])
    S(32E0),     // still 3 bytes encoded code point but when 2nd byte is 0xE0, its range is restricted to [0xA0;0xBF]
    S(32ED),     // still 3 bytes encoded code point but when 2nd byte is 0xED, its range is restricted to [0x80;0x9F]
    S(42),       // normal case for the 2nd byte of a 4 bytes code point ([0x80;0xBF])
    S(42F0),     // still 4 bytes encoded code point but when 2nd byte is 0xF0, its range is restricted to [0x90;0xBF]
    S(42F4),     // still 4 bytes encoded code point but when 2nd byte is 0xF4, its range is restricted to [0x80;0x8F]
    S(43),       // 3rd byte of a 4 bytes encoded code point
    _STATE_COUNT // number of states
};

static const uint8_t state_transition_table[_STATE_COUNT][256] = {
    // handle first byte
    [ S(OK) ]   = { [ 0 ... 0x7F ] = S(OK), [ 0xC2 ... 0xDF ] = S(FB), [ 0xE0 ] = S(32E0), [ 0xE1 ... 0xEC ] = S(32), [ 0xED ] = S(32ED), [ 0xEE ... 0xEF ] = S(32), [ 0xF0 ] = S(42F0), [ 0xF1 ... 0xF3 ] = S(42), [ 0xF4 ] = S(42F4) },
    // final regular byte
    [ S(FB) ]   = { [ 0x80 ... 0xBF ] = S(OK) },
    // 3 bytes encoded code point
    [ S(32) ]   = { [ 0x80 ... 0xBF ] = S(FB) }, // 2nd byte, normal case
    [ S(32E0) ] = { [ 0xA0 ... 0xBF ] = S(FB) }, // 2nd byte, special case for 0xE0
    [ S(32ED) ] = { [ 0x80 ... 0x9F ] = S(FB) }, // 2nd byte, special case for 0xED
    // 4 bytes encoded code point
    [ S(42) ]   = { [ 0x80 ... 0xBF ] = S(43) }, // 2nd byte, normal case
    [ S(42F0) ] = { [ 0x90 ... 0xBF ] = S(43) }, // 2nd byte, special case for 0xF0
    [ S(42F4) ] = { [ 0x80 ... 0x8F ] = S(43) }, // 2nd byte, special case for 0xF4
    [ S(43) ]   = { [ 0x80 ... 0xBF ] = S(FB) }, // 3rd byte
};

/**
 * Check if a string is a valid UTF-8 string
 *
 * @param string the string to check
 * @param string_len its length
 * @param errp, optionnal (NULL to ignore), to have a pointer on the first
 * invalid byte found in the string (*errp is set to NULL if none)
 *
 * @return true if the string is a valid
 */
SHALL_API bool encoding_utf8_check(const char *string, size_t string_len, const char **errp)
{
    int state;
    const uint8_t *s;
    const uint8_t * const end = (const uint8_t *) string + string_len;

    state = S(OK); // accept empty string
    for (s = (const uint8_t *) string; S(__) != state && s < end; s++) {
//         int prev = state;
        state = state_transition_table[state][*s];
//         printf("%ld 0x%02X %d => %d\n", s - (const uint8_t *) string, *s, prev, state);
    }
//     if (S(OK) != state) {
//         printf("%d at %ld (0x%02X)\n", state, s - (const uint8_t *) string, *s);
//     }
    if (NULL != errp) {
        if (S(OK) == state) {
            *errp = NULL;
        } else {
            *errp = (const char *) s;
        }
    }
#if 0
    if (S(OK) != state) {
        if (S(__) != state && s == end) {
            return TRUNCATED;
        } else {
            return INVALID;
        }
    } else {
        return OK;
    }
#endif
    return S(OK) == state;
}

/**
 * Extract encoding from locale or, for BSD systems,
 * from login classes.
 *
 * @return NULL if no suitable charset was found else
 * its name
 */
static const char *encoding_extract_from_locale(void)
{
    const char *encoding;

    encoding = NULL;
#ifdef BSD
    {
# include <sys/types.h>
# include <pwd.h>
# include <login_cap.h>

        login_cap_t *lc;
        const char *tmp;
        struct passwd *pwd;

        if (NULL != (pwd = getpwuid(getuid()))) {
            if (NULL != (lc = login_getuserclass(pwd))) {
                if (NULL != (tmp = login_getcapstr(lc, "charset", NULL, NULL))) {
                    encoding = tmp;
# ifdef WITH_ICU
                    /**
                     * ICU doesn't (didn't?) consider login classes on BSD systems
                     * So do it for ICU and overwrite default converter if needed
                     */
                    ucnv_setDefaultName(tmp);
# endif /* WITH_ICU */
                }
                login_close(lc);
            } else {
                if (NULL != (lc = login_getpwclass(pwd))) {
                    if (NULL != (tmp = login_getcapstr(lc, "charset", NULL, NULL))) {
                        encoding = tmp;
# ifdef WITH_ICU
                        // same here
                        ucnv_setDefaultName(tmp);
# endif /* WITH_ICU */
                    }
                    login_close(lc);
                }
            }
        }
        if (NULL != (tmp = getenv("MM_CHARSET"))) {
            encoding = tmp;
        }
    }
#endif /* BSD */
    if (NULL == encoding) {
#include <langinfo.h>

        setlocale(LC_ALL, "");
        // TODO: linux specific?
        encoding = nl_langinfo(CODESET);
    }

    return encoding;
}

/**
 * Get current encoding for stdin
 *
 * @return NULL if no real charset can be safely associated to stdin
 * (have in mind a shell redirection or pipe) else the name of
 * charset's name of current locale.
 */
SHALL_API const char *encoding_stdin_get(void)
{
    const char *input_encoding;

    input_encoding = NULL;
    if (isatty(STDIN_FILENO)) {
        input_encoding = encoding_extract_from_locale();
    }

    return input_encoding;
}

/**
 * Get current encoding for stdout
 *
 * @return "UTF-8" if stdout is piped/redirected else charset's name
 * of current locale.
 */
SHALL_API const char *encoding_stdout_get(void)
{
    const char *output_encoding;

    output_encoding = "UTF-8";
    if (isatty(STDOUT_FILENO)) {
        const char *tmp;

        if (NULL != (tmp = encoding_extract_from_locale())) {
            output_encoding = tmp;
        }
    }

    return output_encoding;
}

/**
 * Convert a string from UTF-8 to an other encoding
 *
 * @param input_encoding the name of src's charset
 * @param src the string to convert to UTF-8
 * @param src_len its length
 * @param dst a pointer to the malloc'ed output string
 * @param dst_len (optionnal: may be NULL) its length
 *
 * @return true if successfull
 */
SHALL_API bool encoding_convert_to_utf8(const char *input_encoding, const char *src, size_t src_len, char **dst, size_t *dst_len)
{
#if defined(WITH_ICU) || defined(WITH_ICONV)
    return encoding_convert(input_encoding, "UTF-8", src, src_len, dst, dst_len);
#else
    return false;
#endif /* WITH_ICU || WITH_ICONV */
}

/**
 * Convert an UTF-8 encoded string to an other encoding
 *
 * @param output_encoding the name of dst's charset
 * @param src the string to convert from UTF-8
 * @param src_len its length
 * @param dst a pointer to the malloc'ed output string
 * @param dst_len (optionnal: may be NULL) its length
 *
 * @return true if successfull
 */
SHALL_API bool encoding_convert_from_utf8(const char *output_encoding, const char *src, size_t src_len, char **dst, size_t *dst_len)
{
#if defined(WITH_ICU) || defined(WITH_ICONV)
    return encoding_convert("UTF-8", output_encoding, src, src_len, dst, dst_len);
#else
    return false;
#endif /* WITH_ICU || WITH_ICONV */
}
