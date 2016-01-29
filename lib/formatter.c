#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "shall.h"
#include "formatter.h"

// #ifndef DOXYGEN
extern const FormatterImplementation _bbcodefmt;
extern const FormatterImplementation _htmlfmt;
extern const FormatterImplementation _termfmt;
// #endif /* !DOXYGEN */

static const FormatterImplementation *available_formatters[] = {
    &_bbcodefmt,
    &_htmlfmt,
    &_termfmt,
    NULL
};

/**
 * Exposes the number of available builtin formatters
 *
 * @note for external use only
 */
SHALL_API const size_t SHALL_FORMATTER_COUNT = ARRAY_SIZE(available_formatters) - 1;

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
    const FormatterImplementation **imp;

    for (imp = available_formatters; NULL != *imp; imp++) {
        if (0 == ascii_strcasecmp(name, (*imp)->name)) {
            return *imp;
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
    const FormatterImplementation **imp;

    for (imp = available_formatters; NULL != *imp; imp++) {
        cb(*imp, data);
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
 * Initialize an iterator to iterate on available formatters
 *
 * @param it the iterator to set properly
 */
SHALL_API void formatter_implementation_to_iterator(Iterator *it)
{
    null_terminated_ptr_array_to_iterator(it, (void *) available_formatters);
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

/*
static const FormatterOption *NO_FMT_OPTIONS = (FormatterOption []) {
    END_OF_FORMATTER_OPTIONS
};
*/

/**
 * Initialize an iterator to iterate on available options of the given
 * formatter
 *
 * @param it the iterator to set properly
 * @param imp the formatter implementation to iterate on its options
 *
 * @return false if the formatter implementation defines no option
 * (there is nothing to iterate). The given Iterator will not
 * subsequently be initialized in this case.
 */
SHALL_API bool formatter_implementation_options_to_iterator(Iterator *it, const FormatterImplementation *imp)
{
    if (NULL != imp->options) {
        null_sentineled_field_terminated_array_to_iterator(it, imp->options, sizeof(FormatterOption), offsetof(FormatterOption, name));
    }

    return NULL != imp->options;
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
#ifdef TEST
        hashtable_ascii_cs_init(&fmt->optmap, NULL, NULL, NULL); // we need to strdup keys to make sure that they still exists after?
#endif
        if (NULL != base->options) {
            FormatterOption *fo;

            for (fo = base->options; NULL != fo->name; fo++) {
                OptionValue *optvalptr;

                if (NULL != (optvalptr = base->get_option_ptr(fmt, 1, fo->offset, fo->name, fo->name_len))) {
#ifdef TEST
                    hashtable_put(&fmt->optmap, 0, fo->name, optvalptr, NULL);
#endif
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
    if (NULL != fmt->imp->finalize) {
        fmt->imp->finalize(&fmt->optvals);
    }
    if (NULL != fmt->imp->options) {
        FormatterOption *fo;

        for (fo = fmt->imp->options; NULL != fo->name; fo++) {
            switch (fo->type) {
                case OPT_TYPE_INT:
                case OPT_TYPE_BOOL:
                case OPT_TYPE_LEXER:
                case OPT_TYPE_THEME:
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
#ifdef TEST
    hashtable_destroy(&fmt->optmap);
#endif
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

#ifndef TEST
        if (NULL == (optvalptr = fmt->imp->get_option_ptr(fmt, 0, fo->offset, fo->name, fo->name_len))) {
            return OPT_ERR_INVALID_OPTION;
        }
#else
        optvalptr = NULL;
        if (!hashtable_get(&fmt->optmap, name, &optvalptr)) {
            return OPT_ERR_INVALID_OPTION;
        }
#endif
        return option_parse_as_string(optvalptr, fo->type, value, value_len, 1);
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
            option_copy(fo->type, optvalptr, newval, fo->defval);
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
