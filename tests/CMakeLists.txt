cmake_minimum_required(VERSION 3.20)
project(tests VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

include(${CMAKE_CURRENT_SOURCE_DIR}/get_cpm.cmake)
set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_cache)

cpmaddpackage("gh:fmtlib/fmt#11.1.4")
cpmaddpackage("gh:jkammerland/doctest@1.0.1")

# add executables more if needed
add_executable(test_move_only_function test_move_only_function.cpp)
target_link_libraries(test_move_only_function PRIVATE backport doctest::doctest fmt::fmt)
target_compile_definitions(test_move_only_function PRIVATE MOVE_ONLY_FUNCTION_CUSTOM_IMPL)
target_compile_features(test_move_only_function PRIVATE cxx_std_23)

add_executable(test_expected test_expected.cpp)
target_link_libraries(test_expected PRIVATE backport doctest::doctest fmt::fmt)
target_compile_definitions(test_expected PRIVATE EXPECTED_CUSTOM_IMPL)
target_compile_features(test_expected PRIVATE cxx_std_23)

# ~~~
# add tests for ctest
# add_test(NAME test_move_only_function COMMAND test_move_only_function)
# add_test(NAME test_expected COMMAND test_expected)
# ~~~
