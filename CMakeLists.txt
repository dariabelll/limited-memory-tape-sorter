cmake_minimum_required(VERSION 3.16)

project(limited_memory_tape_sorter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

function(set_project_warnings target_name)
    target_compile_options(${target_name}
        PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
            $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )

    if (ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(${target_name}
            PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>
                $<$<CXX_COMPILER_ID:MSVC>:/WX>
        )
    endif()
endfunction()

file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*[/\\\\]main\\.cpp$")

add_library(limited_memory_tape_sorter_lib
    ${PROJECT_SOURCES}
)

target_include_directories(limited_memory_tape_sorter_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_project_warnings(limited_memory_tape_sorter_lib)

add_executable(limited-memory-tape-sorter
    src/main.cpp
)

target_link_libraries(limited-memory-tape-sorter
    PRIVATE
        limited_memory_tape_sorter_lib
)

set_project_warnings(limited-memory-tape-sorter)

if (BUILD_TESTS)
    include(FetchContent)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    enable_testing()

    file(GLOB_RECURSE TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    if (TEST_SOURCES)
        add_executable(limited-memory-tape-sorter-tests
            ${TEST_SOURCES}
        )

        target_link_libraries(limited-memory-tape-sorter-tests
            PRIVATE
                limited_memory_tape_sorter_lib
                GTest::gtest_main
        )

        target_include_directories(limited-memory-tape-sorter-tests
            PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )

        set_project_warnings(limited-memory-tape-sorter-tests)

        include(GoogleTest)

        gtest_discover_tests(limited-memory-tape-sorter-tests)
    endif()

    add_test(
        NAME integration_sort_cli
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/sort_cli.py"
            --exe "$<TARGET_FILE:limited-memory-tape-sorter>"
            --work-dir "${CMAKE_CURRENT_BINARY_DIR}/integration_tests"
    )

    set_tests_properties(integration_sort_cli
        PROPERTIES
            DEPENDS limited-memory-tape-sorter
    )
endif()
