cmake_minimum_required(VERSION 3.5.1)

project(ispc_lang)

option(ISPC_LANG_REGRESSION_TESTING "Whether or not to build the regression tests (requires Flex)." OFF)

option(ISPC_LANG_UNIT_TESTING "Whether or not to build the unit tests (requires Googletest)." OFF)

add_library(${PROJECT_NAME} INTERFACE)

if(MSVC)
  target_compile_options(${PROJECT_NAME} INTERFACE /std:c++14)
else(MSVC)
  target_compile_options(${PROJECT_NAME} INTERFACE -std=c++14)
endif(MSVC)

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(ISPC_LANG_UNIT_TESTING)
  add_subdirectory(unit_testing)
endif(ISPC_LANG_UNIT_TESTING)

if(ISPC_LANG_REGRESSION_TESTING)
  add_subdirectory(regression_testing)
endif(ISPC_LANG_REGRESSION_TESTING)

if(ISPC_LANG_REGRESSION_TESTING OR ISPC_LANG_UNIT_TESTING)
  enable_testing()
endif(ISPC_LANG_REGRESSION_TESTING OR ISPC_LANG_UNIT_TESTING)
