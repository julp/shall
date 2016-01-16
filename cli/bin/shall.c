#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "cpp.h"
#include "types.h"
#include "optparse.h"
#include "shall.h"
#include "xtring.h"
#include "hashtable.h"
#include "themes.h"

#ifndef EUSAGE
# define EUSAGE -2
#endif /* !EUSAGE */

#ifdef _MSC_VER
extern char __progname[];
#else
extern char *__progname;
#endif /* _MSC_VER */

static HashTable lexers;
static Options options[COUNT];
static char optstr[] = "f:l:o:t:LO:";

static struct option long_options[] = {
    { "list",             required_argument, NULL, 'L' },
    { "lexer",            required_argument, NULL, 'l' },
    { "formatter",        required_argument, NULL, 'f' },
    { "lexer-option",     required_argument, NULL, 'o' },
    { "formatter-option", required_argument, NULL, 'O' },
    { "theme",            required_argument, NULL, 't' },
    { "scope",            required_argument, NULL, 's' }, // TODO: optional CSS scope to generate CSS rules for theme
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

static void procfile(const char *filename, Formatter *fmt)
{
    size_t o;
    char *result;
    Lexer *lexer;
    String *buffer;
    const LexerImplementation *limp;

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
        read = fread(bufraw, sizeof(bufraw[0]), ARRAY_SIZE(bufraw), fp);
        if (read > 0) {
            string_append_string_len(buffer, bufraw, read);
#ifdef TEST
            debug("encoding = %s", encoding_guess(buffer->ptr, buffer->len, NULL));
//             if (NULL == encoding) {
                if (NULL != memchr(buffer->ptr, '\0', buffer->len)) {
                    debug("binary file detected");
                }
//             }
            {
                const char *errp;

                if (!encoding_utf8_check(buffer->ptr, buffer->len, &errp)) {
                    debug("Invalid byte found: 0x%02" PRIx8 " at offset %ld", (uint8_t) *errp, errp - buffer->ptr);
                }
            }
#endif
            while (ARRAY_SIZE(bufraw) == read) {
                read = fread(bufraw, sizeof(bufraw[0]), ARRAY_SIZE(bufraw), fp);
                string_append_string_len(buffer, bufraw, read);
            }
        }
        if (stdin != fp) {
            fclose(fp);
        }
    }
    if (NULL == (limp = lexer_implementation_for_filename(filename))) {
        if (NULL == (limp = lexer_implementation_guess(buffer->ptr, buffer->len))) {
            // if at least one -l was used, use first one
            if (NULL == (lexer = hashtable_first(&lexers))) {
                // else use text (cat mode)
                limp = lexer_implementation_by_name("text");
            }
        }
    }
    if (NULL == limp) {
        // set implementation from lexer to avoid segfault
        // (we are using the first registered one)
        limp = lexer_implementation(lexer);
    } else {
        if (!hashtable_direct_get(&lexers, limp, &lexer)) {
            lexer = lexer_create(limp);
            for (o = 0; o < options[LEXER].options_len; o++) {
                if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[o].name, options[LEXER].options[o].value, options[LEXER].options[o].value_len)) {
                    fprintf(stderr, "option '%s' rejected by %s lexer\n", options[LEXER].options[o].name, lexer_implementation_name(lexer_implementation(lexer)));
                }
            }
            hashtable_direct_put(&lexers, 0, limp, lexer, NULL);
#ifdef DEBUG
        } else {
            debug("[CACHE] Hit for %s", lexer_implementation_name(lexer_implementation(lexer)));
#endif
        }
    }
    highlight_string(lexer, fmt, buffer->ptr, buffer->len, &result, NULL);
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
    [ OPT_TYPE_THEME ]  = "theme",
    [ OPT_TYPE_LEXER ]  = "lexer",
};

static void print_lexer_option_cb(int type, const char *name, OptionValue defval, const char *docstr, void *UNUSED(data))
{
    printf("  + %s (%s, default: ", name, type2string[type]);
    switch (type) {
        case OPT_TYPE_BOOL:
        {
            static const char *map[] = { "false", "true" };

            fputs(map[!!OPT_GET_BOOL(defval)], stdout);
            break;
        }
        case OPT_TYPE_INT:
            printf("%d", OPT_GET_INT(defval));
            break;
        case OPT_TYPE_STRING:
        {
            size_t i;

            fputc('"', stdout);
            for (i = 0; i < OPT_STRLEN(defval); i++) {
                if ('"' == OPT_STRVAL(defval)[i]) {
                    fputs("\"", stdout);
                } else {
                    fputc(OPT_STRVAL(defval)[i], stdout);
                }
            }
            fputc('"', stdout);
            break;
        }
        case OPT_TYPE_THEME:
            fputs("null/default", stdout);
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

static void print_formatter_cb(const FormatterImplementation *imp, void *UNUSED(data))
{
    const char *imp_name;

    imp_name = formatter_implementation_name(imp);
    printf("- %s\n", imp_name);
    formatter_implementation_each_option(imp, print_lexer_option_cb, NULL);
}

static void print_theme_cb(const Theme *theme, void *UNUSED(data))
{
    const char *name;

    name = theme_name(theme);
    printf("- %s\n", name);
}

static void destroy_lexer_cb(void *ptr)
{
    Lexer *lexer;

    lexer = (Lexer *) ptr;
    lexer_destroy(lexer, (on_lexer_destroy_cb_t) lexer_destroy);
}

static void on_exit_cb(void)
{
    int o;

    for (o = 0; o < COUNT; o++) {
        options_free(&options[o]);
    }
    hashtable_destroy(&lexers);
}

int main(int argc, char **argv)
{
    int o;
    size_t i;
    Formatter *fmt;
    const FormatterImplementation *fimp;

    fmt = NULL;
    fimp = termfmt;
    for (o = 0; o < COUNT; o++) {
        options_init(&options[o]);
    }
    hashtable_ascii_cs_init(&lexers, NULL, NULL, destroy_lexer_cb);
    atexit(on_exit_cb);
    while (-1 != (o = getopt_long(argc, argv, optstr, long_options, NULL))) {
        switch (o) {
            case 'L':
            {
                if (2 != argc) {
                    usage();
                } else {
                    puts("Available lexers are:");
                    lexer_implementation_each(print_lexer_cb, NULL);
                    puts("\nAvailable formatters are:");
                    formatter_implementation_each(print_formatter_cb, NULL);
                    puts("\nAvailable themes are:");
                    theme_each(print_theme_cb, NULL);
                    return EXIT_SUCCESS;
                }
            }
            case 't':
            {
                if (3 != argc) {
                    usage();
                } else {
                    const Theme *theme;

                    // TODO: do not allow mix of options (L, t and l/f/o/O are mutually exclusive ; L/t don't expect additionnal arg[cv]) - imply to do these checks after full options processing?
                    if (NULL == (theme = theme_by_name(optarg))) {
                        fprintf(stderr, "unknown theme '%s'\n", optarg);
                    } else {
                        char *css;

                        css = theme_export_as_css(theme, NULL, true);
                        fputs(css, stdout);
                        free(css);
                    }
                    return NULL == theme ? EXIT_FAILURE : EXIT_SUCCESS;
                }
            }
            case 'l':
            {
                const LexerImplementation *limp;

                if (NULL == (limp = lexer_implementation_by_name(optarg))) {
                    fprintf(stderr, "skip unknown lexer '%s'\n", optarg);
                    // should we reset its options?
                } else {
                    Lexer *lexer;

                    if (!hashtable_direct_get(&lexers, limp, &lexer)) {
                        lexer = lexer_create(limp);
                        hashtable_direct_put(&lexers, 0, limp, lexer, NULL);
                    }
                    // apply options (-o) which appears before this lexer (-l)
                    for (i = 0; i < options[LEXER].options_len; i++) {
                        if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[i].name, options[LEXER].options[i].value, options[LEXER].options[i].value_len)) {
                            fprintf(stderr, "option '%s' rejected by %s lexer\n", options[LEXER].options[i].name, lexer_implementation_name(limp));
                        }
                    }
                    // then clear these options for the next one (if any other lexer)
                    options_clear(&options[LEXER]);
                }
                break;
            }
            case 'f':
                if (NULL == (fimp = formatter_implementation_by_name(optarg))) {
                    fprintf(stderr, "unknown formatter '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            /**
             * NOTE:
             * - lexer options (-o arguments) apply to the lexer (-l) which follows
             * - lexer options which apply to any lexer (ie at the end of the command line)
             *   will be applied (if supported) to guessed lexers
             *
             * Eg: -o phpopt1 -o phpopt2 -l php -o pgopt1 -l pgsql -o foo -o bar
             *
             * - phpopt1 and phpopt2 concerns PHP lexer
             * - pgopt1, the PostgreSQL lexer
             * - foo and bar, any other lexer
             */
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

    fmt = formatter_create(fimp);
    for (i = 0; i < options[FORMATTER].options_len; i++) {
        if (0 != formatter_set_option_as_string(fmt, options[FORMATTER].options[i].name, options[FORMATTER].options[i].value, options[FORMATTER].options[i].value_len)) {
            fprintf(stderr, "option '%s' rejected by %s formatter\n", options[FORMATTER].options[i].name, formatter_implementation_name(formatter_implementation(fmt)));
        }
    }
    if (0 == argc) {
        procfile("-", fmt);
    } else {
        for ( ; argc--; ++argv) {
            procfile(*argv, fmt);
        }
    }
    formatter_destroy(fmt);

    return EXIT_SUCCESS;
}
