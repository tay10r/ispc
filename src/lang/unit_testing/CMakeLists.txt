cmake_minimum_required(VERSION 3.5.1)

# This is where Googletest installs to on Ubuntu 16.04.
if(IS_DIRECTORY "/usr/src/gtest")
  add_subdirectory("/usr/src/gtest" "gtest")
endif(IS_DIRECTORY "/usr/src/gtest")

if(TARGET gtest)
  set(gtest_libs gtest gtest_main)
else(TARGET gtest)
  find_package(GTest REQUIRED)
  set(gtest_libs GTest::GTest GTest::Main)
endif(TARGET gtest)

add_executable(${PROJECT_NAME}_unit_tests
  lexer_tests.cpp)

set_target_properties(${PROJECT_NAME}_unit_tests
  PROPERTIES
    OUTPUT_NAME run_unit_tests
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME}_unit_tests PRIVATE ${PROJECT_NAME} ${gtest_libs})

add_test(NAME ${PROJECT_NAME}_unit_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}_unit_tests>)

enable_testing()
