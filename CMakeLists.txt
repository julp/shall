cmake_minimum_required(VERSION 2.8.8)

project(shall C)

set(SHALL_VERSION_MAJOR 0)
set(SHALL_VERSION_MINOR 0)
set(SHALL_VERSION_PATCH 0)

math(EXPR SHALL_VERSION_NUMBER "${SHALL_VERSION_MAJOR} * 1000 + ${SHALL_VERSION_MINOR} * 100 + ${SHALL_VERSION_PATCH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=gnu99 -Wwrite-strings -Wstrict-prototypes -Wuninitialized -Wunreachable-code -Wno-comment -Wnonnull -Wunreachable-code")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(CheckIncludeFiles)
check_include_files("inttypes.h" HAVE_INTTYPES_H)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdbool.h" HAVE_STDBOOL_H)
include(CheckFunctionExists)
check_function_exists("fnmatch" HAVE_FNMATCH)
check_function_exists("stpcpy" HAVE_STPCPY)

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

# TODO: commented because HashTable conflicts with PHP binding
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG -DTEST)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

set(COMMON_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/lib/;${PROJECT_SOURCE_DIR}/shared/;${PROJECT_SOURCE_DIR}/include/;${PROJECT_BINARY_DIR}")

set(SOURCES #
    lib/lexer.c lib/formatter.c lib/highlight.c lib/options.c lib/themes.c lib/tokens.c lib/lexers/helpers.c lib/version.c lib/encoding.c
    lib/darray.c lib/dlist.c
    shared/xtring.c shared/hashtable.c
)
set(THEMES monokai molokai)
set(FORMATTERS html terminal plain)
set(C_LEXERS text)
# set(C_LEXERS annotations diff text)
set(RE2C_LEXERS apache c cmake css dtd js json nginx pgsql php ruby varnish xml)

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

foreach(fmt ${FORMATTERS})
    list(APPEND SOURCES "${PROJECT_SOURCE_DIR}/lib/formatters/${fmt}.c")
endforeach(fmt)

configure_file(
    "${PROJECT_SOURCE_DIR}/shall.h.in"
    "${PROJECT_BINARY_DIR}/shall.h"
    @ONLY
)

add_library(common OBJECT EXCLUDE_FROM_ALL shared/utils.c)
set_target_properties(common PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(common_cli OBJECT EXCLUDE_FROM_ALL cli/shared/optparse.c)
set_target_properties(common_cli PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(shall_lib SHARED ${SOURCES} $<TARGET_OBJECTS:common>)

# add_library(shall_lib SHARED $<TARGET_OBJECTS:common>)
# add_executable(shall_bin $<TARGET_OBJECTS:common> cli/bin/shall.c)

add_executable(shall_bin cli/bin/shall.c shared/hashtable.c $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shall_bin shall_lib)

add_executable(shalltest cli/bin/shalltest.c $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shalltest shall_lib)

add_executable(shalldoc cli/bin/shalldoc.c $<TARGET_OBJECTS:common>)
target_link_libraries(shalldoc shall_lib)

set_target_properties(shall_lib PROPERTIES
    INCLUDE_DIRECTORIES "${COMMON_INCLUDE_DIRECTORIES}"
    PUBLIC_HEADER "${PROJECT_BINARY_DIR}/shall.h;include/themes.h;shared/cpp.h;include/tokens.h;include/keywords.h;include/formatter.h;include/xtring.h;include/types.h;include/bool.h;include/xtring.h"
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
# if(DOXYGEN_FOUND)
#     add_custom_target(api_doc ALL doxygen ${PROJECT_SOURCE_DIR}/doxygen.conf DEPENDS ${PROJECT_SOURCE_DIR}/lib/shall.c ${PROJECT_SOURCE_DIR}/doc.h)
# endif(DOXYGEN_FOUND)

install(
    TARGETS shall_lib shall_bin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/shall
)

# enable_testing()
# add_subdirectory(UT)
add_custom_target(check COMMAND find ${PROJECT_SOURCE_DIR}/UT -name '*.ssc' -exec ${PROJECT_BINARY_DIR}/shalltest {} "\;")
