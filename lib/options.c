/**
 * @file lib/options.c
 * @brief common functions to handle lexers and formatters options
 */

#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "options.h"
#include "shall.h"
#include "themes.h"

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
void option_copy(OptionType type, OptionValue *dest, OptionValue src, OptionValue defval)
{
    switch (type) {
        case OPT_TYPE_BOOL:
        case OPT_TYPE_INT:
        case OPT_TYPE_LEXER:
        case OPT_TYPE_THEME:
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
# define STRCASECMP_L(s, sl, ss) \
    ascii_strcasecmp_l(s, sl, ss, STR_LEN(ss))
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
int option_parse_as_string(OptionValue *optvalptr, int type, const char *value, size_t value_len, int reject_lexer_type)
{
    switch (type) {
        case OPT_TYPE_BOOL:
        {
            long bval;

            bval = 0;
            if ('\0' == *value) { // explicitely accept "" as true, see this case as "defined is true"
                bval = 1;
            } else if (0 == STRCASECMP_L(value, value_len, "on") || 0 == STRCASECMP_L(value, value_len, "true")) {
                bval = 1;
            } else if (0 == STRCASECMP_L(value, value_len, "off") || 0 == STRCASECMP_L(value, value_len, "false")) {
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
        case OPT_TYPE_THEME:
        {
            const Theme *theme;

            if (NULL == (theme = theme_by_name(value))) {
                return OPT_ERR_UNKNOWN_THEME;
            } else {
                OPT_THEMPTR(*optvalptr) = theme;
                return OPT_SUCCESS;
            }
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
