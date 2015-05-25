#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "cpp.h"
#include "option.h"
#include "optparse.h"
#include "shall.h"
#include "xtring.h"
#include "hashtable.h"

#ifndef EUSAGE
# define EUSAGE -2
#endif /* !EUSAGE */

#ifdef _MSC_VER
extern char __progname[];
#else
extern char *__progname;
#endif /* _MSC_VER */

static HashTable lexers;
static char optstr[] = "f:l:o:LO:";

static struct option long_options[] = {
    { "list",             required_argument, NULL, 'L' },
    { "lexer",            required_argument, NULL, 'l' },
    { "formatter",        required_argument, NULL, 'f' },
    { "lexer-option",     required_argument, NULL, 'o' },
    { "formatter-option", required_argument, NULL, 'O' },
    { NULL,               no_argument,       NULL, 0   }
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

static void procfile(const char *filename, Lexer *default_lexer, Formatter *fmt, Options *lexer_options)
{
    size_t o;
    ht_hash_t h;
    char *result;
    Lexer *lexer;
    String *buffer;
    const char *imp_name;

    buffer = string_new();
    {
        FILE *fp;
        size_t read;
        char bufraw[1024];

        if (0 == strcmp(filename, "-")) {
            fp = stdin;
        } else {
            if (NULL == (fp = fopen(filename, "r"))) {
                fprintf(stderr, "unable to open '%s', skipping\n", filename);
                return;
            }
        }
        do {
            read = fread(bufraw, sizeof(bufraw[0]), ARRAY_SIZE(bufraw), fp);
            string_append_string_len(buffer, bufraw, read);
        } while (ARRAY_SIZE(bufraw) == read);
        if (stdin != fp) {
            fclose(fp);
        }
    }
    if (NULL == default_lexer) {
        const LexerImplementation *limp;

        if (NULL == (limp = lexer_implementation_for_filename(filename))) {
            if (NULL == (limp = lexer_implementation_guess(buffer->ptr, buffer->len))) {
                fprintf(stderr, "no suitable lexer found for '%s', skipping\n", filename);
                return;
            }
        }
        imp_name = lexer_implementation_name(limp);
        h = hashtable_hash(&lexers, imp_name);
        if (!hashtable_quick_get(&lexers, h, imp_name, &lexer)) {
            lexer = lexer_create(limp);
            for (o = 0; o < lexer_options->options_len; o++) {
                if (0 != lexer_set_option_as_string(lexer, lexer_options->options[o].name, lexer_options->options[o].value, lexer_options->options[o].value_len)) {
                    fprintf(stderr, "option '%s' rejected by %s lexer\n", lexer_options->options[o].name, lexer_implementation_name(lexer_implementation(lexer)));
                }
            }
            hashtable_quick_put(&lexers, 0, h, imp_name, lexer, NULL);
        }
    } else {
        lexer = default_lexer;
    }
    highlight_string(lexer, fmt, buffer->ptr, &result);
    // print result
    puts(result);
    // free
    string_destroy(buffer);
    free(result);
}

static const char *type2string[] = {
    [ OPT_TYPE_INT ]    = "int",
    [ OPT_TYPE_BOOL ]   = "boolean",
    [ OPT_TYPE_STRING ] = "string",
    [ OPT_TYPE_LEXER ]  = "lexer"
};

static void print_lexer_option_cb(int type, const char *name, OptionValue defval, const char *docstr, void *UNUSED(data))
{
    printf("  + %s (%s, default: ", name, type2string[type]);
    switch (type) {
        case OPT_TYPE_BOOL:
        {
            static const char *map[] = { "false", "true" };

//             printf("%s", map[OPT_GET_BOOL(defval)]);
            fputs(map[!!OPT_GET_BOOL(defval)], stdout);
            break;
        }
        case OPT_TYPE_INT:
            printf("%d", OPT_GET_INT(defval));
            break;
        case OPT_TYPE_STRING:
//             printf("%s", OPT_STRVAL(defval));
            fputs(OPT_STRVAL(defval), stdout);
            break;
        case OPT_TYPE_LEXER:
//             if (0 == defval) {
//                 printf("null/none");
                fputs("null/none", stdout);
//             } else {
//                 printf("%s", lexer_implementation_name(lexer_implementation((Lexer *) defval)));
//                 fputs(lexer_implementation_name(lexer_implementation((Lexer *) defval)), stdout);
//             }
            break;
    }
    printf("): %s\n", docstr);
}

static void print_lexer_cb(const LexerImplementation *imp, void *UNUSED(data))
{
    const char *imp_name;

    imp_name = lexer_implementation_name(imp);
    printf("- %s\n", imp_name);
    lexer_implementation_each_option(imp, print_lexer_option_cb, NULL);
}

static void destroy_lexer_cb(void *ptr)
{
    Lexer *lexer;

    lexer = (Lexer *) ptr;
    lexer_destroy(lexer, (on_lexer_destroy_cb_t) lexer_destroy);
}

int main(int argc, char **argv)
{
    int o;
    size_t i;
    Lexer *lexer;
    Formatter *fmt;
    Options options[COUNT];
    const LexerImplementation *limp;
    const FormatterImplementation *fimp;

    fmt = NULL;
    lexer = NULL;
    limp = NULL;
    fimp = termfmt;
    hashtable_ascii_cs_init(&lexers, NULL, NULL, destroy_lexer_cb);
    for (o = 0; o < COUNT; o++) {
        options_init(&options[o]);
    }
    while (-1 != (o = getopt_long(argc, argv, optstr, long_options, NULL))) {
        switch (o) {
            case 'L':
            {
                puts("Available lexers are:");
                lexer_implementation_each(print_lexer_cb, NULL);
                return EXIT_SUCCESS;
            }
            case 'l':
                if (NULL == (limp = lexer_implementation_by_name(optarg))) {
                    fprintf(stderr, "unknown lexer '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'f':
                if (NULL == (fimp = formatter_implementation_by_name(optarg))) {
                    fprintf(stderr, "unknown formatter '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'o':
                options_add(&options[LEXER], optarg);
                break;
            case 'O':
                options_add(&options[FORMATTER], optarg);
                break;
            default:
                usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (NULL != limp) {
        lexer = lexer_create(limp);
        for (i = 0; i < options[LEXER].options_len; i++) {
            if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[i].name, options[LEXER].options[i].value, options[LEXER].options[i].value_len)) {
                fprintf(stderr, "option '%s' rejected by %s lexer\n", options[LEXER].options[i].name, lexer_implementation_name(lexer_implementation(lexer)));
            }
        }
    }
    fmt = formatter_create(fimp);
    for (i = 0; i < options[FORMATTER].options_len; i++) {
        if (0 != formatter_set_option_as_string(fmt, options[FORMATTER].options[i].name, options[FORMATTER].options[i].value, options[FORMATTER].options[i].value_len)) {
            fprintf(stderr, "option '%s' rejected by %s formatter\n", options[FORMATTER].options[i].name, formatter_implementation_name(formatter_implementation(fmt)));
        }
    }
    if (0 == argc) {
        procfile("-", lexer, fmt, &options[LEXER]);
    } else {
        for ( ; argc--; ++argv) {
            procfile(*argv, lexer, fmt, &options[LEXER]);
        }
    }
    if (NULL != lexer) {
        lexer_destroy(lexer, (on_lexer_destroy_cb_t) lexer_destroy);
    }
    formatter_destroy(fmt);
    for (o = 0; o < COUNT; o++) {
        options_free(&options[o]);
    }
    hashtable_destroy(&lexers);

    return EXIT_SUCCESS;
}
