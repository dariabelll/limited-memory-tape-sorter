cmake_minimum_required(VERSION 3.16)

project(limited_memory_tape_sorter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(limited_memory_tape_sorter_lib
    ${PROJECT_SOURCES}
)

target_include_directories(limited_memory_tape_sorter_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(limited_memory_tape_sorter_lib
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if (ENABLE_WARNINGS_AS_ERRORS)
    target_compile_options(limited_memory_tape_sorter_lib
        PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>
            $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
endif()

add_executable(limited-memory-tape-sorter
    src/main.cpp
)

target_link_libraries(limited-memory-tape-sorter
    PRIVATE
        limited_memory_tape_sorter_lib
)

target_compile_options(limited-memory-tape-sorter
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if (BUILD_TESTS)
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
        )

        target_compile_options(limited-memory-tape-sorter-tests
            PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
        )

        add_test(
            NAME limited-memory-tape-sorter-tests
            COMMAND limited-memory-tape-sorter-tests
        )
    endif()
endif()
