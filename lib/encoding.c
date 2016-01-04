
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
static const char *guess_encoding(const char *string, size_t string_len, size_t *signature_len)
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
            *signature_len = signatures[i].signature_len;
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
static const char *guess_encoding(const char *string, size_t string_len, size_t *signature_len)
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
