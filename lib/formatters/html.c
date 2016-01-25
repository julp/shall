#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "themes.h"
#include "formatter.h"

typedef struct {
    OptionValue title;
    OptionValue linestart;
    OptionValue cssclass;
    int full ALIGNED(sizeof(OptionValue));
    int nowrap ALIGNED(sizeof(OptionValue));
    int noclasses ALIGNED(sizeof(OptionValue));
    const Theme *theme ALIGNED(sizeof(OptionValue));
    struct {
        size_t len;
        const char *val;
    } open_span_tag[_TOKEN_COUNT];
} HTMLFormatterData;

#define NL "\n"
#define INDENT "  "

static int html_start_document(String *out, FormatterData *data)
{
    HTMLFormatterData *mydata;
    const Theme *theme;

    mydata = (HTMLFormatterData *) data;
    // TODO: define a default theme in shall itself
    if (NULL == (theme = mydata->theme)) {
        theme = theme_by_name("molokai");
    }
    if (0 != mydata->full) {
        if (5 == mydata->full) {
            STRING_APPEND_STRING(out, "<!DOCTYPE html>")
        } else {
            STRING_APPEND_STRING(out, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">")
        }
        STRING_APPEND_STRING(out, NL "<html>"
            NL INDENT "<head>"
            NL INDENT INDENT "<title>");
        if (NULL != OPT_STRVAL(mydata->title) && OPT_STRLEN(mydata->title) > 0) {
            string_append_xml_len(out, OPT_STRVAL(mydata->title), OPT_STRLEN(mydata->title));
        }
        STRING_APPEND_STRING(out, "</title>"
            NL INDENT INDENT);
        if (5 == mydata->full) {
            STRING_APPEND_STRING(out, "<meta charset=\"utf-8\">");
        } else {
            STRING_APPEND_STRING(out, "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">");
        }
        STRING_APPEND_STRING(out, NL INDENT INDENT "<style type=\"text/css\">");
        if (!mydata->noclasses) {
            char *css;

            css = theme_export_as_css(theme, NULL, true);
            string_append_string(out, css);
            free(css);
        }
        STRING_APPEND_STRING(out, NL INDENT INDENT "</style>"
            NL INDENT "</head>"
            NL INDENT "<body>"
            NL INDENT INDENT);
    }
    if (!mydata->nowrap) {
        if (NULL == OPT_STRVAL(mydata->cssclass) || 0 == OPT_STRLEN(mydata->cssclass)) {
            STRING_APPEND_STRING(out, "<pre>");
        } else {
            STRING_APPEND_STRING(out, "<pre class=\"");
            string_append_string(out, OPT_STRVAL(mydata->cssclass)); // TODO: escaping ('"' + '&')
            STRING_APPEND_STRING(out, "\">");
        }
    }
    if (mydata->noclasses) {
        size_t i;
        String *buffer;

        buffer = string_new();
        for (i = 0; i < _TOKEN_COUNT; i++) {
            mydata->open_span_tag[i].len = 0;
            mydata->open_span_tag[i].val = NULL;
            if (theme->styles[i].flags) {
                string_truncate(buffer);
                STRING_APPEND_STRING(buffer, "<span style=\"");
                if (theme->styles[i].bold) {
                    STRING_APPEND_STRING(buffer, "font-weight: bold;");
                }
                if (theme->styles[i].italic) {
                    STRING_APPEND_STRING(buffer, "font-style: italic;");
                }
                if (theme->styles[i].underline) {
                    STRING_APPEND_STRING(buffer, "text-decoration: underline;");
                }
                if (theme->styles[i].fg_set) {
                    STRING_APPEND_COLOR(buffer, "color: ", theme->styles[i].fg, ";");
                }
                if (theme->styles[i].bg_set) {
                    STRING_APPEND_COLOR(buffer, "background-color: ", theme->styles[i].bg, ";");
                }
                STRING_APPEND_STRING(buffer, "\">");

                mydata->open_span_tag[i].val = strndup(buffer->ptr, buffer->len);
                mydata->open_span_tag[i].len = buffer->len;
            }
        }
        string_destroy(buffer);
    }

    return 0;
}

static int html_end_document(String *out, FormatterData *data)
{
    HTMLFormatterData *mydata;

    mydata = (HTMLFormatterData *) data;
    if (!mydata->nowrap) {
        STRING_APPEND_STRING(out, "</pre>");
    }
    if (0 != mydata->full) {
        STRING_APPEND_STRING(out, NL INDENT "</body>" NL "</html>");
    }
    if (mydata->noclasses) {
        size_t i;

        for (i = 0; i < _TOKEN_COUNT; i++) {
            if (mydata->open_span_tag[i].len > 0) {
                free((void *) mydata->open_span_tag[i].val);
            }
        }
    }

    return 0;
}

static int html_start_token(int token, String *out, FormatterData *data)
{
    if (IGNORABLE != token) {
        HTMLFormatterData *mydata;

        mydata = (HTMLFormatterData *) data;
        if (mydata->noclasses) {
            if (mydata->open_span_tag[token].len > 0) {
                string_append_string_len(out, mydata->open_span_tag[token].val, mydata->open_span_tag[token].len);
            }
        } else {
            if ('\0' == tokens[token].cssclass[1]) {
                char buffer[] = "<span class=\"X\">";

                buffer[13] = tokens[token].cssclass[0];
                STRING_APPEND_STRING(out, buffer);
            } else {
                char buffer[] = "<span class=\"XX\">";

                buffer[13] = tokens[token].cssclass[0];
                buffer[14] = tokens[token].cssclass[1];
                STRING_APPEND_STRING(out, buffer);
            }
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

// NOTE: for debugging and testing, span tag is voluntarily in uppercase to be easier to identify
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
    "Format tokens as HTML <span> tags within a <pre> tag",
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
        { S("full"),      OPT_TYPE_INT,    offsetof(HTMLFormatterData, full),      OPT_DEF_INT(0),     "if not 0, embeds generated output in a whole HTML 4 page (use 5 for a HTML 5 document)" },
        { S("title"),     OPT_TYPE_STRING, offsetof(HTMLFormatterData, title),     OPT_DEF_STRING(""), "when *full* is not 0, this is the content to use as `<title>` for the full HTML output document (its content is escaped)" },
        { S("nowrap"),    OPT_TYPE_BOOL,   offsetof(HTMLFormatterData, nowrap),    OPT_DEF_BOOL(0),    "when set to true, don't wrap output within a `<pre>` tag" },
        { S("noclasses"), OPT_TYPE_BOOL,   offsetof(HTMLFormatterData, noclasses), OPT_DEF_BOOL(0),    "when set to true (not recommanded), output `<span>` tags will not use CSS classes, but inline styles" },
        { S("theme"),     OPT_TYPE_THEME,  offsetof(HTMLFormatterData, theme),     OPT_DEF_THEME,      "the theme to use" },
        { S("cssclass"),  OPT_TYPE_STRING, offsetof(HTMLFormatterData, cssclass),  OPT_DEF_STRING(""), "if valued to `foo`, ` class=\"foo\"` is added to `<pre>` tag" },
        { S("linestart"), OPT_TYPE_INT,    offsetof(HTMLFormatterData, linestart), OPT_DEF_INT(1),     "the line number for the first line" },
        END_OF_FORMATTER_OPTIONS
    }
};

/*SHALL_API*/ const FormatterImplementation *htmlfmt = &_htmlfmt;
