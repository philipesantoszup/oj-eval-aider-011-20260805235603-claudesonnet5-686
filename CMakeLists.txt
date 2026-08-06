cmake_minimum_required(VERSION 3.10)
project(stlite_priority_queue CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

set(TEST_DIRS
    one
    one.memcheck
    two
    two.memcheck
    three
    three.memcheck
    four
    four.memcheck
    five
    five.memcheck
    six
    six.memcheck
)

enable_testing()

foreach(dir ${TEST_DIRS})
    string(REPLACE "." "_" target_name ${dir})
    set(target_name "test_${target_name}")
    add_executable(${target_name} data/${dir}/code.cpp)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()
