cmake_minimum_required(VERSION 3.10)

project(gt_filesystem CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_11)

add_library(project_warnings INTERFACE)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

#include(cmake/Sanitizers.cmake)
#enable_sanitizers(project_options)

add_library(gtfs src/gtfs.cpp)
target_include_directories(gtfs PUBLIC src)
target_link_libraries(gtfs PRIVATE project_options project_warnings)

set(TEST_FS_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_dir" CACHE STRING "directory for FS tests")
configure_file("tests/constants.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/constants.hpp")

add_executable(tests tests/test.cpp)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tests PRIVATE project_options project_warnings gtfs)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_dir)

if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
    add_custom_target(run_tests
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND tests 0
            )

    add_custom_target(run_tests_verbose
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND tests 1
            )
else()
    add_custom_target(run_tests
            COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND tests 0
            )

    add_custom_target(run_tests_verbose
            COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/test_dir/*
            COMMAND tests 1
            )
endif()