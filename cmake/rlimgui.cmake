
include(cmake/imgui.cmake)

set(rlimgui_SOURCES
    ${CMAKE_SOURCE_DIR}/external/rlImGui/rlImGui.cpp)

add_library(rlimgui STATIC ${rlimgui_SOURCES})

target_link_libraries(rlimgui PUBLIC raylib imgui)
