cmake_minimum_required(VERSION 3.20)
project(tests VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

include(${CMAKE_CURRENT_SOURCE_DIR}/get_cpm.cmake)
set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_cache)

cpmaddpackage("gh:fmtlib/fmt#11.1.4")
cpmaddpackage("gh:jkammerland/doctest@1.0.1")

# add executables more if needed
add_executable(tests test_move_only_function.cpp)
target_link_libraries(tests PRIVATE backport doctest::doctest fmt::fmt)
target_compile_definitions(tests PRIVATE MOVE_ONLY_FUNCTION_CUSTOM_IMPL)

# add tests for ctest
add_test(NAME tests COMMAND tests)
