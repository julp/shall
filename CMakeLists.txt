cmake_minimum_required(VERSION 2.8.8)

project(shall C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -DDEBUG -DTEST -Wall -Wextra -std=gnu99 -Wwrite-strings -Wstrict-prototypes -Wuninitialized -Wunreachable-code -Wno-comment -Wnonnull -Wunreachable-code")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(RE2C 0.13.7 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES shall.c xtring.c darray.c)# hashtable.c)
set(FORMATTERS html xml terminal plain)
set(C_LEXERS text diff)
set(RE2C_LEXERS apache c cmake css dtd json nginx pgsql php ruby varnish xml)

foreach(lexer ${C_LEXERS})
    list(APPEND SOURCES "lexers/${lexer}.c")
endforeach(lexer)

foreach(lexer ${RE2C_LEXERS})
    list(APPEND SOURCES "lexers/${lexer}.c")
    RE2C_TARGET(NAME "${lexer}" INPUT "lexers/${lexer}.re" OUTPUT "lexers/${lexer}.c" OPTIONS "-cd8" DEPENDS "lexer-private.h")
endforeach(lexer)

foreach(fmt ${FORMATTERS})
    list(APPEND SOURCES "formatters/${fmt}.c")
endforeach(fmt)

add_library(common OBJECT EXCLUDE_FROM_ALL utils.c)
set_target_properties(common PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(common_cli OBJECT EXCLUDE_FROM_ALL optparse.c)
set_target_properties(common_cli PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(shall_lib SHARED ${SOURCES} $<TARGET_OBJECTS:common>)

# add_library(shall_lib SHARED $<TARGET_OBJECTS:common>)
# add_executable(shall_bin $<TARGET_OBJECTS:common> bin/shall.c)
add_executable(shall_bin bin/shall.c hashtable.c $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shall_bin shall_lib)

add_executable(shalltest bin/shalltest.c $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:common_cli>)
target_link_libraries(shalltest shall_lib)

add_executable(shalldoc bin/shalldoc.c $<TARGET_OBJECTS:common>)
target_link_libraries(shalldoc shall_lib)

set_target_properties(shall_lib PROPERTIES PUBLIC_HEADER "shall.h;cpp.h;tokens.h;keywords.h;lexer.h;formatter.h;option.h")

foreach(target "shall_lib;shall_bin")
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "shall")
endforeach(target)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexers/README.md
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shalldoc --lexers ${CMAKE_CURRENT_SOURCE_DIR}/lexers/README.md
    DEPENDS shalldoc
    COMMENT "Build documentation (shalldoc) for lexers"
)
add_custom_target(
    lexers_doc
    ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexers/README.md
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/formatters/README.md
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shalldoc --formatters ${CMAKE_CURRENT_SOURCE_DIR}/formatters/README.md
    DEPENDS shalldoc
    COMMENT "Build documentation (shalldoc) for formatters"
)
add_custom_target(
    formatters_doc
    ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/formatters/README.md
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shalldoc --rdoc ${CMAKE_CURRENT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
    DEPENDS shalldoc
    COMMENT "Build documentation (shalldoc) for ruby"
)
add_custom_target(
    rdoc
    ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bindings/ruby/doc/shall.rb
)

# find_program(DOXYGEN_EXECUTABLE doxygen)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(api_doc ALL doxygen ${CMAKE_SOURCE_DIR}/doxygen.conf DEPENDS ${CMAKE_SOURCE_DIR}/shall.c ${CMAKE_SOURCE_DIR}/shall.h ${CMAKE_SOURCE_DIR}/doc.h) 
endif(DOXYGEN_FOUND)

install(
    TARGETS shall_lib shall_bin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/shall
)

# enable_testing()
# add_subdirectory(UT)
add_custom_target(check COMMAND find ${CMAKE_SOURCE_DIR}/UT -name '*.ssc' -exec ${PROJECT_BINARY_DIR}/shalltest {} "\;")
