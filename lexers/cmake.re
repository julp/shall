#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "utils.h"
#include "lexer.h"
#include "lexer-private.h"
#include "darray.h"

typedef struct {
    LexerData data;
    int bracket_len; // total length of "[" "="* "["
    DArray state_stack;
} CMakeLexerData;

#define PUSH_STATE(new_state) \
    do { \
        darray_push(&mydata->state_stack, &YYSTATE); \
        BEGIN(new_state); \
    } while (0);

#define POP_STATE() \
    do { \
        darray_pop(&mydata->state_stack, &YYSTATE); \
    } while (0);

static void cmakeinit(LexerData *data)
{
    CMakeLexerData *mydata;

    mydata = (CMakeLexerData *) data;
    darray_init(&mydata->state_stack, 0, sizeof(data->state));
}

enum {
    STATE(INITIAL),
    STATE(IN_QUOTED_ARGUMENT),
    STATE(IN_BRACKET_STRING),
    STATE(IN_BRACKET_COMMENT),
    STATE(IN_VARIABLE_REFERENCE),
};

static int default_token_type[] = {
    IGNORABLE,
    STRING_DOUBLE,
    STRING_DOUBLE,
    COMMENT_MULTILINE,
    NAME_VARIABLE,
};

#define C(s) { s, STR_LEN(s) }

typedef struct {
    const char *name;
    size_t name_len;
} named_element_t;

static const named_element_t builtin_commands[] = {
    C("add_compile_options"),
    C("add_custom_command"),
    C("add_custom_target"),
    C("add_definitions"),
    C("add_dependencies"),
    C("add_executable"),
    C("add_library"),
    C("add_subdirectory"),
    C("add_test"),
    C("aux_source_directory"),
    C("break"),
    C("build_command"),
    C("build_name"),
    C("cmake_host_system_information"),
    C("cmake_minimum_required"),
    C("cmake_policy"),
    C("configure_file"),
    C("create_test_sourcelist"),
    C("define_property"),
    C("else"),
    C("elseif"),
    C("enable_language"),
    C("enable_testing"),
    C("endforeach"),
    C("endfunction"),
    C("endif"),
    C("endmacro"),
    C("endwhile"),
    C("exec_program"),
    C("execute_process"),
    C("export"),
    C("export_library_dependencies"),
    C("file"),
    C("find_file"),
    C("find_library"),
    C("find_package"),
    C("find_path"),
    C("find_program"),
    C("fltk_wrap_ui"),
    C("foreach"),
    C("function"),
    C("get_cmake_property"),
    C("get_directory_property"),
    C("get_filename_component"),
    C("get_property"),
    C("get_source_file_property"),
    C("get_target_property"),
    C("get_test_property"),
    C("if"),
    C("include"),
    C("include_directories"),
    C("include_external_msproject"),
    C("include_regular_expression"),
    C("install"),
    C("install_files"),
    C("install_programs"),
    C("install_targets"),
    C("link_directories"),
    C("link_libraries"),
    C("list"),
    C("load_cache"),
    C("load_command"),
    C("macro"),
    C("make_directory"),
    C("mark_as_advanced"),
    C("math"),
    C("message"),
    C("option"),
    C("output_required_files"),
    C("project"),
    C("qt_wrap_cpp"),
    C("qt_wrap_ui"),
    C("remove"),
    C("remove_definitions"),
    C("return"),
    C("separate_arguments"),
    C("set"),
    C("set_directory_properties"),
    C("set_property"),
    C("set_source_files_properties"),
    C("set_target_properties"),
    C("set_tests_properties"),
    C("site_name"),
    C("source_group"),
    C("string"),
    C("subdir_depends"),
    C("subdirs"),
    C("target_compile_definitions"),
    C("target_compile_options"),
    C("target_include_directories"),
    C("target_link_libraries"),
    C("try_compile"),
    C("try_run"),
    C("unset"),
    C("use_mangled_mesa"),
    C("utility_source"),
    C("variable_requires"),
    C("variable_watch"),
    C("while"),
    C("write_file"),
};

static int named_elements_cmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return ascii_strcasecmp_l(na->name, na->name_len, nb->name, nb->name_len);
}

/**
 * Syntax: http://www.cmake.org/cmake/help/v3.0/manual/cmake-language.7.html
 * Variable names are case-sensitive and may consist of almost any text, but we recommend sticking to names consisting only of alphanumeric characters plus _ and -.
 * Command names are case-insensitive
 *
 * make install && bin/shall CMakeLists.txt lexers/CMakeLists.txt
 *
 * TODO:
 * - variable reference for $ENV{VAR}
 **/
static int cmakelex(YYLEX_ARGS) {
    CMakeLexerData *mydata;

    mydata = (CMakeLexerData *) data;
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

space = [ \t]; // cmake defines it as [ \t]+ but uses a quantifier in its rules
bracket_open = "[" "="* "[";
bracket_close = "]" "="* "]";
identifier = [A-Za-z_][A-Za-z0-9_]*;

<INITIAL>"#" bracket_open {
#if 1
    mydata->bracket_len = YYLENG - STR_LEN("#");
    BEGIN(IN_BRACKET_COMMENT);
#else
    int eqnumber;

    eqnumber = YYLENG - STR_LEN("#[[");
    while (YYCURSOR < YYLIMIT) {
restart_comment_bracket:
        if (']' == *YYCURSOR++ && YYLIMIT - YYCURSOR >/*=*/ eqnumber/* + 1*/) {
            int i;

            for (i = eqnumber; i > 0; i--) {
                if ('=' != *YYCURSOR++) {
                    goto restart_comment_bracket;
                }
            }
            if (']' == *YYCURSOR++) {
                return COMMENT_MULTILINE;
            }
        }
    }
    YYCURSOR = YYLIMIT; // if we reach this point, comment is unterminated
#endif
    return COMMENT_MULTILINE;
}

<INITIAL>bracket_open {
#if 1
    mydata->bracket_len = YYLENG;
    BEGIN(IN_BRACKET_STRING);
#else
    int eqnumber;

    eqnumber = YYLENG - STR_LEN("[[");
    while (YYCURSOR < YYLIMIT) {
restart_string_bracket:
        if (']' == *YYCURSOR++ && YYLIMIT - YYCURSOR >/*=*/ eqnumber/* + 1*/) {
            int i;

            for (i = eqnumber; i > 0; i--) {
                if ('=' != *YYCURSOR++) {
                    goto restart_string_bracket;
                }
            }
            if (']' == *YYCURSOR++) {
                return STRING_DOUBLE;
            }
        }
    }
    YYCURSOR = YYLIMIT; // if we reach this point, comment is unterminated
#endif
    return STRING_DOUBLE;
}

<IN_BRACKET_STRING,IN_BRACKET_COMMENT>bracket_close {
    int old_state;

    old_state = YYSTATE;
    if (YYLENG == mydata->bracket_len) {
        BEGIN(INITIAL);
    }
    return default_token_type[old_state];
}

<INITIAL>["] {
    BEGIN(IN_QUOTED_ARGUMENT);
    return STRING_DOUBLE;
}

<IN_QUOTED_ARGUMENT> "\\" [()#" \\$@^trn;] {
    return ESCAPED_CHAR;
}

<INITIAL,IN_QUOTED_ARGUMENT,IN_BRACKET_STRING,IN_VARIABLE_REFERENCE>"${" {
    PUSH_STATE(IN_VARIABLE_REFERENCE);
    return SEQUENCE_INTERPOLATED;
}

<IN_VARIABLE_REFERENCE>"}" {
    POP_STATE();
    return SEQUENCE_INTERPOLATED;
}

<IN_QUOTED_ARGUMENT>["] {
    BEGIN(INITIAL);
    return STRING_DOUBLE;
}

<INITIAL>"#" {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR) {
            case '\r':
            case '\n':
                break;
            default:
                ++YYCURSOR;
                continue;
        }
        break;
    }
    return COMMENT_SINGLE;
}

<INITIAL>identifier space* "(" {
    named_element_t key = { (char *) YYTEXT, 0 };

#if 1
    // an ugly yyless to reset YYCURSOR on the first character which is not to the identifier
    for (YYCURSOR = YYTEXT; ' ' != *YYCURSOR && '\t' != *YYCURSOR && '(' != *YYCURSOR; YYCURSOR++)
        ;
    key.name_len = YYLENG;
#else
    key.name_len = YYLENG - 1; // TODO: add spaces
    yyless(key.name_len);
#endif
    if (NULL != bsearch(&key, builtin_commands, ARRAY_SIZE(builtin_commands), sizeof(builtin_commands[0]), named_elements_cmp)) {
        return NAME_BUILTIN;
    } else {
        return NAME_FUNCTION;
    }
}

<INITIAL>[()] {
    return PUNCTUATION;
}

<*>[^] {
    return default_token_type[YYSTATE];
}
*/
}

LexerImplementation cmake_lexer = {
    "CMake",
    "Lexer for CMake files",
    NULL,
    (const char * const []) { "CMakeLists.txt", "*.cmake", NULL },
    (const char * const []) { "text/x-cmake", NULL },
    NULL,
    cmakeinit,
    NULL,
    cmakelex,
    sizeof(CMakeLexerData),
    NULL
};
