#include "cpp.h"
#include "shall.h"

#if defined(WITH_ICU)

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
    UErrorCode status;
    const char *encoding;

    status = U_ZERO_ERROR;
    encoding = ucnv_detectUnicodeSignature(string, string_len, signature_len, &status);
    if (U_SUCCESS(status)) {
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

#else

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
