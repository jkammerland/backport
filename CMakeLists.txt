cmake_minimum_required(VERSION 3.25)
project(
  backport
  VERSION 1.0.5
  LANGUAGES CXX)

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
    1.2.0
    GIT_TAG
    v1.2.0
    OPTIONS
    "EXPECTED_BUILD_TESTS OFF"
    "EXPECTED_BUILD_PACKAGE OFF")
else()
  find_package(tl-expected REQUIRED)
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_sources(
  ${PROJECT_NAME}
  INTERFACE FILE_SET
            HEADERS
            BASE_DIRS
            "include"
            FILES
            "include/backport/expected.hpp"
            "include/backport/move_only_function.hpp")

target_link_libraries(${PROJECT_NAME} INTERFACE tl::expected)

option(backport_INSTALL "Install the library" OFF)
if(backport_INSTALL)
  include(FetchContent)
  FetchContent_Declare(
    target_install_package
    GIT_REPOSITORY https://github.com/jkammerland/target_install_package.cmake.git
    GIT_TAG v5.3.1)
  FetchContent_MakeAvailable(target_install_package)

  target_install_package(${PROJECT_NAME})
endif()

option(backport_BUILD_TESTS "Build tests (default)" OFF)
option(BACKPORT_FORCE_DISABLE_BUILD_TESTS "Absolutely disable building tests" OFF)
if(NOT BACKPORT_FORCE_DISABLE_BUILD_TESTS AND backport_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
