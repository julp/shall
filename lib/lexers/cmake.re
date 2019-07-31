/**
 * Spec/language reference:
 * - https://cmake.org/cmake/help/v3.5/manual/cmake-language.7.html
 */

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
    STATE(IN_EXPRESSION),
    STATE(IN_QUOTED_ARGUMENT),
    STATE(IN_BRACKET_STRING),
    STATE(IN_VARIABLE_REFERENCE),
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_EXPRESSION) ] = STRING,
    [ STATE(IN_QUOTED_ARGUMENT) ] = STRING_DOUBLE,
    [ STATE(IN_BRACKET_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_VARIABLE_REFERENCE) ] = NAME_VARIABLE,
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
    NE("build_name"), // deprecated
    NE("cmake_host_system_information"),
    NE("cmake_minimum_required"),
    NE("cmake_parse_arguments"), // becomes a real/builtin command as of 3.5
    NE("cmake_policy"),
    NE("configure_file"),
    NE("continue"), // 3.2
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
    NE("exec_program"), // deprecated
    NE("execute_process"),
    NE("export"),
    NE("export_library_dependencies"), // deprecated
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
    NE("install_files"), // deprecated
    NE("install_programs"), // deprecated
    NE("install_targets"), // deprecated
    NE("link_directories"),
    NE("link_libraries"),
    NE("list"),
    NE("load_cache"),
    NE("load_command"), // deprecated
    NE("macro"),
    NE("make_directory"), // deprecated
    NE("mark_as_advanced"),
    NE("math"),
    NE("message"),
    NE("option"),
    NE("output_required_files"), // deprecated
    NE("project"),
    NE("qt_wrap_cpp"),
    NE("qt_wrap_ui"),
    NE("remove"), // deprecated
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
    NE("subdir_depends"), // deprecated
    NE("subdirs"), // deprecated
    NE("target_compile_definitions"),
    NE("target_compile_features"), // 3.1
    NE("target_compile_options"),
    NE("target_include_directories"),
    NE("target_link_libraries"),
    NE("target_sources"), // 3.1
    NE("try_compile"),
    NE("try_run"),
    NE("unset"),
    NE("use_mangled_mesa"), // deprecated
    NE("utility_source"), // deprecated
    NE("variable_requires"), // deprecated
    NE("variable_watch"),
    NE("while"),
    NE("write_file"), // deprecated
};

#if 0
static const named_element_t predefined_variables[] = {
    NE("APPLE"),
    NE("BORLAND"),
    NE("BUILD_SHARED_LIBS"),
    NE("CMAKE_ABSOLUTE_DESTINATION_FILES"),
    NE("CMAKE_ANDROID_ANT_ADDITIONAL_OPTIONS"),
    NE("CMAKE_ANDROID_API"),
    NE("CMAKE_ANDROID_API_MIN"),
    NE("CMAKE_ANDROID_ARCH"),
    NE("CMAKE_ANDROID_ASSETS_DIRECTORIES"),
    NE("CMAKE_ANDROID_GUI"),
    NE("CMAKE_ANDROID_JAR_DEPENDENCIES"),
    NE("CMAKE_ANDROID_JAR_DIRECTORIES"),
    NE("CMAKE_ANDROID_JAVA_SOURCE_DIR"),
    NE("CMAKE_ANDROID_NATIVE_LIB_DEPENDENCIES"),
    NE("CMAKE_ANDROID_NATIVE_LIB_DIRECTORIES"),
    NE("CMAKE_ANDROID_PROCESS_MAX"),
    NE("CMAKE_ANDROID_PROGUARD"),
    NE("CMAKE_ANDROID_PROGUARD_CONFIG_PATH"),
    NE("CMAKE_ANDROID_SECURE_PROPS_PATH"),
    NE("CMAKE_ANDROID_SKIP_ANT_STEP"),
    NE("CMAKE_ANDROID_STL_TYPE"),
    NE("CMAKE_APPBUNDLE_PATH"),
    NE("CMAKE_AR"),
    NE("CMAKE_ARCHIVE_OUTPUT_DIRECTORY"),
    NE("CMAKE_ARGC"),
    NE("CMAKE_ARGV0"),
    NE("CMAKE_AUTOMOC"),
    NE("CMAKE_AUTOMOC_MOC_OPTIONS"),
    NE("CMAKE_AUTOMOC_RELAXED_MODE"),
    NE("CMAKE_AUTORCC"),
    NE("CMAKE_AUTORCC_OPTIONS"),
    NE("CMAKE_AUTOUIC"),
    NE("CMAKE_AUTOUIC_OPTIONS"),
    NE("CMAKE_BACKWARDS_COMPATIBILITY"),
    NE("CMAKE_BINARY_DIR"),
    NE("CMAKE_BUILD_TOOL"),
    NE("CMAKE_BUILD_TYPE"),
    NE("CMAKE_BUILD_WITH_INSTALL_RPATH"),
    NE("CMAKE_CACHEFILE_DIR"),
    NE("CMAKE_CACHE_MAJOR_VERSION"),
    NE("CMAKE_CACHE_MINOR_VERSION"),
    NE("CMAKE_CACHE_PATCH_VERSION"),
    NE("CMAKE_CFG_INTDIR"),
    NE("CMAKE_CL_64"),
    NE("CMAKE_COLOR_MAKEFILE"),
    NE("CMAKE_COMMAND"),
    NE("CMAKE_COMPILER_2005"),
    NE("CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY"),
    NE("CMAKE_CONFIGURATION_TYPES"),
    NE("CMAKE_CROSSCOMPILING"),
    NE("CMAKE_CROSSCOMPILING_EMULATOR"),
    NE("CMAKE_CTEST_COMMAND"),
    NE("CMAKE_CURRENT_BINARY_DIR"),
    NE("CMAKE_CURRENT_LIST_DIR"),
    NE("CMAKE_CURRENT_LIST_FILE"),
    NE("CMAKE_CURRENT_LIST_LINE"),
    NE("CMAKE_CURRENT_SOURCE_DIR"),
    NE("CMAKE_CXX_COMPILE_FEATURES"),
    NE("CMAKE_CXX_EXTENSIONS"),
    NE("CMAKE_CXX_STANDARD"),
    NE("CMAKE_CXX_STANDARD_REQUIRED"),
    NE("CMAKE_C_COMPILE_FEATURES"),
    NE("CMAKE_C_EXTENSIONS"),
    NE("CMAKE_C_STANDARD"),
    NE("CMAKE_C_STANDARD_REQUIRED"),
    NE("CMAKE_DEBUG_POSTFIX"),
    NE("CMAKE_DEBUG_TARGET_PROPERTIES"),
    NE("CMAKE_DEPENDS_IN_PROJECT_ONLY"),
    NE("CMAKE_DL_LIBS"),
    NE("CMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES"),
    NE("CMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT"),
    NE("CMAKE_ECLIPSE_MAKE_ARGUMENTS"),
    NE("CMAKE_ECLIPSE_VERSION"),
    NE("CMAKE_EDIT_COMMAND"),
    NE("CMAKE_ENABLE_EXPORTS"),
    NE("CMAKE_ERROR_DEPRECATED"),
    NE("CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION"),
    NE("CMAKE_EXECUTABLE_SUFFIX"),
    NE("CMAKE_EXE_LINKER_FLAGS"),
    NE("CMAKE_EXPORT_COMPILE_COMMANDS"),
    NE("CMAKE_EXPORT_NO_PACKAGE_REGISTRY"),
    NE("CMAKE_EXTRA_GENERATOR"),
    NE("CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES"),
    NE("CMAKE_FIND_APPBUNDLE"),
    NE("CMAKE_FIND_FRAMEWORK"),
    NE("CMAKE_FIND_LIBRARY_PREFIXES"),
    NE("CMAKE_FIND_LIBRARY_SUFFIXES"),
    NE("CMAKE_FIND_NO_INSTALL_PREFIX"),
    NE("CMAKE_FIND_PACKAGE_NAME"),
    NE("CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY"),
    NE("CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY"),
    NE("CMAKE_FIND_PACKAGE_WARN_NO_MODULE"),
    NE("CMAKE_FIND_ROOT_PATH"),
    NE("CMAKE_FIND_ROOT_PATH_MODE_INCLUDE"),
    NE("CMAKE_FIND_ROOT_PATH_MODE_LIBRARY"),
    NE("CMAKE_FIND_ROOT_PATH_MODE_PACKAGE"),
    NE("CMAKE_FIND_ROOT_PATH_MODE_PROGRAM"),
    NE("CMAKE_FRAMEWORK_PATH"),
    NE("CMAKE_Fortran_FORMAT"),
    NE("CMAKE_Fortran_MODDIR_DEFAULT"),
    NE("CMAKE_Fortran_MODDIR_FLAG"),
    NE("CMAKE_Fortran_MODOUT_FLAG"),
    NE("CMAKE_Fortran_MODULE_DIRECTORY"),
    NE("CMAKE_GENERATOR"),
    NE("CMAKE_GENERATOR_PLATFORM"),
    NE("CMAKE_GENERATOR_TOOLSET"),
    NE("CMAKE_GNUtoMS"),
    NE("CMAKE_HOME_DIRECTORY"),
    NE("CMAKE_HOST_APPLE"),
    NE("CMAKE_HOST_SOLARIS"),
    NE("CMAKE_HOST_SYSTEM"),
    NE("CMAKE_HOST_SYSTEM_NAME"),
    NE("CMAKE_HOST_SYSTEM_PROCESSOR"),
    NE("CMAKE_HOST_SYSTEM_VERSION"),
    NE("CMAKE_HOST_UNIX"),
    NE("CMAKE_HOST_WIN32"),
    NE("CMAKE_IGNORE_PATH"),
    NE("CMAKE_IMPORT_LIBRARY_PREFIX"),
    NE("CMAKE_IMPORT_LIBRARY_SUFFIX"),
    NE("CMAKE_INCLUDE_CURRENT_DIR"),
    NE("CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE"),
    NE("CMAKE_INCLUDE_DIRECTORIES_BEFORE"),
    NE("CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE"),
    NE("CMAKE_INCLUDE_PATH"),
    NE("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME"),
    NE("CMAKE_INSTALL_MESSAGE"),
    NE("CMAKE_INSTALL_NAME_DIR"),
    NE("CMAKE_INSTALL_PREFIX"),
    NE("CMAKE_INSTALL_RPATH"),
    NE("CMAKE_INSTALL_RPATH_USE_LINK_PATH"),
    NE("CMAKE_INTERNAL_PLATFORM_ABI"),
    NE("CMAKE_IOS_INSTALL_COMBINED"),
    NE("CMAKE_JOB_POOL_COMPILE"),
    NE("CMAKE_JOB_POOL_LINK"),
    NE("CMAKE_LIBRARY_ARCHITECTURE"),
    NE("CMAKE_LIBRARY_ARCHITECTURE_REGEX"),
    NE("CMAKE_LIBRARY_OUTPUT_DIRECTORY"),
    NE("CMAKE_LIBRARY_PATH"),
    NE("CMAKE_LIBRARY_PATH_FLAG"),
    NE("CMAKE_LINK_DEF_FILE_FLAG"),
    NE("CMAKE_LINK_DEPENDS_NO_SHARED"),
    NE("CMAKE_LINK_INTERFACE_LIBRARIES"),
    NE("CMAKE_LINK_LIBRARY_FILE_FLAG"),
    NE("CMAKE_LINK_LIBRARY_FLAG"),
    NE("CMAKE_LINK_LIBRARY_SUFFIX"),
    NE("CMAKE_LINK_SEARCH_END_STATIC"),
    NE("CMAKE_LINK_SEARCH_START_STATIC"),
    NE("CMAKE_MACOSX_BUNDLE"),
    NE("CMAKE_MACOSX_RPATH"),
    NE("CMAKE_MAJOR_VERSION"),
    NE("CMAKE_MAKE_PROGRAM"),
    NE("CMAKE_MATCH_COUNT"),
    NE("CMAKE_MFC_FLAG"),
    NE("CMAKE_MINIMUM_REQUIRED_VERSION"),
    NE("CMAKE_MINOR_VERSION"),
    NE("CMAKE_MODULE_LINKER_FLAGS"),
    NE("CMAKE_MODULE_PATH"),
    NE("CMAKE_NINJA_OUTPUT_PATH_PREFIX"),
    NE("CMAKE_NOT_USING_CONFIG_FLAGS"),
    NE("CMAKE_NO_BUILTIN_CHRPATH"),
    NE("CMAKE_NO_SYSTEM_FROM_IMPORTED"),
    NE("CMAKE_OBJECT_PATH_MAX"),
    NE("CMAKE_OSX_ARCHITECTURES"),
    NE("CMAKE_OSX_DEPLOYMENT_TARGET"),
    NE("CMAKE_OSX_SYSROOT"),
    NE("CMAKE_PARENT_LIST_FILE"),
    NE("CMAKE_PATCH_VERSION"),
    NE("CMAKE_PDB_OUTPUT_DIRECTORY"),
    NE("CMAKE_POSITION_INDEPENDENT_CODE"),
    NE("CMAKE_PREFIX_PATH"),
    NE("CMAKE_PROGRAM_PATH"),
    NE("CMAKE_PROJECT_NAME"),
    NE("CMAKE_RANLIB"),
    NE("CMAKE_ROOT"),
    NE("CMAKE_RUNTIME_OUTPUT_DIRECTORY"),
    NE("CMAKE_SCRIPT_MODE_FILE"),
    NE("CMAKE_SHARED_LIBRARY_PREFIX"),
    NE("CMAKE_SHARED_LIBRARY_SUFFIX"),
    NE("CMAKE_SHARED_LINKER_FLAGS"),
    NE("CMAKE_SHARED_MODULE_PREFIX"),
    NE("CMAKE_SHARED_MODULE_SUFFIX"),
    NE("CMAKE_SIZEOF_VOID_P"),
    NE("CMAKE_SKIP_BUILD_RPATH"),
    NE("CMAKE_SKIP_INSTALL_ALL_DEPENDENCY"),
    NE("CMAKE_SKIP_INSTALL_RPATH"),
    NE("CMAKE_SKIP_INSTALL_RULES"),
    NE("CMAKE_SKIP_RPATH"),
    NE("CMAKE_SOURCE_DIR"),
    NE("CMAKE_STAGING_PREFIX"),
    NE("CMAKE_STATIC_LIBRARY_PREFIX"),
    NE("CMAKE_STATIC_LIBRARY_SUFFIX"),
    NE("CMAKE_STATIC_LINKER_FLAGS"),
    NE("CMAKE_SYSROOT"),
    NE("CMAKE_SYSTEM"),
    NE("CMAKE_SYSTEM_APPBUNDLE_PATH"),
    NE("CMAKE_SYSTEM_FRAMEWORK_PATH"),
    NE("CMAKE_SYSTEM_IGNORE_PATH"),
    NE("CMAKE_SYSTEM_INCLUDE_PATH"),
    NE("CMAKE_SYSTEM_LIBRARY_PATH"),
    NE("CMAKE_SYSTEM_NAME"),
    NE("CMAKE_SYSTEM_PREFIX_PATH"),
    NE("CMAKE_SYSTEM_PROCESSOR"),
    NE("CMAKE_SYSTEM_PROGRAM_PATH"),
    NE("CMAKE_SYSTEM_VERSION"),
    NE("CMAKE_TOOLCHAIN_FILE"),
    NE("CMAKE_TRY_COMPILE_CONFIGURATION"),
    NE("CMAKE_TRY_COMPILE_PLATFORM_VARIABLES"),
    NE("CMAKE_TRY_COMPILE_TARGET_TYPE"),
    NE("CMAKE_TWEAK_VERSION"),
    NE("CMAKE_USER_MAKE_RULES_OVERRIDE"),
    NE("CMAKE_USE_RELATIVE_PATHS"),
    NE("CMAKE_VERBOSE_MAKEFILE"),
    NE("CMAKE_VERSION"),
    NE("CMAKE_VISIBILITY_INLINES_HIDDEN"),
    NE("CMAKE_VS_DEVENV_COMMAND"),
    NE("CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD"),
    NE("CMAKE_VS_INTEL_Fortran_PROJECT_VERSION"),
    NE("CMAKE_VS_MSBUILD_COMMAND"),
    NE("CMAKE_VS_NsightTegra_VERSION"),
    NE("CMAKE_VS_PLATFORM_NAME"),
    NE("CMAKE_VS_PLATFORM_TOOLSET"),
    NE("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION"),
    NE("CMAKE_WARN_DEPRECATED"),
    NE("CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION"),
    NE("CMAKE_WIN32_EXECUTABLE"),
    NE("CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS"),
    NE("CMAKE_XCODE_PLATFORM_TOOLSET"),
    NE("CYGWIN"),
    NE("ENV"),
    NE("EXECUTABLE_OUTPUT_PATH"),
    NE("GHS-MULTI"),
    NE("LIBRARY_OUTPUT_PATH"),
    NE("MINGW"),
    NE("MSVC"),
    NE("MSVC10"),
    NE("MSVC11"),
    NE("MSVC12"),
    NE("MSVC14"),
    NE("MSVC60"),
    NE("MSVC70"),
    NE("MSVC71"),
    NE("MSVC80"),
    NE("MSVC90"),
    NE("MSVC_IDE"),
    NE("MSVC_VERSION"),
    NE("PROJECT_BINARY_DIR"),
    NE("PROJECT_NAME"),
    NE("PROJECT_SOURCE_DIR"),
    NE("PROJECT_VERSION"),
    NE("PROJECT_VERSION_MAJOR"),
    NE("PROJECT_VERSION_MINOR"),
    NE("PROJECT_VERSION_PATCH"),
    NE("PROJECT_VERSION_TWEAK"),
    NE("UNIX"),
    NE("WIN32"),
    NE("WINCE"),
    NE("WINDOWS_PHONE"),
    NE("WINDOWS_STORE"),
    NE("XCODE_VERSION"),
};
#endif

/**
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

    (void) ctxt;
    (void) options;
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
property = [A-Z][A-Z_]*;

<INITIAL>"#" bracket_open {
    YYCTYPE *end;
    size_t bracket_len = YYLENG - 1;
    char bracket_close[bracket_len];

    memcpy(bracket_close, YYTEXT + STR_LEN("#"), bracket_len);
    bracket_close[0] = bracket_close[bracket_len - 1] = ']';
    if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, bracket_close, bracket_len, (const char *) YYLIMIT))) {
        YYCURSOR = YYLIMIT;
    } else {
        YYCURSOR = end + bracket_len;
    }
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> bracket_open {
    mydata->bracket_len = YYLENG;
    BEGIN(IN_BRACKET_STRING);
    TOKEN(STRING_DOUBLE);
}

<IN_BRACKET_STRING> bracket_close {
    int old_state;

    old_state = YYSTATE;
    if (YYLENG == mydata->bracket_len) {
        BEGIN(INITIAL);
    }
    TOKEN(default_token_type[old_state]);
}

<INITIAL> ["] {
    BEGIN(IN_QUOTED_ARGUMENT);
    TOKEN(STRING_DOUBLE);
}

<IN_QUOTED_ARGUMENT> "\\" [()#" \\$@^trn;] {
    TOKEN(ESCAPED_CHAR);
}

// <INITIAL,IN_QUOTED_ARGUMENT,IN_BRACKET_STRING,IN_VARIABLE_REFERENCE>
<*>"${" {
    PUSH_STATE(IN_VARIABLE_REFERENCE);
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_VARIABLE_REFERENCE> "}" {
    POP_STATE();
    TOKEN(SEQUENCE_INTERPOLATED);
}

<IN_QUOTED_ARGUMENT> ["] {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> "#" {
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

<INITIAL> identifier space* "(" {
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

/*
<INITIAL> "$" identifier {
    TOKEN(NAME_VARIABLE);
}
*/

/*
<INITIAL> property {
    TOKEN(NAME);
}
*/

<*> "$<" {
    PUSH_STATE(IN_EXPRESSION);
    TOKEN(TAG_PREPROC);
}

<IN_EXPRESSION> ">" {
    POP_STATE();
    TOKEN(TAG_PREPROC);
}

<IN_EXPRESSION> [:] {
    TOKEN(PUNCTUATION);
}

<INITIAL> [()] {
    TOKEN(PUNCTUATION);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation cmake_lexer = {
    "CMake",
    "Lexer for CMake files",
    NULL, // aliases
    (const char * const []) { "CMakeLists.txt", "*.cmake", NULL },
    (const char * const []) { "text/x-cmake", NULL },
    NULL, // interpreters (shebang)
    NULL, // analyse
    NULL, // init
    cmakelex,
    NULL, // finalize
    sizeof(CMakeLexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
