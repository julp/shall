#include <stddef.h>

#include "cpp.h"
#include "tokens.h"
#include "formatter.h"

typedef struct {
    OptionValue linestart;
    OptionValue cssclass;
} HTMLFormatterData;

static const char * const map[] = {
#define TOKEN(constant, description, cssclass) \
    cssclass,
#include "keywords.h"
#undef TOKEN
};

#define STRING_APPEND_STRING(string, suffix) \
    do { \
        string_append_string_len(string, suffix, STR_LEN(suffix)); \
    } while (0);

static int html_start_document(String *out, FormatterData *data)
{
    HTMLFormatterData *mydata;

    mydata = (HTMLFormatterData *) data;
    if (NULL == OPT_STRVAL(mydata->cssclass) || 0 == OPT_STRLEN(mydata->cssclass)) {
        STRING_APPEND_STRING(out, "<pre>");
    } else {
        STRING_APPEND_STRING(out, "<pre class=\"");
        string_append_string(out, OPT_STRVAL(mydata->cssclass));

        STRING_APPEND_STRING(out, "\">");
    }

    return 0;
}

static int html_end_document(String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "</pre>");

    return 0;
}

static int html_start_token(int token, String *out, FormatterData *UNUSED(data))
{
    if (IGNORABLE != token) {
        if ('\0' == map[token][1]) {
            char buffer[] = "<span class=\"X\">";

            buffer[13] = map[token][0];
            STRING_APPEND_STRING(out, buffer);
        } else {
            char buffer[] = "<span class=\"XX\">";

            buffer[13] = map[token][0];
            buffer[14] = map[token][1];
            STRING_APPEND_STRING(out, buffer);
        }
    }

    return 0;
}

static int html_end_token(int token, String *out, FormatterData *UNUSED(data))
{
    if (IGNORABLE != token) {
        STRING_APPEND_STRING(out, "</span>");
    }

    return 0;
}

static int html_write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_xml_len(out, token, token_len);

    return 0;
}

static int html_start_lexing(const char *lexname, String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "<SPAN class=\"");
    string_append_string(out, lexname);
    STRING_APPEND_STRING(out, "\">");

    return 0;
}

static int html_end_lexing(const char *UNUSED(lexname), String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "</SPAN>");

    return 0;
}

const FormatterImplementation _htmlfmt = {
    "HTML",
    "Format tokens as HTML 4 <span> tags within a <pre> tag",
    formatter_implementation_default_get_option_ptr,
    html_start_document,
    html_end_document,
    html_start_token,
    html_end_token,
    html_write_token,
    html_start_lexing,
    html_end_lexing,
    sizeof(HTMLFormatterData),
    (/*const*/ FormatterOption /*const*/ []) {
        { S("linestart"), OPT_TYPE_INT, offsetof(HTMLFormatterData, linestart), OPT_DEF_INT(1), "The line number for the first line" },
        { S("cssclass"), OPT_TYPE_STRING, offsetof(HTMLFormatterData, cssclass), OPT_DEF_STRING(""), "if valued to `foo`, ` class=\"foo\"` is added to `<pre>` tag" },
        END_OF_FORMATTER_OPTIONS
    }
};

/*SHALL_API*/ const FormatterImplementation *htmlfmt = &_htmlfmt;
