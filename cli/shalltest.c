#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <fts.h>

#include "cpp.h"
#include "types.h"
#include "optparse.h"
#include "shall.h"
#include "utils.h"
#include "xtring.h"

#ifndef EUSAGE
# define EUSAGE -2
#endif /* !EUSAGE */

#ifdef _MSC_VER
extern char __progname[];
#else
extern char *__progname;
#endif /* _MSC_VER */

#define RED(str)   "\33[1;31m" str "\33[0m"
#define GREEN(str) "\33[1;32m" str "\33[0m"

#define STERR(format, ...) \
    fprintf(stderr, "[ ERR ] " format "\n", ## __VA_ARGS__)

#define STWARN(format, ...) \
    fprintf(stderr, "[ WARN ] " format "\n", ## __VA_ARGS__)

static char optstr[] = "v";

static struct option long_options[] = {
//     { "list",             required_argument, NULL, 'L' },
    { "verbose", no_argument, NULL, 'v' },
    { NULL,      no_argument, NULL, 0   }
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

static int string_fgets(String *str, FILE *fp)
{
    if (feof(fp)) {
        return 0;
    } else {
        while (1) {
            size_t buffer_len;
            char buffer[8192];

            if (NULL == fgets(buffer, ARRAY_SIZE(buffer), fp)) {
                break;
            }
            buffer_len = strlen(buffer);
            string_append_string_len(str, buffer, buffer_len);
            if ('\n' == buffer[buffer_len - 1]) {
                break;
            }
        }
        return 1;
    }
}

typedef union {
    struct {
        String *line;
        String *desc;
        String *source;
        String *expect;
    };
    String *buffer[4];
} st_ctxt_t;

static void ctxt_init(st_ctxt_t *ctxt)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(ctxt->buffer); i++) {
        ctxt->buffer[i] = string_new();
    }
}

static void ctxt_apply_cb(st_ctxt_t *ctxt, void (*cb)(String *))
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(ctxt->buffer); i++) {
        cb(ctxt->buffer[i]);
    }
}

static void ctxt_destroy(st_ctxt_t *ctxt)
{
    ctxt_apply_cb(ctxt, string_destroy);
}

static void ctxt_flush(st_ctxt_t *ctxt)
{
    ctxt_apply_cb(ctxt, string_truncate);
}

static void ctxt_chomp(st_ctxt_t *ctxt)
{
    ctxt_apply_cb(ctxt, string_chomp);
}

static int procfile(const char *filename, st_ctxt_t *ctxt, int verbosity)
{
    enum {
        PART_NONE,
        PART_DESCRIPTION,
        PART_SOURCE,
        PART_EXPECT,
        PART_LEXER,
        PART_FORMATTER
    };

    enum { FD_EXPECT, FD_SOURCE, FD_COUNT };

    size_t i;
    FILE *fp;
    Lexer *lexer;
    char *result;
    Formatter *fmt;
    int oldpart, part;
    size_t result_len;
    Options options[COUNT];
    int ret, status, fd[FD_COUNT] = { -1, -1 };
    int guess_limp;
    const LexerImplementation *limp;
    const FormatterImplementation *fimp;
//     char ppath[FD_COUNT][PATH_MAX]; // TODO: build a unique path to run several shalltest in parallel ("/tmp/shall.<source/expect>.$$")?
    const char *ppath[FD_COUNT] = { "/tmp/shallexpect", "/tmp/shallsource" };

    ret = 0;
    limp = NULL;
    result = NULL;
    guess_limp = 0;
    fimp = plainfmt;
    oldpart = part = PART_NONE;
    ctxt_flush(ctxt);
    for (i = 0; i < COUNT; i++) {
        options_init(&options[i]);
    }
    if (NULL == (fp = fopen(filename, "r"))) {
        return 0;
    }
    while (string_fgets(ctxt->line, fp)) {
        if (string_startswith(ctxt->line, "--", STR_LEN("--"))) {
            char *p;

            p = ctxt->line->ptr + STR_LEN("--");
            while (' ' == *p || '\t' == *p) {
                ++p;
            }
            if (0 == strncmp("TEST", p, STR_LEN("TEST"))) {
                part = PART_DESCRIPTION;
                p += STR_LEN("TEST");
            } else if (0 == strncmp("SOURCE", p, STR_LEN("SOURCE"))) {
                part = PART_SOURCE;
                p += STR_LEN("SOURCE");
            } else if (0 == strncmp("LEXER", p, STR_LEN("LEXER"))) {
                part = PART_LEXER;
                p += STR_LEN("LEXER");
            } else if (0 == strncmp("FORMATTER", p, STR_LEN("FORMATTER"))) {
                part = PART_FORMATTER;
                p += STR_LEN("FORMATTER");
            } else if (0 == strncmp("EXPECT", p, STR_LEN("EXPECT"))) {
                part = PART_EXPECT;
                p += STR_LEN("EXPECT");
            }
            while (' ' == *p || '\t' == *p) {
                ++p;
            }
            if ('-' == *p && '-' == p[1] && '\n' == p[2]) {
                oldpart = part;
                string_truncate(ctxt->line);
                continue;
            } else {
                part = oldpart;
            }
        }
        if (PART_NONE == part) {
            STWARN("line '%s' found out of any section", ctxt->line->ptr);
        }
        if (part == oldpart) {
            if (PART_LEXER == part || PART_FORMATTER == part) {
                int has_equal;

                string_chomp(ctxt->line);
                has_equal = NULL != memchr(ctxt->line->ptr, '=', ctxt->line->len);
                if (PART_LEXER == part) {
                    if (!has_equal) {
                        if (0 == strcmp_l(ctxt->line->ptr, ctxt->line->len, "guess", STR_LEN("guess"))) {
                            guess_limp = 1;
                        } else {
                            limp = lexer_implementation_by_name(ctxt->line->ptr);
                        }
                    } else {
                        options_add(&options[LEXER], ctxt->line->ptr);
                    }
                } else if (PART_FORMATTER == part) {
                    if (!has_equal) {
                        fimp = formatter_implementation_by_name(ctxt->line->ptr);
                    } else {
                        options_add(&options[FORMATTER], ctxt->line->ptr);
                    }
                }
            } else {
                string_append_string_len(ctxt->buffer[part], ctxt->line->ptr, ctxt->line->len);
            }
        }
        string_truncate(ctxt->line);
    }
    fclose(fp);
    ctxt_chomp(ctxt);
    if (string_empty(ctxt->desc)) {
        STWARN("no description set for %s", filename);
    }
    if (guess_limp) {
        limp = lexer_implementation_guess(ctxt->source->ptr, ctxt->source->len);
    }
    if (NULL == limp) {
        STERR("no lexer set for %s", filename);
        // TODO: cleanup?
        return 0;
    }
    lexer = lexer_create(limp);
    for (i = 0; i < options[LEXER].options_len; i++) {
        if (0 != lexer_set_option_as_string(lexer, options[LEXER].options[i].name, options[LEXER].options[i].value, options[LEXER].options[i].value_len)) {
            STWARN("option '%s' rejected by %s lexer", options[LEXER].options[i].name, lexer_implementation_name(lexer_implementation(lexer)));
        }
    }
    fmt = formatter_create(fimp);
    for (i = 0; i < options[FORMATTER].options_len; i++) {
        if (0 != formatter_set_option_as_string(fmt, options[FORMATTER].options[i].name, options[FORMATTER].options[i].value, options[FORMATTER].options[i].value_len)) {
            STWARN("option '%s' rejected by %s formatter", options[FORMATTER].options[i].name, formatter_implementation_name(formatter_implementation(fmt)));
        }
    }
    result_len = highlight_string(lexer, fmt, ctxt->source->ptr, &result);
    if (verbosity) {
        printf("=== <source> ===\n%s\n=== </source> ===\n", ctxt->source->ptr);
        printf("=== <get> ===\n%s\n=== </get> ===\n", result);
        printf("=== <expect> ===\n%s\n=== </expect> ===\n", ctxt->expect->ptr);
    }
    lexer_destroy(lexer, (on_lexer_destroy_cb_t) lexer_destroy);
    formatter_destroy(fmt);
    for (i = 0; i < COUNT; i++) {
        options_free(&options[i]);
    }
    {
        pid_t pid;

        // snprintf(ppath[FD_SOURCE], "/tmp/shall.source.%d", pid);
        for (i = 0; i < FD_COUNT; i++) {
            unlink(ppath[i]);
            if (0 != mkfifo(ppath[i], 0640)) {
                STERR("mkfifo %s failed: %s", ppath[i], strerror(errno));
                return 0;
            }
        }
        pid = fork();
        if (-1 == pid) {
            return 0;
        } else if (0 == pid) {
            int fdout;
            char *dot, dpath[PATH_MAX];

            strcpy(dpath, filename);
            dot = strrchr(dpath, '.');
            assert(NULL != dot);
//             strcat(dot + 1, "diff");
            *++dot = 'd';
            *++dot = 'i';
            *++dot = 'f';
            *++dot = 'f';
            *++dot = '\0';
            fdout = open(dpath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            dup2(fdout, STDOUT_FILENO);
            close(fdout);
            execlp("diff", "diff", "-u", ppath[FD_EXPECT], ppath[FD_SOURCE], NULL);
            STERR("execlp failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < FD_COUNT; i++) {
            if (-1 == (fd[i] = open(ppath[i], O_WRONLY))) {
                STERR("open %s failed: %s", ppath[i], strerror(errno));
                goto end;
            }
        }
        // ugly hack for now to match the trailing '\n' added by the plain formatter
        if (plainfmt == fimp) {
            string_append_char(ctxt->expect, '\n');
        }
        if (-1 == write(fd[FD_EXPECT], ctxt->expect->ptr, ctxt->expect->len)) {
            STERR("write failed: %s", strerror(errno));
            goto end;
        }
        if (-1 == write(fd[FD_SOURCE], result, result_len)) {
            STERR("write failed: %s", strerror(errno));
            goto end;
        }
end:
        for (i = 0; i < FD_COUNT; i++) {
            if (-1 != fd[i]) {
                close(fd[i]);
            }
        }
        waitpid(pid, &status, 0);
        ret = WIFEXITED(status) ? EXIT_SUCCESS == WEXITSTATUS(status) : 0;
        for (i = 0; i < FD_COUNT; i++) {
            unlink(ppath[i]);
        }
    }
    if (NULL != result) {
        free(result);
    }

    printf("Test: %s (%s) [ %s ]\n", ctxt->desc->ptr, filename, ret ? GREEN("PASS") : RED("FAIL"));
    
    return ret;
}

static int strendswith(const char *string, size_t string_len, const char *suffix, size_t suffix_len)
{
    if (suffix_len > string_len) {
        return 0;
    }

    return 0 == strcmp(string + string_len - suffix_len, suffix);
}

static int procdir(char **argv, st_ctxt_t *ctxt, int verbosity)
{
    int ret;
    FTS *fts;
    FTSENT *p;

    ret = 1;
    if (NULL == (fts = fts_open(argv, FTS_NOSTAT | FTS_NOCHDIR, NULL))) {
        STERR("can't fts_open: %s", strerror(errno));
        return 0;
    }
    while (NULL != (p = fts_read(fts))) {
        switch (p->fts_info) {
            case FTS_DNR:
            case FTS_ERR:
                STERR("fts_read failed on %s: %s", p->fts_path, strerror(p->fts_errno));
                break;
            case FTS_D:
            case FTS_DP:
                break;
            case FTS_DC:
                STWARN("recursive directory loop on %s", p->fts_path);
                break;
            default:
                if (strendswith(p->fts_path, p->fts_pathlen, ".ssc", STR_LEN(".ssc"))) {
                    ret &= procfile(p->fts_path, ctxt, verbosity);
                }
                break;
        }
    }
    fts_close(fts);

    return ret;
}

int main(int argc, char **argv)
{
    st_ctxt_t ctxt;
    int o, res, verbosity;

    res = 1;
    verbosity = 0;
    ctxt_init(&ctxt);
    while (-1 != (o = getopt_long(argc, argv, optstr, long_options, NULL))) {
        switch (o) {
            case 'v':
                ++verbosity;
                break;
            default:
                usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (0 == argc) {
        usage();
    } else {
#if 0
        for ( ; argc--; ++argv) {
            res &= procfile(*argv, &ctxt, verbosity);
        }
#else
        res = procdir(argv, &ctxt, verbosity);
#endif
    }
    ctxt_destroy(&ctxt);

    return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
