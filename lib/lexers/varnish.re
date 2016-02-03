#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

extern const LexerImplementation c_lexer;

enum {
    STATE(INITIAL),
    STATE(IN_C),
    STATE(IN_COMMENT),
    STATE(IN_STRING),
    STATE(IN_LONG_STRING),
};

static int default_token_type[] = {
    IGNORABLE,         // INITIAL
    IGNORABLE,         // IN_C
    COMMENT_MULTILINE, // IN_COMMENT
    STRING_SINGLE,     // IN_STRING
    STRING_SINGLE,     // IN_LONG_STRING
};

static int varnishanalyse(const char *src, size_t src_len)
{
    if (src_len >= STR_LEN("vcl") && 0 == memcmp(src, "vcl", STR_LEN("vcl"))) {
        return 100;
    }

    return 0;
}

#define Um 0  /* undefined minimal version */
#define UM 99 /* undefined maximal version */
typedef struct {
    int starts_with;
    // [min_version;max_version[
    int min_version;
    int max_version;
    const char *name;
    size_t name_len;
} varnish_named_element_t;

typedef struct {
    LexerData data;
    int version;
} VarnishLexerData;

typedef struct {
    int version ALIGNED(sizeof(OptionValue));
} VarnishLexerOption;

static void varnishinit(const OptionValue *options, LexerData *data, void *UNUSED(ctxt))
{
    VarnishLexerData *mydata;
    const VarnishLexerOption *myoptions;

    mydata = (VarnishLexerData *) data;
    myoptions = (const VarnishLexerOption *) options;
    mydata->version = myoptions->version;
}

#define VAR(s) \
    { 1, Um, UM, s, STR_LEN(s) },

#define ELT(s) \
    { 0, Um, UM, s, STR_LEN(s) },

#define ELT3(s) \
    { 0, Um, 3, s, STR_LEN(s) },

#define ELT4(s) \
    { 0, 4, UM, s, STR_LEN(s) },

static const varnish_named_element_t functions[] = {
    ELT("ban")
    ELT3("ban_url")
    ELT("call")
    ELT3("error")
    ELT("hash_data")
    ELT4("new")
    ELT("regsub")
    ELT("regsuball")
    ELT4("return")
    ELT4("rollback")
    ELT3("std.author")
    ELT4("std.cache_req_body")
    ELT("std.collect")
    ELT("std.duration")
    ELT("std.fileread")
    ELT4("std.healthy")
    ELT("std.integer")
    ELT4("std.ip")
    ELT("std.log")
    ELT4("std.port")
    ELT4("std.querysort")
    ELT("std.random")
    ELT4("std.real")
    ELT4("std.real2time")
    ELT4("std.rollback")
    ELT("std.set_ip_tos")
    ELT4("std.strstr")
    ELT("std.syslog")
    ELT4("std.time")
    ELT4("std.time2integer")
    ELT4("std.time2real")
    ELT4("std.timestamp")
    ELT("std.tolower")
    ELT("std.toupper")
    ELT4("synth")
    ELT4("synthetic")
};

// TODO:
// - distinguish client from server variables?
// - regrouper variables et functions en ajoutant un membre token_type ?
// sed -E 's#^(ELT[34]?|VAR)\("([^"]*)"\)#\0 \2#' sort.txt | sort -k 2 | cut -d " " -f 1
static const varnish_named_element_t variables[] = {
    ELT("bereq")
    ELT("bereq.backend")
    ELT("bereq.between_bytes_timeout")
    ELT("bereq.connect_timeout")
    ELT("bereq.first_byte_timeout")
    VAR("bereq.http.")
    ELT4("bereq.method")
    ELT("bereq.proto")
    ELT3("bereq.request")
    ELT("bereq.retries")
    ELT4("bereq.uncacheable")
    ELT("bereq.url")
    ELT("bereq.xid")
    ELT("beresp")
    ELT4("beresp.age")
    ELT4("beresp.backend")
    ELT("beresp.backend.ip")
    ELT("beresp.backend.name")
    ELT3("beresp.backend.port")
    ELT("beresp.do_esi")
    ELT("beresp.do_gunzip")
    ELT("beresp.do_gzip")
    ELT("beresp.do_stream")
    ELT("beresp.grace")
    VAR("beresp.http.")
    ELT("beresp.keep")
    ELT("beresp.proto")
    ELT("beresp.reason")
    ELT3("beresp.response")
    ELT3("beresp.saintmode")
    ELT("beresp.status")
    ELT3("beresp.storage")
    ELT4("beresp.storage_hint")
    ELT("beresp.ttl")
    ELT4("beresp.uncacheable")
    ELT4("beresp.was_304")
    ELT("client.identity")
    ELT("client.ip")
    ELT3("client.port")
    ELT("local.ip")
    ELT("now")
    ELT4("obj.age")
    ELT("obj.grace")
    ELT("obj.hits")
    VAR("obj.http.")
    ELT("obj.keep")
    ELT3("obj.lastuse")
    ELT("obj.proto")
    ELT("obj.reason")
    ELT3("obj.response")
    ELT("obj.status")
    ELT("obj.ttl")
    ELT4("obj.uncacheable")
    ELT4("remote.ip")
    ELT("req")
    ELT3("req.backend")
    ELT3("req.backend.healthy")
    ELT4("req.backend_hint")
    ELT("req.can_gzip")
    ELT("req.esi")
    ELT("req.esi_level")
    ELT3("req.grace")
    ELT("req.hash_always_miss")
    ELT("req.hash_ignore_busy")
    VAR("req.http.")
    ELT3("req.keep")
    ELT4("req.method")
    ELT("req.proto")
    ELT3("req.request")
    ELT("req.restarts")
    ELT("req.ttl")
    ELT("req.url")
    ELT("req.xid")
    ELT("resp")
    VAR("resp.http.")
    ELT("resp.proto")
    ELT4("resp.reason")
    ELT3("resp.response")
    ELT("resp.status")
    ELT("server.hostname")
    ELT("server.identity")
    ELT("server.ip")
    ELT3("server.port")
};

#define VCL(s) \
    { 0, Um, UM, "vcl_" s, STR_LEN("vcl_" s) },

#define VCL3(s) \
    { 0, Um, 3, "vcl_" s, STR_LEN("vcl_" s) },

#define VCL4(s) \
    { 0, 4, UM, "vcl_" s, STR_LEN("vcl_" s) },

static const varnish_named_element_t subroutines[] = {
    VCL3("fetch")
    VCL4("backend_error")
    VCL4("backend_fetch")
    VCL4("backend_response")
    VCL4("purge")
    VCL("deliver")
    VCL("fini")
    VCL("hash")
    VCL("hit")
    VCL("init")
    VCL("miss")
    VCL("pass")
    VCL("pipe")
    VCL("recv")
    VCL("synth")
};

static int varnish_named_elements_cmp(const void *a, const void *b)
{
    const varnish_named_element_t *na, *nb;

    na = (const varnish_named_element_t *) a; /* key */
    nb = (const varnish_named_element_t *) b;
    if (nb->starts_with) {
        return strncmp_l(na->name, na->name_len, nb->name, nb->name_len, nb->name_len);
    } else {
        return strcmp_l(na->name, na->name_len, nb->name, nb->name_len);
    }
}

static int varnishlex(YYLEX_ARGS)
{
    VarnishLexerData *mydata;

    (void) options;
    mydata = (VarnishLexerData *) data;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
DNUM = ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*);
SPACE = [ \f\n\r\t\v]+;

//IDENT1 = [a-zA-Z];
//IDENT = IDENT1 | [0-9_-];
//VAR = IDENT | '.';

// TODO:
// backend/probe/acl/director
// fonctions (return, synth, etc)
// inline C
// return action/keyword?

<INITIAL> (LNUM | DNUM) ("ms" | [smhdwy]) {
    TOKEN(LITERAL_DURATION);
}

<INITIAL> LNUM [KMGT]? 'B' {
    TOKEN(LITERAL_SIZE);
}

<INITIAL> "/*" {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT> "*/" {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> "//" | "#" {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                //CG(zend_lineno)++;
                break;
            default:
                continue;
        }
        break;
    }
    TOKEN(COMMENT_SINGLE);
}

// vcl statement really expects a float
<INITIAL> "vcl" SPACE+ DNUM {
    // TODO: better
    // NOTE: don't use strtod to parse version number as it depends on locale for decimal separator
    yyless(STR_LEN("vcl"));
    mydata->version = 4;
    TOKEN(KEYWORD_DECLARATION);
}

<INITIAL> "sub" {
    data->next_label = FUNCTION;
    TOKEN(KEYWORD);
}

<INITIAL> "remove" {
    if (mydata->version < 4) {
        TOKEN(KEYWORD);
    } else {
        TOKEN(IGNORABLE);
    }
}

<INITIAL> "set" | "unset" | "include" | "import" | "if" | "else" | "elseif" | "elif" | "elsif" {
    TOKEN(KEYWORD);
}

<INITIAL> "true" | "false" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL>"C{" {
    BEGIN(IN_C);
#if 0
    TOKEN(NAME_TAG);
#else
    YYCTYPE *end;

    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "}C", STR_LEN("}C"), (const char *) YYLIMIT))) {
        end = YYLIMIT;
    } else {
        end -= STR_LEN("}C");
    }
    prepend_lexer_implementation(ctxt, &c_lexer);
    DELEGATE_UNTIL_AFTER_TOKEN(end, IGNORABLE, NAME_TAG);
#endif
}

<IN_C>"}C" {
    BEGIN(INITIAL);
    unprepend_lexer(ctxt, &c_lexer);
    TOKEN(NAME_TAG);
}

<IN_C>[^] {
#if 0
    YYCTYPE *end;

    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "}C", STR_LEN("}C"), (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end;
    }
    prepend_lexer_implementation(ctxt, &c_lexer);
    DELEGATE_UNTIL(IGNORABLE);
#else
    // TODO: remove the rule, already handled by <*>[^]
    TOKEN(IGNORABLE);
#endif
}

<INITIAL> [a-zA-Z_.-]+ {
    size_t i;
    int type;
    int next_label;

    type = IGNORABLE;
    next_label = data->next_label;
    data->next_label = 0;
    if (FUNCTION == next_label) {
        for (i = 0; i < ARRAY_SIZE(subroutines); i++) {
            if (0 == strcmp_l(subroutines[i].name, subroutines[i].name_len, (char *) YYTEXT, YYLENG)) {
                if (mydata->version >= subroutines[i].min_version && mydata->version < subroutines[i].max_version) {
                    type = NAME_FUNCTION;
                }/* else {
                    type = IGNORABLE;
                }*/
                break;
            }
        }
    } else {
        {
            varnish_named_element_t *match, key = { 0, 0, 0, (char *) YYTEXT, YYLENG };

            if (NULL != (match = bsearch(&key, variables, ARRAY_SIZE(variables), sizeof(variables[0]), varnish_named_elements_cmp))) {
                if (mydata->version >= match->min_version && mydata->version < match->max_version) {
                    type = NAME_VARIABLE;
                }/* else {
                    type = IGNORABLE;
                }*/
            }
        }
        for (i = 0; i < ARRAY_SIZE(functions); i++) {
            if (0 == strcmp_l(functions[i].name, functions[i].name_len, (char *) YYTEXT, YYLENG)) {
                type = NAME_FUNCTION;
                break;
            }
        }
    }
    TOKEN(type);
}

<INITIAL> [{}();.,] {
    TOKEN(PUNCTUATION);
}

<INITIAL> "++" | "--" | "&&" | "||" | [<=>!*/+-]"=" | "<<" | ">>" | "!~" | [-+*/%><=!&|~] {
    TOKEN(OPERATOR);
}

<INITIAL> LNUM {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> DNUM {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> '{"' {
    BEGIN(IN_LONG_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_LONG_STRING> '"}' {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<INITIAL> '"' {
    BEGIN(IN_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_STRING> '\\'[\\"nt] {
    TOKEN(ESCAPED_CHAR);
}

<IN_STRING> '"' {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation varnish_lexer = {
    "Varnish",
    "A lexer for Varnish configuration language",
    (const char * const []) { "varnishconf", "VCL", NULL },
    (const char * const []) { "*.vcl", NULL },
    (const char * const []) { "text/x-varnish", NULL },
    NULL, // interpreters
    varnishanalyse,
    varnishinit,
    varnishlex,
    NULL, // finalize
    sizeof(VarnishLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { S("version"), OPT_TYPE_INT, offsetof(VarnishLexerOption, version), OPT_DEF_INT(3), "VCL version, default is 3 if `vcl` statement is absent" },
        END_OF_OPTIONS
    },
    (const LexerImplementation * const []) { &c_lexer, NULL }
};
