/**
 * @file lib/lexer.c
 * @brief interface with lexers
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <limits.h> /* PATH_MAX */

#include "cpp.h"
#include "utils.h"
#include "lexer.h"
#include "shall.h"

#ifndef DOXYGEN
extern const LexerImplementation apache_lexer;
extern const LexerImplementation bash_lexer;
extern const LexerImplementation c_lexer;
extern const LexerImplementation cmake_lexer;
extern const LexerImplementation css_lexer;
extern const LexerImplementation diff_lexer;
extern const LexerImplementation dtd_lexer;
extern const LexerImplementation erb_lexer; // provided by ruby lexer
extern const LexerImplementation go_lexer;
extern const LexerImplementation html_lexer; // provided by xml lexer
extern const LexerImplementation js_lexer;
extern const LexerImplementation json_lexer;
extern const LexerImplementation lua_lexer;
extern const LexerImplementation nginx_lexer;
extern const LexerImplementation php_lexer;
extern const LexerImplementation postgresql_lexer;
extern const LexerImplementation python_lexer;
extern const LexerImplementation ruby_lexer;
extern const LexerImplementation text_lexer;
extern const LexerImplementation twig_lexer;
extern const LexerImplementation varnish_lexer;
extern const LexerImplementation xml_lexer;
#endif /* !DOXYGEN */

// only final/public lexers
static const LexerImplementation *available_lexers[] = {
    &apache_lexer,
    &bash_lexer,
    &c_lexer,
    &cmake_lexer,
    &css_lexer,
    &diff_lexer,
    &dtd_lexer,
    &erb_lexer,
    &go_lexer,
    &html_lexer,
    &lua_lexer,
    &js_lexer,
    &json_lexer,
    &nginx_lexer,
    &php_lexer,
    &postgresql_lexer,
    &python_lexer,
    &ruby_lexer,
    &text_lexer, // may be a good idea to keep it last?
    &twig_lexer,
    &varnish_lexer,
    &xml_lexer,
    NULL
};

/**
 * Exposes the number of available builtin lexers
 *
 * @note for external use only (preallocation for example)
 */
SHALL_API const size_t SHALL_LEXER_COUNT = ARRAY_SIZE(available_lexers) - 1;

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
    if ('\0' != *name) {
        const LexerImplementation **imp;

        for (imp = available_lexers; NULL != *imp; imp++) {
            if (0 == ascii_strcasecmp(name, (*imp)->name)) {
                return *imp;
            }
            if (NULL != (*imp)->aliases) {
                const char * const *alias;

                for (alias = (*imp)->aliases; NULL != *alias; alias++) {
                    if (0 == ascii_strcasecmp(name, *alias)) {
                        return *imp;
                    }
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
    char basename[PATH_MAX];

    if (NULL != shall_basename(filename, basename, ARRAY_SIZE(basename))) {
        const LexerImplementation **imp;

        for (imp = available_lexers; NULL != *imp; imp++) {
            if (NULL != (*imp)->patterns) {
                const char * const *pattern;

                for (pattern = (*imp)->patterns; NULL != *pattern; pattern++) {
                    if (0 == fnmatch(*pattern, basename, FNM_CASEFOLD)) {
                        return *imp;
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
    const LexerImplementation **imp;

    for (imp = available_lexers; NULL != *imp; imp++) {
        if (NULL != (*imp)->mimetypes) {
            const char * const *mimetype;

            for (mimetype = (*imp)->mimetypes; NULL != *mimetype; mimetype++) {
                if (0 == strcmp(name, *mimetype)) {
                    return *imp;
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
    const LexerImplementation **imp;

    for (imp = available_lexers; NULL != *imp; imp++) {
        cb(*imp, data);
    }
}

/**
 * Initialize an iterator to iterate on available formatters
 *
 * @param it the iterator to set properly
 */
SHALL_API void lexer_implementations_to_iterator(Iterator *it)
{
    null_terminated_ptr_array_to_iterator(it, (void *) available_lexers);
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
 * Initialize an iterator to iterate on MIME types associated to the given
 * lexer implementation
 *
 * @param it the iterator to set properly
 * @param imp the lexer implementation to iterate on its mimetypes
 *
 * @return false if the lexer implementation defines no mimetypes
 * (there is nothing to iterate). The given Iterator will not
 * subsequently be initialized in this case.
 */
SHALL_API bool lexer_implementation_mimetypes_to_iterator(Iterator *it, const LexerImplementation *imp)
{
    if (NULL != imp->mimetypes) {
        null_terminated_ptr_array_to_iterator(it, (void *) imp->mimetypes);
    }

    return NULL != imp->mimetypes;
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
 * Initialize an iterator to iterate on aliases associated to the given
 * lexer implementation
 *
 * @param it the iterator to set properly
 * @param imp the lexer implementation to iterate on its aliases
 *
 * @return false if the lexer implementation defines no aliases
 * (there is nothing to iterate). The given Iterator will not
 * subsequently be initialized in this case.
 */
SHALL_API bool lexer_implementation_aliases_to_iterator(Iterator *it, const LexerImplementation *imp)
{
    if (NULL != imp->aliases) {
        null_terminated_ptr_array_to_iterator(it, (void *) imp->aliases);
    }

    return NULL != imp->aliases;
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
 * Initialize an iterator to iterate on filenames (fnmatch pattern) associated
 * to the given lexer implementation
 *
 * @param it the iterator to set properly
 * @param imp the lexer implementation to iterate on its filenames
 *
 * @return false if the lexer implementation defines no filenames
 * (there is nothing to iterate). The given Iterator will not
 * subsequently be initialized in this case.
 */
SHALL_API bool lexer_implementation_filenames_to_iterator(Iterator *it, const LexerImplementation *imp)
{
    if (NULL != imp->patterns) {
        null_terminated_ptr_array_to_iterator(it, (void *) imp->patterns);
    }

    return NULL != imp->patterns;
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
 * Initialize an iterator to iterate on available options of the given
 * lexer implementation
 *
 * @param it the iterator to set properly
 * @param imp the lexer implementation to iterate on its options
 *
 * @return false if the lexer implementation defines no option
 * (there is nothing to iterate). The given Iterator will not
 * subsequently be initialized in this case.
 */
SHALL_API bool lexer_implementation_options_to_iterator(Iterator *it, const LexerImplementation *imp)
{
    if (NULL != imp->options) {
        null_sentineled_field_terminated_array_to_iterator(it, imp->options, sizeof(LexerOption), offsetof(LexerOption, name));
    }

    return NULL != imp->options;
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
    const LexerImplementation **imp;

    for (imp = available_lexers; NULL != *imp; imp++) {
        if (NULL != (*imp)->interpreters) {
            const char * const *interpreter;

            for (interpreter = (*imp)->interpreters; NULL != *interpreter; interpreter++) {
                if (0 == fnmatch(*interpreter, basename_interpreter, 0)) {
                    return (*imp);
                }
            }
        }
    }

    return NULL;
}

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
        int best_score, score;
        const LexerImplementation **imp;

        score = best_score = 0;
        for (imp = available_lexers; NULL != *imp; imp++) {
            if (NULL != (*imp)->analyse) {
                if ((score = (*imp)->analyse(src, src_len)) > best_score) {
                    best_match = *imp;
                    best_score = score;
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
    LexerOption *lo;
    size_t additionnal_size_to_alloc;

    additionnal_size_to_alloc = 0;
    if (NULL != imp->options) {
        for (lo = imp->options; NULL != lo->name; lo++)
            ;
        additionnal_size_to_alloc = (lo - imp->options) * sizeof(OptionValue);
    }
    if (NULL != (lexer = malloc(sizeof(*lexer) + additionnal_size_to_alloc))) {
        lexer->imp = imp;
        if (NULL != imp->options) {
            for (lo = imp->options; NULL != lo->name; lo++) {
// debug("%s = %zu (%zu/%zu)", lo->name, lo->offset / sizeof(OptionValue), lo->offset, sizeof(OptionValue));
                memcpy(&lexer->optvals[lo->offset / sizeof(OptionValue)], &lo->defval, sizeof(lo->defval));
            }
        }
#ifdef TEST
        if (NULL != imp->implicit) {
            const LexerImplementation * const *dep;

            for (dep = imp->implicit; NULL != *dep; dep++) {
// debug("%s depends on %s", imp->name, (*dep)->name);
            }
        }
#endif /* TEST */
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
                OptionValue optval;

                optval = lexer->optvals[lo->offset / sizeof(OptionValue)];
                if (NULL != OPT_LEXPTR(optval)) {
                    cb(OPT_LEXPTR(optval));
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
    if (NULL != lexer->imp->options) {
        LexerOption *lo;

        for (lo = lexer->imp->options; NULL != lo->name; lo++) {
            OptionValue optval;

            optval = lexer->optvals[lo->offset / sizeof(OptionValue)];
            switch (lo->type) {
                case OPT_TYPE_INT:
                case OPT_TYPE_BOOL:
                case OPT_TYPE_THEME:
                    /* NOP */
                    break;
                case OPT_TYPE_STRING:
                    OPT_STRVAL_FREE(optval, lo->defval);
                    break;
                case OPT_TYPE_LEXER:
                {
                    if (NULL != cb) {
                        if (NULL != OPT_LEXPTR(optval)) {
                            cb(OPT_LEXPTR(optval));
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
 * @todo prefer an other syntax to set sublexer and their options.
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
            option_copy(lo->type, optvalptr, newval, lo->defval);
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
        return option_parse_as_string(&lexer->optvals[lo->offset / sizeof(OptionValue)], lo->type, value, value_len, 0);
    }

    return OPT_ERR_INVALID_OPTION;
}
