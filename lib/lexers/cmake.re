#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "utils.h"
#include "lexer.h"

typedef struct {
    LexerData data;
    int bracket_len; // total length of "[" "="* "["
} CMakeLexerData;

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

static const named_element_t builtin_commands[] = {
    NE("add_compile_options"),
    NE("add_custom_command"),
    NE("add_custom_target"),
    NE("add_definitions"),
    NE("add_dependencies"),
    NE("add_executable"),
    NE("add_library"),
    NE("add_subdirectory"),
    NE("add_test"),
    NE("aux_source_directory"),
    NE("break"),
    NE("build_command"),
    NE("build_name"),
    NE("cmake_host_system_information"),
    NE("cmake_minimum_required"),
    NE("cmake_policy"),
    NE("configure_file"),
    NE("create_test_sourcelist"),
    NE("define_property"),
    NE("else"),
    NE("elseif"),
    NE("enable_language"),
    NE("enable_testing"),
    NE("endforeach"),
    NE("endfunction"),
    NE("endif"),
    NE("endmacro"),
    NE("endwhile"),
    NE("exec_program"),
    NE("execute_process"),
    NE("export"),
    NE("export_library_dependencies"),
    NE("file"),
    NE("find_file"),
    NE("find_library"),
    NE("find_package"),
    NE("find_path"),
    NE("find_program"),
    NE("fltk_wrap_ui"),
    NE("foreach"),
    NE("function"),
    NE("get_cmake_property"),
    NE("get_directory_property"),
    NE("get_filename_component"),
    NE("get_property"),
    NE("get_source_file_property"),
    NE("get_target_property"),
    NE("get_test_property"),
    NE("if"),
    NE("include"),
    NE("include_directories"),
    NE("include_external_msproject"),
    NE("include_regular_expression"),
    NE("install"),
    NE("install_files"),
    NE("install_programs"),
    NE("install_targets"),
    NE("link_directories"),
    NE("link_libraries"),
    NE("list"),
    NE("load_cache"),
    NE("load_command"),
    NE("macro"),
    NE("make_directory"),
    NE("mark_as_advanced"),
    NE("math"),
    NE("message"),
    NE("option"),
    NE("output_required_files"),
    NE("project"),
    NE("qt_wrap_cpp"),
    NE("qt_wrap_ui"),
    NE("remove"),
    NE("remove_definitions"),
    NE("return"),
    NE("separate_arguments"),
    NE("set"),
    NE("set_directory_properties"),
    NE("set_property"),
    NE("set_source_files_properties"),
    NE("set_target_properties"),
    NE("set_tests_properties"),
    NE("site_name"),
    NE("source_group"),
    NE("string"),
    NE("subdir_depends"),
    NE("subdirs"),
    NE("target_compile_definitions"),
    NE("target_compile_options"),
    NE("target_include_directories"),
    NE("target_link_libraries"),
    NE("try_compile"),
    NE("try_run"),
    NE("unset"),
    NE("use_mangled_mesa"),
    NE("utility_source"),
    NE("variable_requires"),
    NE("variable_watch"),
    NE("while"),
    NE("write_file"),
};

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
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

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
                TOKEN(COMMENT_MULTILINE);
            }
        }
    }
    YYCURSOR = YYLIMIT; // if we reach this point, comment is unterminated
#endif
    TOKEN(COMMENT_MULTILINE);
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
                TOKEN(STRING_DOUBLE);
            }
        }
    }
    YYCURSOR = YYLIMIT; // if we reach this point, comment is unterminated
#endif
    TOKEN(STRING_DOUBLE);
}

<IN_BRACKET_STRING,IN_BRACKET_COMMENT>bracket_close {
    int old_state;

    old_state = YYSTATE;
    if (YYLENG == mydata->bracket_len) {
        BEGIN(INITIAL);
    }
    TOKEN(default_token_type[old_state]);
}

<INITIAL>["] {
    BEGIN(IN_QUOTED_ARGUMENT);
    TOKEN(STRING_DOUBLE);
}

<IN_QUOTED_ARGUMENT> "\\" [()#" \\$@^trn;] {
    TOKEN(ESCAPED_CHAR);
}

<INITIAL,IN_QUOTED_ARGUMENT,IN_BRACKET_STRING,IN_VARIABLE_REFERENCE>"${" {
    PUSH_STATE(IN_VARIABLE_REFERENCE);
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_VARIABLE_REFERENCE>"}" {
    POP_STATE();
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_QUOTED_ARGUMENT>["] {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
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
    TOKEN(COMMENT_SINGLE);
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
    if (NULL != bsearch(&key, builtin_commands, ARRAY_SIZE(builtin_commands), sizeof(builtin_commands[0]), named_elements_casecmp)) {
        TOKEN(NAME_BUILTIN);
    } else {
        TOKEN(NAME_FUNCTION);
    }
}

<INITIAL>[()] {
    TOKEN(PUNCTUATION);
}

<*>[^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation cmake_lexer = {
    "CMake",
    0,
    "Lexer for CMake files",
    NULL,
    (const char * const []) { "CMakeLists.txt", "*.cmake", NULL },
    (const char * const []) { "text/x-cmake", NULL },
    NULL,
    NULL,
    NULL,
    cmakelex,
    sizeof(CMakeLexerData),
    NULL,
    NULL
};
