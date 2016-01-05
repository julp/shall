#include <stddef.h>

#include "tokens.h"
#include "formatter.h"

static const char * const map[] = {
#define TOKEN(constant, description, cssclass) \
    cssclass,
#include "keywords.h"
#undef TOKEN
};

static int xml_start_document(String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "<shall>");

    return 0;
}
static int xml_end_document(String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "</shall>");

    return 0;
}

static int start_token(int token, String *out, FormatterData *UNUSED(data))
{
    char buffer[] = "<\"XX\">";

    buffer[2] = map[token][0];
    buffer[3] = map[token][1];
    STRING_APPEND_STRING(out, buffer);

    return 0;
}

static int end_token(int token, String *out, FormatterData *UNUSED(data))
{
    char buffer[] = "</\"XX\">";

    buffer[3] = map[token][0];
    buffer[4] = map[token][1];
    STRING_APPEND_STRING(out, buffer);

    return 0;
}

static int write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_xml_len(out, token, token_len);

    return 0;
}

const FormatterImplementation _xmlfmt = {
    "XML",
    "Format tokens as a XML tree",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    xml_start_document,
    xml_end_document,
    start_token,
    end_token,
    write_token,
    sizeof(FormatterData),
    NULL
};

/*SHALL_API*/ const FormatterImplementation *xmlfmt = &_xmlfmt;
