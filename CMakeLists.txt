cmake_minimum_required(VERSION 3.24)

project(picoc
        VERSION 4.0
        DESCRIPTION "PicoC Interpreter"
        LANGUAGES C CXX ASM
)

# always generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(picoc_generate_include target name source output)
    add_custom_target(generate-${name} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${output})
    cmake_path(GET output PARENT_PATH output-dir)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output-dir})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${output}
        COMMENT "Generating ${output}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        COMMAND xxd -i < ${CMAKE_CURRENT_SOURCE_DIR}/${source} > ${CMAKE_CURRENT_SOURCE_DIR}/${output}
        VERBATIM
    )
    add_dependencies(${target} generate-${name})
endfunction(picoc_generate_include)

###################
# PicoC static library
###################
add_library(picoclib STATIC)

target_compile_definitions(picoclib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

target_compile_definitions(picoclib PUBLIC
    UNIX_HOST
    VER="xxx"       # FIXME: `git show-ref --abbrev=8 --head --hash head`
    TAG="3.2.2"
)

target_include_directories(picoclib PUBLIC
    src/include
)

target_include_directories(picoclib PRIVATE
    src/interpreter
    src/cstdlib
)

target_sources(picoclib PRIVATE
    src/interpreter/table.c
    src/interpreter/lex.c
    src/interpreter/parse.c
    src/interpreter/expression.c
    src/interpreter/heap.c
    src/interpreter/type.c
	src/interpreter/variable.c
	src/interpreter/clibrary.c
	src/interpreter/platform.c
	src/interpreter/include.c
	src/interpreter/debug.c
   	src/cstdlib/stdio.c
    src/cstdlib/math.c
    src/cstdlib/string.c
    src/cstdlib/stdlib.c
    src/cstdlib/time.c
    src/cstdlib/errno.c
    src/cstdlib/ctype.c
    src/cstdlib/stdbool.c
    src/cstdlib/unistd.c
    src/platform/platform_unix.c
    src/platform/library_unix.c
)

###################
# PicoC Command Line
###################
add_executable(picoc)
target_sources(picoc PRIVATE
    src/cli/picoc.c
)
target_link_libraries(picoc
    picoclib
    m
    readline
)
picoc_generate_include(picoc license LICENSE src/cli/generated/LICENSE.h)
target_include_directories(picoc PUBLIC
    src/cli/generated
)

###################
# PicoC tests
###################
set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
include(CTest)
add_custom_target(tests)
add_dependencies(tests picoc)
function(picoc_add_testfile suite path file)
endfunction(picoc_add_testfile)

function(picoc_test_suite suite)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "" "PATH" "TESTFILES"
    )
    add_custom_target(tests-${suite})
    foreach(test IN LISTS arg_TESTFILES)
        picoc_add_testfile(${suite} ${arg_PATH} ${test})
        add_test(NAME test-${suite}-${test}
            COMMAND ${CMAKE_SOURCE_DIR}/tests/runtest.sh
                ${CMAKE_BINARY_DIR}/picoc
                ${CMAKE_CURRENT_LIST_DIR}/${arg_PATH}/${test}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${arg_PATH}
        )
    endforeach()
endfunction(picoc_test_suite)

add_subdirectory(tests)
