#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "cpp.h"
#include "tokens.h"
#include "option.h"
#include "shall.h"
#include "xtring.h"

#ifndef EUSAGE
# define EUSAGE -2
#endif /* !EUSAGE */

#ifdef _MSC_VER
extern char __progname[];
#else
extern char *__progname;
#endif /* _MSC_VER */

enum {
    LEXER,
    FORMATTER,
    COUNT
};

enum {
    MARKDOWN,
    RDOC
};

typedef void (*implementation_each)(void (*)(const void *, void *), void *);
typedef const char *(*implementation_name)(const void *);
typedef const char *(*implementation_description)(const void *);
typedef void (*implementation_alias_each)(const void *, void (*)(const char *, void *), void *);
typedef void (*implementation_filename_each)(const void *, void (*)(const char *, void *), void *);
typedef void (*implementation_mimetype_each)(const void *, void (*)(const char *, void *), void *);
typedef void (*implementation_option_each)(const void *, void (*)(int, const char *, OptionValue, const char *, void *), void *);

static struct {
    const char *title;
    implementation_each each;
    implementation_name name;
    implementation_description description;
    implementation_alias_each each_alias;
    implementation_filename_each each_filename;
    implementation_mimetype_each each_mimetype;
    implementation_option_each each_option;
} callbacks[] = {
    [ LEXER ] = {
        "Lexer",
        (implementation_each) lexer_implementation_each,
        (implementation_name) lexer_implementation_name,
        (implementation_description) lexer_implementation_description,
        (implementation_alias_each) lexer_implementation_each_alias,
        (implementation_filename_each) lexer_implementation_each_filename,
        (implementation_mimetype_each) lexer_implementation_each_mimetype,
        (implementation_option_each) lexer_implementation_each_option,
    },
    [ FORMATTER ] = {
        "Formatter",
        (implementation_each) formatter_implementation_each,
        (implementation_name) formatter_implementation_name,
        (implementation_description) formatter_implementation_description,
        NULL,
        NULL,
        NULL,
        (implementation_option_each) formatter_implementation_each_option,
    }
};

static char optstr[] = "flr";

static struct option long_options[] = {
    { "rdoc",       no_argument, NULL, 'r' },
    { "lexers",     no_argument, NULL, 'l' },
    { "formatters", no_argument, NULL, 'f' },
    { NULL,         no_argument, NULL, 0   }
};

static void usage(void)
{
    fprintf(
        stderr,
        "usage: %s [-%s]\n",
        __progname,
        optstr
    );
    exit(EUSAGE);
}

static const char *subheading;
static int type, printed, indent;

static void list_cb(const char *name, void *fp)
{
    if (0 == printed) {
//         fprintf(fp, "%s: \n\n", subheading);
        fprintf(fp, "%s: ", subheading);
        fprintf(fp, "%s", name);
    } else {
        fprintf(fp, ", %s", name);
    }
//     fprintf(fp, "%s\n", name);
    ++printed;
}

static const char *type2string[] = {
    [ OPT_TYPE_INT ]    = "int",
    [ OPT_TYPE_BOOL ]   = "boolean",
    [ OPT_TYPE_STRING ] = "string",
    [ OPT_TYPE_LEXER ]  = "lexer"
};

static void option_cb(int opt_type, const char *name, OptionValue defval, const char *docstr, void *fp)
{
    if (0 == printed) {
        fputs("| Option | Type | Default value | Description |\n", fp);
        fputs("| ------ | ---- | ------------- | ----------- |\n", fp);
    }
    fprintf(fp, "| %s | %s | ", name, type2string[opt_type]);
    switch (opt_type) {
        case OPT_TYPE_BOOL:
        {
            static const char *map[] = { "false", "true" };

            fputs(map[!!OPT_GET_BOOL(defval)], fp);
            break;
        }
        case OPT_TYPE_INT:
            fprintf(fp, "%d", OPT_GET_INT(defval));
            break;
        case OPT_TYPE_STRING:
            if (0 == OPT_STRVAL(defval)) {
                fputs("null/none", fp);
            } else {
                fprintf(fp, "\"%s\"", OPT_STRVAL(defval));
            }
            break;
        case OPT_TYPE_LEXER:
        {
            if (NULL == OPT_LEXPTR(defval)) {
                fputs("null/none", fp);
            } else {
                fputs(lexer_implementation_name(lexer_implementation(lexer_unwrap(defval))), fp);
            }
            break;
        }
    }
    fprintf(fp, " | %s |\n", docstr);
    ++printed;
}

static void markdown_implementation_cb(const void *imp, void *fp)
{
    const char *desc;

    fprintf(fp, "## %s\n\n", callbacks[type].name(imp));

    if (NULL != (desc = callbacks[type].description(imp))) {
        fputs(desc, fp);
        fputs("\n\n", fp);
    }

    if (NULL != callbacks[type].each_alias) {
        printed = 0;
        subheading = "Alias(es)";
        callbacks[type].each_alias(imp, list_cb, fp);
        if (printed > 0) {
            fputs("\n\n", fp);
        }
    }

    if (NULL != callbacks[type].each_filename) {
        printed = 0;
        subheading = "Filename(s)";
        callbacks[type].each_filename(imp, list_cb, fp);
        if (printed > 0) {
            fputs("\n\n", fp);
        }
    }

    if (NULL != callbacks[type].each_mimetype) {
        printed = 0;
        subheading = "MIME type(s)";
        callbacks[type].each_mimetype(imp, list_cb, fp);
        if (printed > 0) {
            fputs("\n\n", fp);
        }
    }

    if (NULL != callbacks[type].each_option) {
        printed = 0;
        subheading = "Options";
        callbacks[type].each_option(imp, option_cb, fp);
        if (printed > 0) {
            fputs("\n", fp);
        }
    }
}

static void generate_markdown(FILE *fp)
{
    fprintf(fp, "# %ss\n\n", callbacks[type].title);
    fputs("NOTE: this file is generated by shalldoc, do not edit it\n\n", fp);
    callbacks[type].each(markdown_implementation_cb, fp);
}

static void rdoc_implementation_cb(const void *imp, void *fp)
{
    fprintf(fp, "%*c# %s\n%*cclass %s < Base ; end\n", indent * 4, ' ', callbacks[type].description(imp), indent * 4, ' ', callbacks[type].name(imp));
}

static void generate_rdoc(FILE *fp)
{
    fputs("module Shall\n", fp);
    ++indent;
    for (type = 0; type < COUNT; type++) {
        fprintf(fp, "%*cmodule %s\n", indent * 4, ' ', callbacks[type].title);
        ++indent;
        callbacks[type].each(rdoc_implementation_cb, fp);
        --indent;
        fprintf(fp, "%*cend\n", indent * 4, ' ');
    }
    fprintf(fp, "%*cmodule Token\n", indent * 4, ' ');
    ++indent;
#define TOKEN(constant, description, cssclass) \
    do { \
        fprintf(fp, "%*c# %s\n", indent * 4, ' ', description); \
        fprintf(fp, "%*c%s = %d\n", indent * 4, ' ', #constant, constant); \
    } while (0);
#include "keywords.h"
#undef TOKEN
    --indent;
    fprintf(fp, "%*cend\n", indent * 4, ' ');
    fputs("end\n", fp);
}

static void (*format_callbacks[])(FILE *fp) = {
    [ MARKDOWN ] = generate_markdown,
    [ RDOC ] = generate_rdoc
};

int main(int argc, char **argv)
{
    FILE *fp;
    int o, format;

    format = type = -1;
    while (-1 != (o = getopt_long(argc, argv, optstr, long_options, NULL))) {
        switch (o) {
            case 'l':
                type = LEXER;
                format = MARKDOWN;
                break;
            case 'f':
                type = FORMATTER;
                format = MARKDOWN;
                break;
            case 'r':
//                 type = LEXER | FORMATTER;
                format = RDOC;
                break;
            default:
                usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (format < 0) {
        usage();
    }
    if (1 != argc) {
        usage();
    }
    if (NULL == (fp = fopen(argv[0], "w"))) {
        return EXIT_FAILURE;
    }
    format_callbacks[format](fp);
    fclose(fp);

    return EXIT_SUCCESS;
}
