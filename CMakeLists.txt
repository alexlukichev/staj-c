cmake_minimum_required(VERSION 2.8)

project(staj-c C)

enable_testing()

set(SRC staj.c staj.h staj_errors.h)
add_library(staj STATIC ${SRC})
target_include_directories(staj PUBLIC .)

add_executable(test_staj test_staj.c)
target_link_libraries(test_staj staj)
add_test(test_staj ${CMAKE_CURRENT_BINARY_DIR}/test_staj)
