cmake_minimum_required(VERSION 2.8.8)

project(shall C)

set(SHALL_VERSION_MAJOR 0)
set(SHALL_VERSION_MINOR 0)
set(SHALL_VERSION_PATCH 0)

math(EXPR SHALL_VERSION_NUMBER "${SHALL_VERSION_MAJOR} * 1000 + ${SHALL_VERSION_MINOR} * 100 + ${SHALL_VERSION_PATCH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=gnu99 -Wwrite-strings -Wstrict-prototypes -Wuninitialized -Wunreachable-code -Wno-comment -Wnonnull -Wunreachable-code")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCSourceCompiles)

check_include_files("inttypes.h" HAVE_INTTYPES_H)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdbool.h" HAVE_STDBOOL_H)

check_function_exists("fnmatch" HAVE_FNMATCH)
check_function_exists("stpcpy" HAVE_STPCPY)

set(COMMON_INCLUDE_DIRECTORIES #
    ${PROJECT_SOURCE_DIR}/lib/
    ${PROJECT_SOURCE_DIR}/shared/
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_BINARY_DIR}
)
set(SHALL_LIB_INCLUDE_DIRS ${COMMON_INCLUDE_DIRECTORIES})

find_package(BISON REQUIRED)

set(WITH_ICU false)
set(WITH_ICONV false)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(ICU COMPONENTS uc i18n QUIET)
if(NOT ICU_FOUND)
    find_path(ICONV_INCLUDE_DIR iconv.h)
    find_library(ICONV_LIBRARIES NAMES iconv libiconv c)
    if(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
        set(WITH_ICONV true)
        list(APPEND SHALL_LIB_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
        set(CMAKE_REQUIRED_FLAGS "-Werror")
        check_c_source_compiles("
        #include <iconv.h>
        int main(void) {
            iconv_t conv = 0;
            const char *in = 0;
            size_t ilen = 0;
            char *out = 0;
            size_t olen = 0;
            iconv(conv, &in, &ilen, &out, &olen);
            return 0;
        }
        " ICONV_SECOND_ARGUMENT_IS_CONST)
        if(ICONV_SECOND_ARGUMENT_IS_CONST)
            set(ICONV_CONST "const")
        else(ICONV_SECOND_ARGUMENT_IS_CONST)
            set(ICONV_CONST "")
        endif(ICONV_SECOND_ARGUMENT_IS_CONST)
        check_library_exists("iconv" "iconvctl" ${ICONV_LIBRARIES} HAVE_ICONVCTL)
    endif(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
else(NOT ICU_FOUND)
    set(WITH_ICU true)
    list(APPEND SHALL_LIB_INCLUDE_DIRS ${ICU_INCLUDE_DIRS})
endif(NOT ICU_FOUND)

get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)
if("${srcdir}" STREQUAL "${bindir}")
    message("Don't build ${CMAKE_PROJECT_NAME} into its source directory")
    message("Use a temporary directory instead:")
    message("  mkdir -p /my/tmp/build/path/")
    message("  cd /my/tmp/build/path/")
    message("  cmake /path/to/shall/sources # your extra arguments")
    message(FATAL_ERROR "")
endif("${srcdir}" STREQUAL "${bindir}")

find_package(RE2C 0.13.7 REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    # TODO: -DTEST disabled because HashTable conflicts with PHP binding
#     add_definitions(-DDEBUG -DTEST)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

set(SOURCES #
    lib/lexer.c lib/formatter.c lib/highlight.c lib/options.c lib/themes.c lib/tokens.c lib/lexers/helpers.c lib/version.c lib/encoding.c lib/iterator.c
    lib/darray.c lib/dlist.c
    shared/xtring.c shared/hashtable.c
)
set(THEMES monokai molokai)
set(FORMATTERS bbcode html plain rtf terminal)
set(C_LEXERS cpp diff text) # annotations
set(RE2C_LEXERS apache c cmake css dtd js json nginx pgsql php python ruby varnish xml)
set(BISON_GRAMMARS )

foreach(lexer ${C_LEXERS})
    list(APPEND SOURCES "${PROJECT_SOURCE_DIR}/lib/lexers/${lexer}.c")
endforeach(lexer)

foreach(theme ${THEMES})
    list(APPEND SOURCES "${PROJECT_SOURCE_DIR}/lib/themes/${theme}.c")
endforeach(theme)

set(RE2C_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/genlexers/")
file(MAKE_DIRECTORY "${RE2C_OUTPUT_DIRECTORY}")
foreach(lexer ${RE2C_LEXERS})
    list(APPEND SOURCES "${RE2C_OUTPUT_DIRECTORY}/${lexer}.c")
    RE2C_TARGET(NAME "${lexer}" INPUT "${PROJECT_SOURCE_DIR}/lib/lexers/${lexer}.re" OUTPUT "${RE2C_OUTPUT_DIRECTORY}/${lexer}.c" OPTIONS "-cd8" DEPENDS "${PROJECT_SOURCE_DIR}/lib/lexer.h")
endforeach(lexer)

set(BISON_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/gengrammars/")
file(MAKE_DIRECTORY "${BISON_OUTPUT_DIRECTORY}")
foreach(grammar ${BISON_GRAMMARS})
    list(APPEND SOURCES "${BISON_OUTPUT_DIRECTORY}/${grammar}.c")
    BISON_TARGET("${grammar}" "${PROJECT_SOURCE_DIR}/lib/lexers/${grammar}.y" "${BISON_OUTPUT_DIRECTORY}/${grammar}.c" COMPILE_FLAGS "-dp ${grammar}")
endforeach(grammar)

foreach(fmt ${FORMATTERS})
    list(APPEND SOURCES "${PROJECT_SOURCE_DIR}/lib/formatters/${fmt}.c")
endforeach(fmt)

# public
configure_file(
    "${PROJECT_SOURCE_DIR}/vernum.h.in"
    "${PROJECT_BINARY_DIR}/vernum.h"
    @ONLY
)

# private
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    @ONLY
)

add_library(common OBJECT EXCLUDE_FROM_ALL shared/utils.c)
set_target_properties(common PROPERTIES COMPILE_FLAGS "-fPIC" INCLUDE_DIRECTORIES "${COMMON_INCLUDE_DIRECTORIES}")

add_library(common_cli OBJECT EXCLUDE_FROM_ALL cli/shared/optparse.c cli/shared/lexer_group.c)
set_target_properties(common_cli PROPERTIES COMPILE_FLAGS "-fPIC" INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/cli/shared/;${COMMON_INCLUDE_DIRECTORIES}")

add_library(shall_lib SHARED ${SOURCES} $<TARGET_OBJECTS:common>)
if(WITH_ICU OR WITH_ICONV)
    target_link_libraries(shall_lib ${ICONV_LIBRARIES} ${ICU_LIBRARIES})
endif(WITH_ICU OR WITH_ICONV)

# add_library(shall_lib SHARED $<TARGET_OBJECTS:common>)
# add_executable(shall_bin $<TARGET_OBJECTS:common> cli/bin/shall.c)

add_executable(shall_bin cli/bin/shall.c shared/hashtable.c $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shall_bin shall_lib)

add_executable(shalltest cli/bin/shalltest.c $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shalltest shall_lib)

add_executable(shalldoc cli/bin/shalldoc.c $<TARGET_OBJECTS:common>)
target_link_libraries(shalldoc shall_lib)

set(SHALL_PUBLIC_HEADERS
    ${PROJECT_BINARY_DIR}/vernum.h
    ${PROJECT_SOURCE_DIR}/include/version.h
    ${PROJECT_SOURCE_DIR}/include/shall.h
    ${PROJECT_SOURCE_DIR}/include/themes.h
    ${PROJECT_SOURCE_DIR}/include/tokens.h
    ${PROJECT_SOURCE_DIR}/include/keywords.h
    ${PROJECT_SOURCE_DIR}/include/formatter.h
    ${PROJECT_SOURCE_DIR}/include/xtring.h
    ${PROJECT_SOURCE_DIR}/include/types.h
    ${PROJECT_SOURCE_DIR}/include/options.h
    ${PROJECT_SOURCE_DIR}/include/iterator.h
    ${PROJECT_SOURCE_DIR}/include/machine.h
)

set_target_properties(shall_lib PROPERTIES
    INCLUDE_DIRECTORIES "${SHALL_LIB_INCLUDE_DIRS}"
    PUBLIC_HEADER "${SHALL_PUBLIC_HEADERS}"
)
set_target_properties(shall_bin shalltest shalldoc PROPERTIES INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/cli/shared/;${COMMON_INCLUDE_DIRECTORIES}")

foreach(target "shall_lib;shall_bin")
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "shall")
endforeach(target)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/lib/lexers/README.md
    COMMAND ${PROJECT_BINARY_DIR}/shalldoc --lexers ${PROJECT_SOURCE_DIR}/lib/lexers/README.md
    DEPENDS ${PROJECT_BINARY_DIR}/shalldoc
    COMMENT "Build documentation (shalldoc) for lexers"
)
add_custom_target(
    lexers_doc
    ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/lib/lexers/README.md
)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/lib/formatters/README.md
    COMMAND ${PROJECT_BINARY_DIR}/shalldoc --formatters ${PROJECT_SOURCE_DIR}/lib/formatters/README.md
    DEPENDS ${PROJECT_BINARY_DIR}/shalldoc
    COMMENT "Build documentation (shalldoc) for formatters"
)
add_custom_target(
    formatters_doc
    ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/lib/formatters/README.md
)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
    COMMAND ${PROJECT_BINARY_DIR}/shalldoc --rdoc ${PROJECT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
    DEPENDS ${PROJECT_BINARY_DIR}/shalldoc
    COMMENT "Build documentation (shalldoc) for ruby"
)
add_custom_target(
    rdoc
    ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
)

# find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(
        "${PROJECT_SOURCE_DIR}/doxygen.conf.in"
        "${PROJECT_BINARY_DIR}/doxygen.conf"
        @ONLY
    )
    add_custom_target(api_doc ALL doxygen ${PROJECT_BINARY_DIR}/doxygen.conf DEPENDS ${PROJECT_SOURCE_DIR}/doc.h)
endif(DOXYGEN_FOUND)

install(
    TARGETS shall_lib shall_bin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/shall
)

# enable_testing()
# add_subdirectory(UT)
add_custom_target(check COMMAND find ${PROJECT_SOURCE_DIR}/UT -name '*.ssc' -exec ${PROJECT_BINARY_DIR}/shalltest {} "\;")
