cmake_minimum_required(VERSION 3.20)
project(tests VERSION 1.0.0)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/get_cpm.cmake)
set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_cache)

cpmaddpackage("gh:fmtlib/fmt#11.1.4")
cpmaddpackage("gh:jkammerland/doctest@1.0.1")

# add executables more if needed
add_executable(test_move_only_function test_move_only_function.cpp)
target_link_libraries(test_move_only_function PRIVATE backport doctest::doctest fmt::fmt)
target_compile_definitions(test_move_only_function PRIVATE MOVE_ONLY_FUNCTION_CUSTOM_IMPL)
target_compile_features(test_move_only_function PRIVATE cxx_std_23)
add_test(NAME test_move_only_function COMMAND test_move_only_function)

add_executable(test_expected test_expected.cpp)
target_link_libraries(test_expected PRIVATE backport doctest::doctest fmt::fmt)
target_compile_definitions(test_expected PRIVATE EXPECTED_CUSTOM_IMPL)
target_compile_features(test_expected PRIVATE cxx_std_23)
add_test(NAME test_expected COMMAND test_expected)

add_executable(test_cpp11_compat cpp11.cpp)
target_link_libraries(test_cpp11_compat PRIVATE backport)
target_compile_features(test_cpp11_compat PRIVATE cxx_std_11)
set_property(TARGET test_cpp11_compat PROPERTY CXX_STANDARD 11)
set_property(TARGET test_cpp11_compat PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET test_cpp11_compat PROPERTY CXX_EXTENSIONS OFF)
add_test(NAME test_cpp11_compat COMMAND test_cpp11_compat)

add_executable(test_cpp20_compat cpp20.cpp)
target_link_libraries(test_cpp20_compat PRIVATE backport)
target_compile_features(test_cpp20_compat PRIVATE cxx_std_20)
set_property(TARGET test_cpp20_compat PROPERTY CXX_STANDARD 20)
set_property(TARGET test_cpp20_compat PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET test_cpp20_compat PROPERTY CXX_EXTENSIONS OFF)
add_test(NAME test_cpp20_compat COMMAND test_cpp20_compat)
