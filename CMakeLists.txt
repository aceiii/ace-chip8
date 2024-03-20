cmake_minimum_required(VERSION 3.15)

project(ace-chip8 CXX)

set(EXE_NAME ace-chip8)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(EXPECTED_BUILD_TESTS OFF)
set(ARGPARSE_BUILD_TESTS OFF)

add_subdirectory(external/spdlog)
add_subdirectory(external/argparse)
add_subdirectory(external/expected)
add_subdirectory(external/optional)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} spdlog)
target_link_libraries(${EXE_NAME} argparse)
target_link_libraries(${EXE_NAME} expected)
