#include <spdlog/spdlog.h>
#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

auto main() -> int {
    spdlog::info("Hello, world!!!");

    int width = 800;
    int height = 600;

    InitWindow(width, height, "CHIP-8");
    SetExitKey(KEY_ESCAPE);

    SetTargetFPS(60);

    rlImGuiSetup(true);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        rlImGuiBegin();
        bool open = true;
        ImGui::ShowDemoWindow(&open);
        rlImGuiEnd();

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();

    CloseWindow();

    return 0;
}
