#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "cpp.h"
#include "optparse.h"
#include "shall.h"
#include "xtring.h"
#include "hashtable.h"
#include "themes.h"
#include "vernum.h"
#include "version.h"
#include "encoding.h"
#include "lexer_group.h"

#if defined(__FreeBSD__) && __FreeBSD__ >= 9
# include <unistd.h>
# include <sys/capsicum.h>

# define CAP_RIGHTS_LIMIT(fd, ...) \
    do { \
        cap_rights_t rights; \
 \
        cap_rights_init(&rights, ## __VA_ARGS__); \
        if (0 != cap_rights_limit(fd, &rights) && ENOSYS != errno) { \
            perror("cap_rights_limit"); \
        } \
    } while (0);

# define CAP_ENTER() \
    do { \
        if (0 != cap_enter() && ENOSYS != errno) { \
            perror("cap_enter"); \
        } \
    } while (0);
#else
# define CAP_RIGHTS_LIMIT(fd, ...) \
    /* NOP */

# define CAP_ENTER() \
    /* NOP */
#endif /* FreeBSD >= 9.0 */

#ifndef EUSAGE
# define EUSAGE -2
#endif /* !EUSAGE */

#ifdef _MSC_VER
extern char __progname[];
#else
extern char *__progname;
#endif /* _MSC_VER */

static bool vFlag;
static HashTable lexers;
static const char *outputenc;
static OptionsStore options[COUNT];
static char optstr[] = "cef:l:o:t:vLO:";

static struct option long_options[] = {
    { "chain",            no_argument,       NULL, 'c' },
    { "example",          no_argument,       NULL, 'e' },
    { "list",             required_argument, NULL, 'L' },
    { "lexer",            required_argument, NULL, 'l' },
    { "formatter",        required_argument, NULL, 'f' },
    { "lexer-option",     required_argument, NULL, 'o' },
    { "formatter-option", required_argument, NULL, 'O' },
    { "theme",            required_argument, NULL, 't' },
    { "sample",           no_argument,       NULL, 'e' },
    { "scope",            required_argument, NULL, 's' }, // TODO: optional CSS scope to generate CSS rules for theme
    { "verbose",          no_argument,       NULL, 'v' },
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

static void procfile(const char *filename, FILE *fp, Formatter *fmt)
{
    size_t o;
    char *result;
    Lexer *lexer;
    LexerGroup *g;
    String *buffer;
    size_t result_len;
    const char *inputenc;
    const LexerImplementation *limp;

    result = NULL;
    inputenc = NULL;
    {
        size_t read;
        char bufraw[1024];

        if (0 == strcmp(filename, "-")) {
            inputenc = encoding_stdin_get();
        }
        buffer = string_new();
        read = fread(bufraw, sizeof(bufraw[0]), ARRAY_SIZE(bufraw), fp);
        if (read > 0) {
            string_append_string_len(buffer, bufraw, read);
            if (NULL != memchr(buffer->ptr, '\0', buffer->len)) {
                fprintf(stderr, "%s: binary file found, skip\n", filename);
                goto failure;
            } else {
                inputenc = encoding_guess(buffer->ptr, buffer->len, NULL);
            }
            while (ARRAY_SIZE(bufraw) == read) {
                read = fread(bufraw, sizeof(bufraw[0]), ARRAY_SIZE(bufraw), fp);
                string_append_string_len(buffer, bufraw, read);
            }
        }
        if (ferror(fp)) {
            fprintf(stderr, "failed to read %s\n", filename);
            goto failure;
        }
    }
    if (NULL != inputenc && 0 != strcmp("UTF-8", inputenc)) {
        bool ok;
        char *utf8;
        size_t utf8_len;

        ok = encoding_convert_to_utf8(inputenc, buffer->ptr, buffer->len, &utf8, &utf8_len);
        string_destroy(buffer);
        if (ok) {
            buffer = string_adopt_string_len(utf8, utf8_len);
        } else {
            fprintf(stderr, "failed to convert '%s' (from %s) to UTF-8\n", inputenc, filename);
            goto failure;
        }
    }
    if (NULL == (limp = lexer_implementation_for_filename(filename))) {
        if (NULL == (limp = lexer_implementation_guess(buffer->ptr, buffer->len))) {
            // if at least one -l was used, use first one
            if (NULL == (g = hashtable_first(&lexers))) {
                // else use text (acts as cat)
                limp = lexer_implementation_by_name("text");
            } else {
                lexer = g->lexers[0];
            }
        }
    }
    if (NULL == limp) {
        // set implementation from lexer to avoid segfault
        // (we are using the first registered one)
        limp = lexer_implementation(lexer);
    } else {
        if (!hashtable_direct_get(&lexers, limp, &g)) {
            g = group_new(lexer = lexer_create(limp));
            for (o = 0; o < options[LEXER].options_len; o++) {
                if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[o].name, options[LEXER].options[o].value, options[LEXER].options[o].value_len)) {
                    fprintf(stderr, "option '%s' rejected by %s lexer\n", options[LEXER].options[o].name, lexer_implementation_name(lexer_implementation(lexer)));
                }
            }
            hashtable_direct_put(&lexers, 0, limp, g, NULL);
#ifdef DEBUG
        } else {
            debug("[CACHE] Hit for %s", lexer_implementation_name(lexer_implementation(g->lexers[0])));
#endif /* DEBUG */
        }
    }
    if (vFlag) {
        fprintf(stdout, "%s:\n", filename);
    }
    highlight_string(buffer->ptr, buffer->len, &result, &result_len, fmt, g->count, g->lexers);
    if (0 != strcmp("UTF-8", outputenc)) {
        bool ok;
        char *nonutf8;
        size_t nonutf8_len;

        ok = encoding_convert_from_utf8(outputenc, result, result_len, &nonutf8, &nonutf8_len);
        free(result);
        if (ok) {
            result = nonutf8;
        } else {
            fprintf(stderr, "failed to convert result from UTF-8 to %s\n", outputenc);
            goto failure;
        }
    }
    // print result
    puts(result);
failure:
    // free
    string_destroy(buffer);
    if (NULL != result) {
        free(result);
    }
    if (stdin != fp) {
        fclose(fp);
    }
}

static const char *type2string[] = {
    [ OPT_TYPE_INT ]    = "int",
    [ OPT_TYPE_BOOL ]   = "boolean",
    [ OPT_TYPE_STRING ] = "string",
    [ OPT_TYPE_THEME ]  = "theme",
    [ OPT_TYPE_LEXER ]  = "lexer",
};

static void print_option_cb(int type, const char *name, OptionValue defval, const char *docstr, void *UNUSED(data))
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
    lexer_implementation_each_option(imp, print_option_cb, NULL);
}

static void print_formatter_cb(const FormatterImplementation *imp, void *UNUSED(data))
{
    const char *imp_name;

    imp_name = formatter_implementation_name(imp);
    printf("- %s\n", imp_name);
    formatter_implementation_each_option(imp, print_option_cb, NULL);
}

static void print_theme_cb(const Theme *theme, void *UNUSED(data))
{
    const char *name;

    name = theme_name(theme);
    printf("- %s\n", name);
}

/*
static void destroy_lexer_cb(void *ptr)
{
    Lexer *lexer;

    lexer = (Lexer *) ptr;
    lexer_destroy(lexer, (on_lexer_destroy_cb_t) lexer_destroy);
}
*/

static void on_exit_cb(void)
{
    int o;

    for (o = 0; o < COUNT; o++) {
        options_store_free(&options[o]);
    }
    hashtable_destroy(&lexers);
}

int main(int argc, char **argv)
{
    int o;
    size_t i;
    LexerGroup *g;
    Formatter *fmt;
    bool cFlag, eFlag;
    const FormatterImplementation *fimp;

    {
        Version v = { SHALL_VERSION_MAJOR, SHALL_VERSION_MINOR, SHALL_VERSION_PATCH };

        if (!version_check(v)) {
            fprintf(stderr, "version mismatch\n");
            return EXIT_FAILURE;
        }
    }
    g = NULL;
    fmt = NULL;
#if 0
    fimp = termfmt;
#else
    fimp = formatter_implementation_by_name("terminal");
#endif
    assert(NULL != fimp);
    eFlag = cFlag = vFlag = false;
    for (o = 0; o < COUNT; o++) {
        options_store_init(&options[o]);
    }
    outputenc = encoding_stdout_get();
    hashtable_ascii_cs_init(&lexers, NULL, NULL, group_destroy);
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
#if 1
                    {
#include "formatter.h"
                        Iterator it;
                        const FormatterImplementation *imp;

                        formatter_implementations_to_iterator(&it);
                        for (iterator_first(&it); iterator_is_valid(&it, NULL, &imp); iterator_next(&it)) {
                            Iterator subit;

                            printf("- %s\n", formatter_implementation_name(imp));
                            if (formatter_implementation_options_to_iterator(&subit, imp)) {
                                const FormatterOption *option;

                                for (iterator_first(&subit); iterator_is_valid(&subit, NULL, &option); iterator_next(&subit)) {
                                    print_option_cb(option->type, option->name, option->defval, option->docstr, NULL);
                                }
                                iterator_close(&subit);
                            }
                        }
                        iterator_close(&it);
                    }
#else
                    formatter_implementation_each(print_formatter_cb, NULL);
#endif
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

                    // TODO: forbids secondary option with chaining?
                    if (cFlag) {
                        group_append(&g, lexer = lexer_create(limp));
                    } else {
                        if (!hashtable_direct_get(&lexers, limp, &g)) {
                            g = group_new(lexer = lexer_create(limp));
                            hashtable_direct_put(&lexers, 0, limp, g, NULL);
                        } else {
                            lexer = g->lexers[0];
                        }
                    }
                    // apply options (-o) which appears before this lexer (-l)
                    for (i = 0; i < options[LEXER].options_len; i++) {
                        if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[i].name, options[LEXER].options[i].value, options[LEXER].options[i].value_len)) {
                            fprintf(stderr, "option '%s' rejected by %s lexer\n", options[LEXER].options[i].name, lexer_implementation_name(limp));
                        }
                    }
                    // then clear these options for the next one (if any other lexer)
                    options_store_clear(&options[LEXER]);
                }
                cFlag = false;
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
                options_store_add(&options[LEXER], optarg);
                break;
            case 'O':
                options_store_add(&options[FORMATTER], optarg);
                break;
            case 'v':
                vFlag = true;
                break;
            /**
             * Chain/stack lexers directly, this feature replaces the -o secondary=<name>
             * The user can preset its own stack.
             *
             * TODO: this really works if the top/first lexer matches with the filename.
             * Eg: -l php -cl erb -cl html implies an .php extension (not .erb) and vice
             * versa (-l erb -cl php -cl html with .erb, not .php)
             *
             * This "behavior" is the result of the current implementation of procfile
             * where we try first to find a lexer based on the filename (with a call to
             * lexer_implementation_for_filename)
             *
             * A workaround is to pipe the output of cat (cat file | shall).
             */
            case 'c':
                cFlag = true;
                break;
            case 'e':
                eFlag = true;
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
    {
        FILE *fp[argc + 1];

#if defined(__OpenBSD__) && OpenBSD >= 201605
        if (-1 == pledge("stdio rpath", NULL)) {
            perror("pledge");
        }
#endif /* OpenBSD >= 5.9 */
        CAP_RIGHTS_LIMIT(STDOUT_FILENO, CAP_WRITE);
        CAP_RIGHTS_LIMIT(STDERR_FILENO, CAP_WRITE);
        if (0 == argc) {
            fp[0] = stdin;
            CAP_RIGHTS_LIMIT(STDIN_FILENO, CAP_READ);
        } else {
            int i;
            char **p;

            for (i = argc, p = argv; 0 != i--; ++p) {
                if (0 == strcmp(*p, "-")) {
                    fp[p - argv] = stdin;
                    CAP_RIGHTS_LIMIT(STDIN_FILENO, CAP_READ);
                } else {
                    if (NULL == (fp[p - argv] = fopen(*p, "r"))) {
                        fprintf(stderr, "unable to open '%s', skip\n", *p);
                    } else {
                        CAP_RIGHTS_LIMIT(fileno(fp[p - argv]), CAP_READ);
                    }
                }
            }
        }
        CAP_ENTER();
        if (eFlag) {
            char *result;

            highlight_sample(&result, NULL, fmt);
            fputs(result, stdout);
            free(result);
        } else {
            if (0 == argc) {
                procfile("-", fp[0], fmt);
            } else {
                char **p;

                for (p = argv; 0 != argc--; ++p) {
                    if (NULL != fp[p - argv]) {
                        procfile(*p, fp[p - argv], fmt);
                    }
                }
            }
        }
    }
    formatter_destroy(fmt);

    return EXIT_SUCCESS;
}
