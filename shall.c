/**
 * @file
 */

#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <limits.h>

#include "cpp.h"
#include "utils.h"
#include "tokens.h"
// #include "option.h"
#include "lexer.h"
#include "lexer-private.h"
#include "formatter.h"
#include "shall.h"

#ifndef DOXYGEN
extern const LexerImplementation apache_lexer;
extern const LexerImplementation c_lexer;
extern const LexerImplementation cmake_lexer;
extern const LexerImplementation css_lexer;
extern const LexerImplementation dtd_lexer;
extern const LexerImplementation diff_lexer;
extern const LexerImplementation json_lexer;
extern const LexerImplementation nginx_lexer;
extern const LexerImplementation php_lexer;
extern const LexerImplementation erb_lexer;
extern const LexerImplementation ruby_lexer;
extern const LexerImplementation text_lexer;
extern const LexerImplementation varnish_lexer;
extern const LexerImplementation xml_lexer;
extern const LexerImplementation postgresql_lexer;
#endif /* !DOXYGEN */

// only final/public lexers
static const LexerImplementation *available_lexers[] = {
    &php_lexer,
    &erb_lexer,
    &ruby_lexer,
    &xml_lexer,
    &dtd_lexer,
    &c_lexer,
    &cmake_lexer,
    &css_lexer,
    &diff_lexer,
    &json_lexer,
    &nginx_lexer,
    &apache_lexer,
    &varnish_lexer,
    &postgresql_lexer,
    &text_lexer // may be a good idea to keep it last
};

/**
 * Exposes the number of available builtin lexers
 *
 * @note for external use only, keep using ARRAY_SIZE internally
 */
SHALL_API const size_t SHALL_LEXER_COUNT = ARRAY_SIZE(available_lexers);

#ifndef DOXYGEN
// extern const FormatterImplementation _xmlfmt;
extern const FormatterImplementation _htmlfmt;
extern const FormatterImplementation _termfmt;
extern const FormatterImplementation _plainfmt;
#endif /* !DOXYGEN */

static const FormatterImplementation *available_formatters[] = {
//     &_xmlfmt,
    &_htmlfmt,
    &_termfmt,
    &_plainfmt
};

/**
 * Exposes the number of available builtin formatters
 *
 * @note for external use only, keep using ARRAY_SIZE internally
 */
SHALL_API const size_t SHALL_FORMATTER_COUNT = ARRAY_SIZE(available_formatters);

/* ========== helpers ========== */

/**
 * Gets (unwraps if necessary) a lexer from a raw OptionValue
 *
 * @param optval the base option value
 *
 * @return the lexer
 */
SHALL_API Lexer *lexer_unwrap(OptionValue optval)
{
    return LEXER_UNWRAP(optval);
}

#ifndef DOXYGEN
# define OPT_STRVAL_FREE(opt, def) \
    do { \
        if (NULL != OPT_STRVAL(opt) && OPT_STRVAL(opt) != OPT_STRVAL(def)) { \
            free((char *) OPT_STRVAL(opt)); \
        } \
    } while (0);
#endif /* !DOXYGEN */

/**
 * Copies an OptionValue from one to another.
 * The real goal of this function is to:
 * - free memory of the old value if different of the default value
 * - make a copy of the given value (src)
 * For now, it only concerns strings.
 *
 * @param type the type of the option
 * @param dest the option value where to copy to
 * @param src the option value where to copy from
 * @param defval the default value
 */
static inline void opt_copy(OptionType type, OptionValue *dest, OptionValue src, OptionValue defval)
{
    switch (type) {
        case OPT_TYPE_BOOL:
        case OPT_TYPE_INT:
        case OPT_TYPE_LEXER:
            memcpy(dest, &src, sizeof(*dest));
            break;
        case OPT_TYPE_STRING:
            OPT_STRVAL_FREE(*dest, defval);
            OPT_STRVAL(*dest) = strdup(OPT_STRVAL(src));
            OPT_STRLEN(*dest) = OPT_STRLEN(src);
            break;
        default:
            assert(0);
            break;
    }
}

#ifndef DOXYGEN
# define DIRECTORY_SEPARATOR '/'
#endif /* !DOXYGEN */

/**
 * Gets trailing name component of path
 *
 * @param path the path
 * @param bname the buffer to receive the "calculated" filename
 * @param bname_size the size of bname, have to be >= 2
 *
 * @return bname or NULL if the bname size is insuffisant
 */
static char *shall_basename(const char *path, char *bname, size_t bname_size)
{
    size_t len;
    const char *endp, *startp;

    if (bname_size < 2) {
        return NULL;
    }
    /* Empty or NULL string gets treated as "." */
    if (NULL == path || '\0' == *path) {
        bname[0] = '.';
        bname[1] = '\0';
        return bname;
    }
    /* Strip any trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && DIRECTORY_SEPARATOR == *endp) {
        endp--;
    }
    /* All slashes becomes "/" */
    if (endp == path && DIRECTORY_SEPARATOR == *endp) {
        bname[0] = DIRECTORY_SEPARATOR;
        bname[1] = '\0';
        return bname;
    }
    /* Find the start of the base */
    startp = endp;
    while (startp > path && DIRECTORY_SEPARATOR != *(startp - 1)) {
        startp--;
    }
    len = endp - startp + 1;
    if (len >= bname_size) {
//         errno = ENAMETOOLONG;
        return NULL;
    }
    memcpy(bname, startp, len);
    bname[len] = '\0';

    return bname;
}

#ifndef DOXYGEN
# define UTF8_BOM "\xEF\xBB\xBF"
#endif /* !DOXYGEN */

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

#endif

#ifndef DOXYGEN
# define STRCASECMP(s, ss) \
    strncasecmp(s, ss, STR_LEN(ss))
#endif /* !DOXYGEN */

/**
 * Helper for lexer_set_option_as_string and formatter_set_option_as_string as they share the same code
 *
 * @param optvalptr the internal pointer into lexer or formatter to the option value
 * @param type the type of the option (one of the OPT_TYPE_* constant)
 * @param value the new value to set
 * @param value_len its length
 * @param reject_lexer_type true for formatters to invalidate a Lexer as value
 *
 * @return a int as follows:
 *  + OPT_SUCCESS if no error
 *  + OPT_ERR_TYPE_MISMATCH if type's value does not match expected one
 *  + OPT_ERR_INVALID_OPTION if this is not a valid option name
 *  + OPT_ERR_UNKNOWN_LEXER if option expect a lexer but value is not a valid lexer implementation name
 *  + OPT_ERR_INVALID_VALUE if the value is not acceptable
 */
static int parse_option_as_string(OptionValue *optvalptr, int type, const char *value, size_t value_len, int reject_lexer_type)
{
    switch (type) {
        case OPT_TYPE_BOOL:
        {
            long bval;

            bval = 0;
            if ('\0' == *value) { // explicitely accept "" as true, see this case as "defined is true"
                bval = 1;
            } else if (0 == STRCASECMP(value, "on") || 0 == STRCASECMP(value, "true")) {
                bval = 1;
            } else if (0 == STRCASECMP(value, "off") || 0 == STRCASECMP(value, "false")) {
                // NOP (already to 0)
            } else {
                bval = strtol(value, NULL, 10);
            }
            OPT_SET_BOOL(*optvalptr, bval);
            return OPT_SUCCESS;
        }
        case OPT_TYPE_INT:
        {
            long lval;

            lval = strtol(value, NULL, 10);
            OPT_SET_INT(*optvalptr, lval);
            return OPT_SUCCESS;
        }
        case OPT_TYPE_STRING:
        {
            OPT_STRVAL(*optvalptr) = strdup(value);
            OPT_STRLEN(*optvalptr) = value_len;
            return OPT_SUCCESS;
        }
        case OPT_TYPE_LEXER:
            if (reject_lexer_type) {
                return OPT_ERR_INVALID_VALUE;
            } else {
                const LexerImplementation *limp;

                if (NULL == (limp = lexer_implementation_by_name(value))) {
                    return OPT_ERR_UNKNOWN_LEXER;
                } else {
                    OPT_LEXPTR(*optvalptr) = (void *) lexer_create(limp);
                    OPT_LEXUWF(*optvalptr) = NULL;
                    return OPT_SUCCESS;
                }
            }
        default:
            return OPT_ERR_TYPE_MISMATCH;
    }
}

/* ========== LexerImplementation functions ========== */

/**
 * Gets a lexer implementation from its name.
 * (lookup is done on the official name and the aliases)
 *
 * @param name the implementation name
 *
 * @return NULL or the LexerImplementation which matches the given name
 */
SHALL_API const LexerImplementation *lexer_implementation_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
        if (0 == ascii_strcasecmp(name, available_lexers[i]->name)) {
            return available_lexers[i];
        }
        if (NULL != available_lexers[i]->aliases) {
            const char * const *alias;

            for (alias = available_lexers[i]->aliases; NULL != *alias; alias++) {
                if (0 == ascii_strcasecmp(name, *alias)) {
                    return available_lexers[i];
                }
            }
        }
    }

    return NULL;
}

/**
 * Gets a lexer implementation from a filename
 * (lookup for a match between lexer implementation's glob patterns and filename's basename)
 *
 * @param filename a path
 *
 * @return NULL or the LexerImplementation which applies to the given filename
 */
SHALL_API const LexerImplementation *lexer_implementation_for_filename(const char *filename)
{
    size_t i;
    char basename[PATH_MAX];

    if (NULL != shall_basename(filename, basename, ARRAY_SIZE(basename))) {
        for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
            if (NULL != available_lexers[i]->patterns) {
                const char * const *pattern;

                for (pattern = available_lexers[i]->patterns; NULL != *pattern; pattern++) {
                    if (0 == fnmatch(*pattern, basename, 0)) {
                        return available_lexers[i];
                    }
                }
            }
        }
    }

    return NULL;
}

/**
 * Gets a lexer implementation from a MIME type
  *
 * @param name a MIME type
 *
 * @return NULL or the LexerImplementation which matches exactly the given MIME type
 */
SHALL_API const LexerImplementation *lexer_implementation_for_mimetype(const char *name)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
        if (NULL != available_lexers[i]->mimetypes) {
            const char * const *mimetype;

            for (mimetype = available_lexers[i]->mimetypes; NULL != *mimetype; mimetype++) {
                if (0 == strcmp(name, *mimetype)) {
                    return available_lexers[i];
                }
            }
        }
    }

    return NULL;
}

/**
 * Executes the given callback for each builtin lexer implementation
 *
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void lexer_implementation_each(void (*cb)(const LexerImplementation *, void *), void *data)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
        cb(available_lexers[i], data);
    }
}

/**
 * Gets official name of a lexer implementation
 *
 * @param imp the lexer implementation
 *
 * @return its name
 */
SHALL_API const char *lexer_implementation_name(const LexerImplementation *imp)
{
    return imp->name;
}

/**
 * Gets description of a lexer implementation
 *
 * @param imp the lexer implementation
 *
 * @return its documentation string
 */
SHALL_API const char *lexer_implementation_description(const LexerImplementation *imp)
{
    return imp->docstr;
}

/**
 * Executes the given callback for each defined MIME type of a given lexer implementation
 *
 * @param imp the lexer implementation
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void lexer_implementation_each_mimetype(const LexerImplementation *imp, void (*cb)(const char *, void *), void *data)
{
    if (NULL != imp->mimetypes) {
        const char * const *mimetype;

        for (mimetype = imp->mimetypes; NULL != *mimetype; mimetype++) {
            cb(*mimetype, data);
        }
    }
}

/**
 * Executes the given callback for each additionnal name (or alias) of a given lexer implementation
 *
 * @param imp the lexer implementation
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void lexer_implementation_each_alias(const LexerImplementation *imp, void (*cb)(const char *, void *), void *data)
{
    if (NULL != imp->aliases) {
        const char * const *alias;

        for (alias = imp->aliases; NULL != *alias; alias++) {
            cb(*alias, data);
        }
    }
}

/**
 * Executes the given callback for each file pattern of a given lexer implementation
 *
 * @param imp the lexer implementation
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void lexer_implementation_each_filename(const LexerImplementation *imp, void (*cb)(const char *, void *), void *data)
{
    if (NULL != imp->patterns) {
        const char * const *pattern;

        for (pattern = imp->patterns; NULL != *pattern; pattern++) {
            cb(*pattern, data);
        }
    }
}

/**
 * Executes the given callback for each predefined option of a given lexer implementation
 *
 * @param imp the lexer implementation
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void lexer_implementation_each_option(const LexerImplementation *imp, void (*cb)(int, const char *, OptionValue, const char *, void *), void *data)
{
    if (NULL != imp->options) {
        LexerOption *lo;

        for (lo = imp->options; NULL != lo->name; lo++) {
            cb(lo->type, lo->name, lo->defval, lo->docstr, data);
        }
    }
}

/**
 * Attempts to find an appropriate lexer implementation from the basename interpreter
 * Example: get Python for "python-27" as filename and a lexer defining "python*" as a
 * possible interpreter pattern
 *
 * @param basename_interpreter the filename (only its basename part)
 *
 * @return NULL if no suitable lexer implementation match else the lexer implementation that matches
 */
static const LexerImplementation *find_interpreter(const char *basename_interpreter)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
        if (NULL != available_lexers[i]->interpreters) {
            const char * const *interpreter;

            for (interpreter = available_lexers[i]->interpreters; NULL != *interpreter; interpreter++) {
                if (0 == fnmatch(*interpreter, basename_interpreter, 0)) {
                    return available_lexers[i];
                }
            }
        }
    }

    return NULL;
}

#ifndef DOXYGEN
# define SHELLMAGIC "#!"
#endif /* !DOXYGEN */

/**
 * Attempts to find out an appropriate lexer (implementation) for a given source.
 *
 * First, we skip an eventual UTF-8 BOM.
 * 
 * Next, we lookup for a shebang, if one is found, we look for a match with the
 * predefined (glob) patterns of the lexer implementations.
 *
 * Else (no shebang found or there is no match), we call the analyse callback of
 * each implementation and return the one which does the best score (int it returns)
 *
 * @param src the string to analyse
 * @param src_len its length
 *
 * @return the LexerImplementation which seems to be the best fit or NULL
 */
SHALL_API const LexerImplementation *lexer_implementation_guess(const char *src, size_t src_len)
{
    const LexerImplementation *best_match;

    best_match = NULL;
    // skip UTF-8 BOM
    if (src_len >= STR_LEN(UTF8_BOM) && 0 == memcmp(src, UTF8_BOM, STR_LEN(UTF8_BOM))) {
        src += STR_LEN(UTF8_BOM);
        src_len -= STR_LEN(UTF8_BOM);
    }
    if (src_len > STR_LEN(SHELLMAGIC) && 0 == memcmp(src, SHELLMAGIC, STR_LEN(SHELLMAGIC))) {
        const char *p, *interpreter_start, *interpreter_end;
        const char * const src_end = src + src_len;

        p = src + STR_LEN(SHELLMAGIC);
        while (p < src_end && (' ' == *p || '\t' == *p)) {
            ++p;
        }
        interpreter_start = p;
        while (p < src_end && (' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p && '\0' != *p)) {
            ++p;
        }
        interpreter_end = p;
        if (interpreter_end > interpreter_start) {
            char path_buffer[PATH_MAX], basename_buffer[PATH_MAX];

//             strncpy(path_buffer, interpreter_start, interpreter_end - interpreter_start);
            // ensure interpreter_end - interpreter_start < PATH_MAX
            memcpy(path_buffer, interpreter_start, interpreter_end - interpreter_start);
            path_buffer[interpreter_end - interpreter_start] = '\0';
            if (0 == strcmp(path_buffer, "/usr/bin/env")) {
                // for stuff like this #!/usr/bin/env -S /usr/local/bin/php -n -q -dsafe_mode=0
                while ('\n' != *p && '\0' != *p) {
                    const char *option_start, *option_end;

                    while (p < src_end && (' ' == *p || '\t' == *p)) {
                        ++p;
                    }
                    option_start = p;
                    while (p < src_end && (' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p && '\0' != *p)) {
                        ++p;
                    }
                    option_end = p;
//                     while (--p > option_start && (' ' == *p == || ('\t' == *p)) {
//                         option_end = p;
//                     }
//                     debug("option found: %.*s", (int) (option_end - option_start), option_start);
                    if (option_end > option_start && '-' != *option_start) {
//                         strncpy(path_buffer, option_start, option_end - option_start);
                        // ensure option_end - option_start < PATH_MAX
                        memcpy(path_buffer, option_start, option_end - option_start);
                        path_buffer[option_end - option_start] = '\0';
                        if (NULL != shall_basename(path_buffer, basename_buffer, ARRAY_SIZE(basename_buffer))) {
                            if (NULL != (best_match = find_interpreter(basename_buffer))) {
                                return best_match;
                            }
                        }
                    }
                }
            } else {
                if (NULL != shall_basename(path_buffer, basename_buffer, ARRAY_SIZE(basename_buffer))) {
                    if (NULL != (best_match = find_interpreter(basename_buffer))) {
                        return best_match;
                    }
                }
            }
        }
        src_len -= p - src + 1;
        src = p + 1;
    }
    {
        size_t i;
        int best_score, score;

        score = best_score = 0;
        for (i = 0; i < ARRAY_SIZE(available_lexers); i++) {
            if (NULL != available_lexers[i]->analyse) {
                if ((score = available_lexers[i]->analyse(src, src_len)) > best_score) {
                    best_score = score;
                    best_match = available_lexers[i];
                }
            }
        }
    }

    return best_match;
}

/* ========== LexerOption functions ========== */

/**
 * Helper to retrieve option specification (LexerOption) for a given
 * lexer implementation by its name
 *
 * @param imp the lexer implementation
 * @param name the option's name
 *
 * @return NULL if the option is not available or its specification
 */
static LexerOption *lexer_option_by_name(const LexerImplementation *imp, const char *name)
{
    if (NULL != imp->options) {
        LexerOption *lo;

        for (lo = imp->options; NULL != lo->name; lo++) {
            if (0 == strcmp(name, lo->name)) {
                return lo;
            }
        }
    }

    return NULL;
}

/* ========== Lexer functions ========== */

/**
 * Gets lexer implementation from a lexer
 *
 * @param lexer the lexer
 *
 * @return the LexerImplementation the lexer depends on
 */
SHALL_API const LexerImplementation *lexer_implementation(Lexer *lexer)
{
    return lexer->imp;
}

/**
 * Creates a new lexer from a lexer implementation
 *
 * @param imp the lexer implementation
 *
 * @return a new lexer for the given implementation
 */
SHALL_API Lexer *lexer_create(const LexerImplementation *imp)
{
    Lexer *lexer;

    if (NULL != (lexer = malloc(sizeof(*lexer) + imp->data_size))) {
        LexerData *data;

        lexer->imp = imp;
        bzero(lexer->optvals, imp->data_size);
        data = (LexerData *) &lexer->optvals;
        if (NULL == (data->state_stack = malloc(sizeof(*data->state_stack)))) {
            free(lexer);
            return NULL;
        }
        darray_init(data->state_stack, 0, sizeof(data->state));
        if (NULL != imp->options) {
            LexerOption *lo;

            for (lo = imp->options; NULL != lo->name; lo++) {
// debug("%s = %zu (%zu/%zu)", lo->name, lo->offset / sizeof(OptionValue), lo->offset, sizeof(OptionValue));
                memcpy(&lexer->optvals[lo->offset / sizeof(OptionValue)], &lo->defval, sizeof(lo->defval));
            }
        }
        if (NULL != imp->init) {
            imp->init((LexerData *) lexer->optvals);
        }
        if (NULL != imp->implicit) {
            const LexerImplementation * const *dep;

            for (dep = imp->implicit; NULL != *dep; dep++) {
// debug("%s depends on %s", imp->name, (*dep)->name);
            }
        }
    }

    return lexer;
}

/**
 * Apply a callback on sublexers (an other lexer used as an option) of a lexer
 * This function is intended for garbage collectors of bindings.
 *
 * @note the callback is not called for the top lexer
 *
 * @param lexer the "parent" lexer
 * @param cb the callback to invoke for each sublexers
 */
SHALL_API void lexer_each_sublexers(Lexer *lexer, on_lexer_destroy_cb_t cb)
{
    if (NULL != lexer->imp->options) {
        LexerOption *lo;

        for (lo = lexer->imp->options; NULL != lo->name; lo++) {
            if (OPT_TYPE_LEXER == lo->type) {
                Lexer *auxiliary;
                OptionValue optval;

                optval = lexer->optvals[lo->offset / sizeof(OptionValue)];
                auxiliary = lexer_unwrap(optval);
                if (NULL != auxiliary) {
                    cb(auxiliary);
                }
            }
        }
    }
}

/**
 * Disposes of a lexer (frees memory). This function should be called
 * once per "object" returned by lexer_create().
 *
 * @param lexer the lexer to dispose of
 * @param cb a callback to invoke on each sublexer
 *
 * The use of *cb* is mostly intended for garbage collection of bindings.
 * It can also be defined to lexer_destroy in CLI program to easily free
 * memory.
 */
SHALL_API void lexer_destroy(Lexer *lexer, on_lexer_destroy_cb_t cb)
{
    LexerData *data;

    data = (LexerData *) &lexer->optvals;
    if (NULL != data->state_stack) {
        darray_destroy(data->state_stack);
        free(data->state_stack);
    }
    if (NULL != lexer->imp->options) {
        LexerOption *lo;

        for (lo = lexer->imp->options; NULL != lo->name; lo++) {
            OptionValue optval;

            optval = lexer->optvals[lo->offset / sizeof(OptionValue)];
            switch (lo->type) {
                case OPT_TYPE_BOOL:
                case OPT_TYPE_INT:
                    /* NOP */
                    break;
                case OPT_TYPE_STRING:
                    OPT_STRVAL_FREE(optval, lo->defval);
                    break;
                case OPT_TYPE_LEXER:
                {
                    if (NULL != cb) {
                        Lexer *auxiliary;

                        auxiliary = lexer_unwrap(optval);
                        if (NULL != auxiliary) {
                            cb(auxiliary);
                        }
                    }
                    break;
                }
                default:
                    assert(0);
                    break;
            }
        }
    }
    free(lexer);
}

/**
 * Creates a lexer and set its options from a (query) string.
 * Example: php?asp_tags=on&start_inline=on to create a PHP
 * lexer with asp_tags and start_inline options valued to on.
 *
 * @todo preferer an other syntax to set sublexer and their options.
 * Something like: `lexer1[option=value;secondary=lexer2[option=value;secondary=lexer3]]`
 * (implies to use a stack)
 *
 * @param name the query string to parse
 * @param real_name if not null, points on the official implementation name
 *
 * @return NULL if implementation name matches nothing else a lexer
 */
SHALL_API Lexer *lexer_from_string(const char *name, const char **real_name)
{
    char *qm;
    Lexer *lexer;
    const LexerImplementation *imp;

    imp = NULL;
    lexer = NULL;
    if (NULL != (qm = strchr(name, '?'))) {
        char *copy, *ptr, *pname, *pvalue;

        copy = strdup(name);
        pname = copy + (qm - name);
        *pname++ = '\0';
//         debug("name = >%s<", copy);
        if (NULL != (imp = lexer_implementation_by_name(copy))) {
            lexer = lexer_create(imp);
            ptr = strpbrk(pname, ";&");
            if (NULL == ptr) {
//                 debug(">%s< : no value", pname);
                lexer_set_option_as_string(lexer, pname, "", 0);
            } else {
                do {
                    *ptr = '\0';
                    pvalue = memchr(pname, '=', ptr - pname - 1);
                    if (ptr - pname > 1 && pname != pvalue) {
                        if (NULL != pvalue) {
                            *pvalue++ = '\0';
//                             debug(">%s< : >%s<", pname, pvalue);
                            lexer_set_option_as_string(lexer, pname, pvalue, strlen(pvalue));
                        } else {
//                             debug(">%s< : no value", pname);
                            lexer_set_option_as_string(lexer, pname, "", 0);
                        }
                    }
                    pname = ptr + 1;
                } while (NULL != (ptr = strpbrk(pname, ";&")));
                if (NULL != pname && '\0' != *pname) {
                    if (NULL != (pvalue = strchr(pname, '='))) {
                        *pvalue++ = '\0';
//                         debug(">%s< : >%s<", pname, pvalue);
                        lexer_set_option_as_string(lexer, pname, pvalue, strlen(pvalue));
                    } else {
//                         debug(">%s< : no value", pname);
                        lexer_set_option_as_string(lexer, pname, "", 0);
                    }
                }
            }
        }
        free(copy);
    } else {
        if (NULL != (imp = lexer_implementation_by_name(name))) {
            lexer = lexer_create(imp);
        }
    }
    if (NULL != imp && NULL != real_name) {
        *real_name = imp->name;
    }

    return lexer;
}

/**
 * Gets current value of a lexer option
 *
 * @param lexer the lexer
 * @param name the option name
 * @param ptr the pointer where to put the address of the value. Based on
 * the returned value (its type), caller should know what to do of `*ptr` next.
 *
 * @return -1 if option's name is invalid else one of OPT_TYPE_* constant
 */
SHALL_API int lexer_get_option(Lexer *lexer, const char *name, OptionValue **ptr)
{
    LexerOption *lo;

    if (NULL != (lo = lexer_option_by_name(lexer->imp, name))) {
// debug("name = %zu\n", lo->offset / sizeof(OptionValue));
        *ptr = &lexer->optvals[lo->offset / sizeof(OptionValue)];
        return lo->type;
    }

    return -1;
}

/**
 * Sets a lexer as lexer option
 *
 * @param lexer the lexer
 * @param name the name of the option
 * @param type the type of newval
 * @param newval the new value
 * @param oldval the old value if this was a Lexer, NULL if you don't want it (intended for garbage collection)
 *
 * @return a int as follows:
 *  + OPT_SUCCESS if no error
 *  + OPT_ERR_TYPE_MISMATCH if type's value does not match expected one
 *  + OPT_ERR_INVALID_OPTION if this is not a valid option name
 *  + OPT_ERR_UNKNOWN_LEXER if option expect a lexer but value is not a valid lexer implementation name
 *  + OPT_ERR_INVALID_VALUE if the value is not acceptable
 */
SHALL_API int lexer_set_option(Lexer *lexer, const char *name, OptionType type, OptionValue newval, void **oldval)
{
    LexerOption *lo;

    if (NULL != (lo = lexer_option_by_name(lexer->imp, name))) {
        if (lo->type != type) {
            return OPT_ERR_TYPE_MISMATCH;
        } else {
            OptionValue *optvalptr;

            optvalptr = &lexer->optvals[lo->offset / sizeof(OptionValue)];
            if (OPT_TYPE_LEXER == lo->type && lexer_unwrap(newval) == lexer) {
                return OPT_ERR_INVALID_OPTION;
            }
            if (NULL != oldval) {
                if (OPT_TYPE_LEXER == lo->type) {
                    *oldval = OPT_LEXPTR(*optvalptr);
                } else {
                    *oldval = NULL; // inform caller that wasn't a lexer
                }
            }
            opt_copy(lo->type, optvalptr, newval, lo->defval);
            return OPT_SUCCESS;
        }
    }

    return OPT_ERR_INVALID_OPTION;
}

/**
 * Sets a lexer option from a string. This string will be parsed according to the option type
 *
 * @note this function is reserved to CLI tools (shall and shalltest)
 *
 * @param lexer the lexer
 * @param name the name of the option to set
 * @param value the value as string, to parse
 * @param value_len its length
 *
 * @return a int as follows:
 *  + OPT_SUCCESS if no error
 *  + OPT_ERR_INVALID_OPTION if this is not a valid option name
 *  + OPT_ERR_UNKNOWN_LEXER if option expect a lexer but value is not a valid lexer implementation name
 *  + OPT_ERR_INVALID_VALUE if the value is not acceptable
 */
SHALL_API int lexer_set_option_as_string(Lexer *lexer, const char *name, const char *value, size_t value_len)
{
    LexerOption *lo;

    if (NULL != (lo = lexer_option_by_name(lexer->imp, name))) {
        return parse_option_as_string(&lexer->optvals[lo->offset / sizeof(OptionValue)], lo->type, value, value_len, 0);
    }

    return OPT_ERR_INVALID_OPTION;
}

/* ========== FormatterImplementation functions ========== */

/**
 * Gets official name of a formatter implementation
 *
 * @param imp the formatter implementation
 *
 * @return its name
 */
SHALL_API const char *formatter_implementation_name(const FormatterImplementation *imp)
{
    return imp->name;
}

/**
 * Gets description of a formatter implementation
 *
 * @param imp the formatter implementation
 *
 * @return its documentation string
 */
SHALL_API const char *formatter_implementation_description(const FormatterImplementation *imp)
{
    return imp->docstr;
}

/**
 * Gets a formatter implementation from its name.
 *
 * @param name the implementation name
 *
 * @return NULL or the FormatterImplementation which matches the given name
 */
SHALL_API const FormatterImplementation *formatter_implementation_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_formatters); i++) {
        if (0 == ascii_strcasecmp(name, available_formatters[i]->name)) {
            return available_formatters[i];
        }
    }

    return NULL;
}

/**
 * Executes the given callback for each builtin formatter implementation
 *
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void formatter_implementation_each(void (*cb)(const FormatterImplementation *, void *), void *data)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(available_formatters); i++) {
        cb(available_formatters[i], data);
    }
}

/**
 * Executes the given callback for each predefined option of a given formatter implementation
 *
 * @param imp the formatter implementation
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void formatter_implementation_each_option(const FormatterImplementation *imp, void (*cb)(int, const char *, OptionValue, const char *, void *), void *data)
{
    if (NULL != imp->options) {
        FormatterOption *fo;

        for (fo = imp->options; NULL != fo->name; fo++) {
            cb(fo->type, fo->name, fo->defval, fo->docstr, data);
        }
    }
}

/**
 * Default callback for internal formatters
 *
 * @param fmt the concerned formatter
 * @param define true if we are in initialization step, to allocate space for options if the formatter needs to (unused)
 * @param offset the internal offset of the member in Formatter.options structure
 * @param name the name of the option for which we want its value (unused)
 * @param name_len its length
 *
 * @return a pointer to option's value to read or alter it
 */
SHALL_API OptionValue *formatter_implementation_default_get_option_ptr(Formatter *fmt, int UNUSED(define), size_t offset, const char *UNUSED(name), size_t UNUSED(name_len))
{
    if (NULL != fmt->imp->options) {
        return &fmt->optvals[offset / sizeof(OptionValue)];
    }

    return NULL;
}

/* ========== FormatterOption functions ========== */

/**
 * Helper to retrieve option specification (FormatterOption) for a given
 * formatter implementation by its name
 *
 * @param imp the formatter implementation
 * @param name the option's name
 *
 * @return NULL if the option is not available or its specification
 */
static FormatterOption *formatter_option_by_name(const FormatterImplementation *imp, const char *name)
{
    if (NULL != imp->options) {
        FormatterOption *fo;

        for (fo = imp->options; NULL != fo->name; fo++) {
            if (0 == strcmp(name, fo->name)) {
                return fo;
            }
        }
    }

    return NULL;
}

/* ========== Formatter functions ========== */

/**
 * Gets formatter implementation from a formatter
 *
 * @param fmt the formatter
 *
 * @return the FormatterImplementation the formatter depends on
 */
SHALL_API const FormatterImplementation *formatter_implementation(Formatter *fmt)
{
    return fmt->imp;
}

/**
 * Creates a new formatter from a formatter implementation which may inherit
 * from an other one.
 *
 * @param super the targeted formatter implementation from which we intent to
 * create a new formatter
 * @param base the parent formatter implementation
 *
 * @return a new formatter for the given implementation
 */
SHALL_API Formatter *formatter_create_inherited(const FormatterImplementation *super, const FormatterImplementation *base)
{
    Formatter *fmt;

    if (NULL != (fmt = malloc(sizeof(*fmt) + super->data_size/* - sizeof(fmt->data)*/))) {
        fmt->imp = super;
        hashtable_ascii_cs_init(&fmt->optmap, NULL, NULL, NULL);
        if (NULL != base->options) {
            FormatterOption *fo;

            for (fo = base->options; NULL != fo->name; fo++) {
                OptionValue *optvalptr;

                if (NULL != (optvalptr = base->get_option_ptr(fmt, 1, fo->offset, fo->name, fo->name_len))) {
                    hashtable_put(&fmt->optmap, 0, fo->name, optvalptr, NULL);
                    memcpy(optvalptr, &fo->defval, sizeof(fo->defval));
                }
            }
        }
#if 0
        if (super != base && NULL != super->options) {
            FormatterOption *fo;

            for (fo = super->options; NULL != fo->name; fo++) {
                OptionValue *optvalptr;

                if (NULL != (optvalptr = super->get_option_ptr(fmt, 1, fo->offset, fo->name, fo->name_len))) {
                    memcpy(optvalptr, &fo->defval, sizeof(fo->defval));
                }
            }
        }
#endif
    }

    return fmt;
}

/**
 * Creates a new formatter from a **base** formatter implementation
 *
 * @param imp the formatter implementation
 *
 * @return a new formatter for the given implementation
 */
SHALL_API Formatter *formatter_create(const FormatterImplementation *imp)
{
#if 0
    Formatter *fmt;

    if (NULL != (fmt = malloc(sizeof(*fmt) + imp->data_size/* - sizeof(fmt->data)*/))) {
        fmt->imp = imp;
        if (NULL != imp->options) {
            FormatterOption *fo;

            for (fo = imp->options; NULL != fo->name; fo++) {
                OptionValue *optvalptr;

                if (NULL != (optvalptr = imp->get_option_ptr(fmt, 1, fo->offset, fo->name, fo->name_len))) {
                    memcpy(optvalptr, &fo->defval, sizeof(fo->defval));
                }
            }
        }
#if 0
        if (NULL != imp->init) {
            imp->init(&fmt->data);
        }
#endif
    }

    return fmt;
#else
        return formatter_create_inherited(imp, imp);
#endif
}

/**
 * Disposes of a formatter (frees memory). This function should be called
 * once per "object" returned by formatter_create().
 *
 * @param fmt the formatter to dispose of
 */
SHALL_API void formatter_destroy(Formatter *fmt)
{
    if (NULL != fmt->imp->options) {
        FormatterOption *fo;

        for (fo = fmt->imp->options; NULL != fo->name; fo++) {
            switch (fo->type) {
                case OPT_TYPE_LEXER:
                case OPT_TYPE_BOOL:
                case OPT_TYPE_INT:
                    /* NOP */
                    break;
                case OPT_TYPE_STRING:
                {
                    OptionValue *optvalptr;

                    if (NULL != (optvalptr = fmt->imp->get_option_ptr(fmt, 0, fo->offset, fo->name, fo->name_len))) {
                        OPT_STRVAL_FREE(*optvalptr, fo->defval);
                    }
                    break;
                }
                default:
                    assert(0);
                    break;
            }
        }
    }
    hashtable_destroy(&fmt->optmap);
    free(fmt);
}

/**
 * Sets a formatter option from a string. This string will be parsed according to the option type
 *
 * @note this function is reserved to CLI tools (shall and shalltest)
 *
 * @param fmt the formatter
 * @param name the name of the option to set
 * @param value the value as string, to parse
 * @param value_len its length
 *
 * @return a int as follows:
 *  + OPT_SUCCESS if no error
 *  + OPT_ERR_INVALID_OPTION if this is not a valid option name
 *  + OPT_ERR_UNKNOWN_LEXER if option expect a lexer but value is not a valid lexer implementation name
 *  + OPT_ERR_INVALID_VALUE if the value is not acceptable
 */
SHALL_API int formatter_set_option_as_string(Formatter *fmt, const char *name, const char *value, size_t value_len)
{
    FormatterOption *fo;

    if (NULL != (fo = formatter_option_by_name(fmt->imp, name))) {
        OptionValue *optvalptr;

#if 0
        if (NULL == (optvalptr = fmt->imp->get_option_ptr(fmt, 0, fo->offset, fo->name, fo->name_len))) {
            return OPT_ERR_INVALID_OPTION;
        }
#else
        optvalptr = NULL;
        if (!hashtable_get(&fmt->optmap, name, &optvalptr)) {
            return OPT_ERR_INVALID_OPTION;
        }
#endif
        return parse_option_as_string(optvalptr, fo->type, value, value_len, 1);
    }

    return OPT_ERR_INVALID_OPTION;
}

/**
 * Sets a formatter option
 *
 * @param fmt the formatter
 * @param name the name of the option
 * @param type the type of newval
 * @param newval the new value
 *
 * @return a int as follows:
 *  + OPT_SUCCESS if no error
 *  + OPT_ERR_TYPE_MISMATCH if type's value does not match expected one
 *  + OPT_ERR_INVALID_OPTION if this is not a valid option name
 *  + OPT_ERR_UNKNOWN_LEXER if option expect a lexer but value is not a valid lexer implementation name
 *  + OPT_ERR_INVALID_VALUE if the value is not acceptable
 */
SHALL_API int formatter_set_option(Formatter *fmt, const char *name, OptionType type, OptionValue newval)
{
    FormatterOption *fo;

    if (NULL != (fo = formatter_option_by_name(fmt->imp, name))) {
        OptionValue *optvalptr;

        optvalptr = fmt->imp->get_option_ptr(fmt, 0, fo->offset, fo->name, fo->name_len);
        if (NULL == optvalptr) {
            return OPT_ERR_INVALID_OPTION;
        } else if (type != fo->type) {
            return OPT_ERR_TYPE_MISMATCH;
        } else {
            opt_copy(fo->type, optvalptr, newval, fo->defval);
            return OPT_SUCCESS;
        }
    }

    return OPT_ERR_INVALID_OPTION;
}

/**
 * Gets current value of a formatter option
 *
 * @param fmt the formatter
 * @param name the option name
 * @param ptr the pointer where to put the address of the value. Based on
 * the returned value, caller should know what to do of `*ptr` next.
 *
 * @return -1 if option's name is invalid else one of OPT_TYPE_* constant
 */
SHALL_API int formatter_get_option(Formatter *fmt, const char *name, OptionValue **ptr)
{
    FormatterOption *fo;

    if (NULL != (fo = formatter_option_by_name(fmt->imp, name))) {
        if (NULL != ptr) {
            *ptr = fmt->imp->get_option_ptr(fmt, 0, fo->offset, fo->name, fo->name_len);
        }
        return fo->type;
    }

    return -1;
}

/* ========== Highlighting ========== */

/**
 * Highlight string according to given lexer and formatter
 *
 * @param lexer the lexer to tokenize the input string
 * @param fmt the formatter to generate output from tokens
 * @param src the input string
 * @param dest the output string
 *
 * @return the length of *dest
 */
SHALL_API size_t highlight_string(Lexer *lexer, Formatter *fmt, const char *src, char **dest)
{
    String *buffer;
    int prev, token;
    LexerInput yy;
    size_t src_len, buffer_len;

    src_len = strlen(src);
    // skip UTF-8 BOM
    if (src_len >= STR_LEN(UTF8_BOM) && 0 == memcmp(src, UTF8_BOM, STR_LEN(UTF8_BOM))) {
        src += STR_LEN(UTF8_BOM);
        src_len -= STR_LEN(UTF8_BOM);
    }
    buffer = string_new();
    prev = -1;
//     yy.bol = 1;
//     yy.lineno = 0;
    if (NULL != fmt->imp->start_document) {
        fmt->imp->start_document(buffer, &fmt->optvals);
    }
    // skip shebang
    if (src_len > STR_LEN(SHELLMAGIC) && 0 == memcmp(src, SHELLMAGIC, STR_LEN(SHELLMAGIC))) {
        char *lf;

        if (NULL != (lf = strchr(src, '\n'))) {
            fmt->imp->start_token(prev = IGNORABLE, buffer, &fmt->optvals);
            fmt->imp->write_token(buffer, src, ++lf - src, &fmt->optvals);
            src_len -= lf - src;
            src = lf;
        }
    }
    yy.cursor = (YYCTYPE *) src;
    yy.limit = (YYCTYPE *) src + src_len;
    // TODO: prendre le token avant de rencontrer la fin
    // YYFILL ne doit pas permettre de prendre en compte le type du token au début sur un token de plusieurs caractères
    while ((token = lexer->imp->yylex(&yy, (LexerData *) lexer->optvals)) > 0) {
        int yyleng;

        yyleng = yy.cursor - yy.yytext;
        if (prev != token) {
            if (prev != -1/* && prev != IGNORABLE*/) {
                fmt->imp->end_token(prev, buffer, &fmt->optvals);
            }
//             if (token != IGNORABLE) {
                fmt->imp->start_token(token, buffer, &fmt->optvals);
//             }
        }
        fmt->imp->write_token(buffer, yy.yytext, yyleng, &fmt->optvals);
        prev = token;
    }
    if (prev != -1/* && prev != IGNORABLE*/) {
        fmt->imp->end_token(token, buffer, &fmt->optvals);
    }
    if (NULL != fmt->imp->end_document) {
        fmt->imp->end_document(buffer, &fmt->optvals);
    }

    buffer_len = buffer->len;
    *dest = string_orphan(buffer);

    return buffer_len;
}
