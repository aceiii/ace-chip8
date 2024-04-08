cmake_minimum_required(VERSION 3.15)

project(ace-chip8 VERSION 0.0.1 LANGUAGES CXX)

set(EXE_NAME ace-chip8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(SOURCE_FILES
    src/main.cpp
    src/interpreter.cpp
    src/interface.cpp
    src/timer.cpp
    src/random.cpp
    src/applog.cpp
    src/screen.cpp
    src/assembly.cpp
    src/keyboard.cpp
    src/keyboard.h
)

set(EXPECTED_BUILD_TESTS OFF)
set(ARGPARSE_BUILD_TESTS OFF)

include(cmake/raylib.cmake)
include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)

add_subdirectory(external/spdlog)
add_subdirectory(external/argparse)
add_subdirectory(external/expected)
add_subdirectory(external/magic_enum)
add_subdirectory(external/nativefiledialog-extended)
add_subdirectory(external/tomlplusplus)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} spdlog)
target_link_libraries(${EXE_NAME} argparse)
target_link_libraries(${EXE_NAME} expected)
target_link_libraries(${EXE_NAME} magic_enum)
# target_link_libraries(${EXE_NAME} raylib)
target_link_libraries(${EXE_NAME} rlimgui)
target_link_libraries(${EXE_NAME} nfd)
target_link_libraries(${EXE_NAME} tomlplusplus::tomlplusplus)

target_include_directories(${EXE_NAME} PUBLIC external/rlimgui)
