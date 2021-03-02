cmake_minimum_required(VERSION 3.5.1)

set(cxxflags -Wall -Wextra -Werror -Wfatal-errors)

find_package(FLEX)

flex_target(ispc-flex
    "${CMAKE_CURRENT_SOURCE_DIR}/lex.ll"
    "${CMAKE_CURRENT_BINARY_DIR}/flex_output.cpp"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/flex_output.h")

set(flex_sources
    "${CMAKE_CURRENT_BINARY_DIR}/flex_output.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/flex_output.h")

add_library(ispc-flex OBJECT ${flex_sources})

target_include_directories(ispc-flex PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_options(ispc-flex PRIVATE -std=c++11 -Werror -Wfatal-errors)

add_executable(ispc-lexer-regression-test
    lexer_test.cpp
    lexer_impl.h
    lexer_impl.cpp
    lexer_impl_nonflex.h
    lexer_impl_nonflex.cpp
    lexer_impl_flex.h
    lexer_impl_flex.cpp
    $<TARGET_OBJECTS:ispc-flex>)

set_target_properties(ispc-lexer-regression-test
    PROPERTIES
      OUTPUT_NAME run_lexer_regression_test
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

target_include_directories(ispc-lexer-regression-test
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(ispc-lexer-regression-test PRIVATE ispc_lang)

target_compile_options(ispc-lexer-regression-test PRIVATE ${cxxflags})

set(integers_test_options --test-all-lane-widths)

file(GLOB lexer_tests "lexer_tests/*.ispc")

foreach(lexer_test_path ${lexer_tests})

  get_filename_component(lexer_test "${lexer_test_path}" NAME_WE)

  add_test(NAME ispc_lexer_regression_test:${lexer_test}
    COMMAND $<TARGET_FILE:ispc-lexer-regression-test> ${${lexer_test}_test_options} ${lexer_test_path})

endforeach(lexer_test_path ${lexer_tests})

enable_testing()
