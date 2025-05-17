cmake_minimum_required(VERSION 3.25)
project(
  backport
  VERSION 1.0.3
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  target_install_package
  GIT_REPOSITORY https://github.com/jkammerland/target_install_package.cmake.git
  GIT_TAG v1.0.6)
FetchContent_MakeAvailable(target_install_package)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/get_cpm.cmake)

option(backport_USE_SYSTEM_EXPECTED "Use system expected" OFF)
if(NOT backport_USE_SYSTEM_EXPECTED)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/get_cpm.cmake)
  set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_cache)
  cpmaddpackage(
    NAME
    expected
    GITHUB_REPOSITORY
    TartanLlama/expected
    VERSION
    1.1.0
    GIT_TAG
    v1.1.0
    OPTIONS
    "EXPECTED_BUILD_TESTS OFF"
    "EXPECTED_BUILD_PACKAGE OFF")
else()
  find_package(tl-expected REQUIRED)
endif()

add_library(${PROJECT_NAME} INTERFACE tests/test_expected.cpp)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                                                     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_sources(
  ${PROJECT_NAME}
  INTERFACE FILE_SET
            HEADERS
            BASE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            FILES
            "include/backport/expected.h"
            "include/backport/move_only_function.h")

target_link_libraries(${PROJECT_NAME} INTERFACE tl::expected)

option(backport_INSTALL "Install the library" OFF)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR} OR backport_INSTALL)
  target_install_package(${PROJECT_NAME})
endif()

option(backport_BUILD_TESTS "Build tests (default)" OFF)
option(BACKPORT_FORCE_DISABLE_BUILD_TESTS "Absolutely disable building tests" OFF)
if(NOT BACKPORT_FORCE_DISABLE_BUILD_TESTS AND backport_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_subdirectory(tests)
endif()
