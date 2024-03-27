#include <spdlog/spdlog.h>
#include <raylib.h>

auto main() -> int {
    spdlog::info("Hello, world!!!");

    int width = 800;
    int height = 600;

    InitWindow(width, height, "CHIP-8");
    SetExitKey(KEY_ESCAPE);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
