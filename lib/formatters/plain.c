#include <stddef.h>

#include "cpp.h"
#include "tokens.h"
#include "formatter.h"

typedef struct {
    int nolexing ALIGNED(sizeof(OptionValue));
} PlainFormatterData;

#if 0
static int start_document(String *out, FormatterData *data)
{
    STRING_APPEND_STRING(out, "--BOS--");

    return 0;
}

static int end_document(String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "--EOS--");

    return 0;
}
#endif

static int start_token(int token, String *out, FormatterData *UNUSED(data))
{
    string_append_string_len(out, tokens[token].name, tokens[token].name_len);
    STRING_APPEND_STRING(out, ": ");

    return 0;
}

static int end_token(int UNUSED(token), String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "\n");

    return 0;
}

static int write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len_dump(out, token, token_len);

    return 0;
}

static int start_lexing(const char *lexer, String *out, FormatterData *data)
{
    PlainFormatterData *mydata;

    mydata = (PlainFormatterData *) data;
    if (!mydata->nolexing) {
        STRING_APPEND_STRING(out, "===== ");
        string_append_string(out, lexer);
        STRING_APPEND_STRING(out, " =====\n");
    }

    return 0;
}

static int end_lexing(const char *lexer, String *out, FormatterData *data)
{
    PlainFormatterData *mydata;

    mydata = (PlainFormatterData *) data;
    if (!mydata->nolexing) {
        STRING_APPEND_STRING(out, "===== /");
        string_append_string(out, lexer);
        STRING_APPEND_STRING(out, " =====\n");
    }

    return 0;
}

static const FormatterImplementation _plainfmt = {
    "Plain",
    "Format tokens in plain text, mostly instended for tests. Each token is written on a new line with the form: <token name>: <token value>",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    NULL,
    NULL,
    start_token,
    end_token,
    write_token,
    start_lexing,
    end_lexing,
    sizeof(PlainFormatterData),
    (/*const*/ FormatterOption /*const*/ []) {
        { S("nolexing"), OPT_TYPE_BOOL,  offsetof(PlainFormatterData, nolexing), OPT_DEF_BOOL(1), "if set to false, mention, in output, lexer switches" },
        END_OF_FORMATTER_OPTIONS
    }
};

/*SHALL_API */const FormatterImplementation *plainfmt = &_plainfmt;
