#include "interface.h"

#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

constexpr int kMaxSamples = 512;
constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;

AudioStream stream;
bool mouseDown = false;
float sineIdx = 0.0f;

float square(float val) {
    if (val > 0) {
        return 1.0f;
    } else if (val < 0) {
        return -1.0f;
    }
    return 0;
}

void Interface::initialize() {
    int width = 1200;
    int height = 800;

    InitWindow(width, height, "CHIP-8");
    InitAudioDevice();

    stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamBufferSizeDefault(kMaxSamplesPerUpdate);
    SetAudioStreamCallback(stream, [] (void* buffer, unsigned int frames) {
        const float frequency = 440.0f;

        float incr = frequency / float(kAudioSampleRate);
        short *d = (short *)buffer;

        for (unsigned int i = 0; i < frames; i++)
        {
            if (!mouseDown) {
                d[i] = 0;
                continue;
            }

            d[i] = (short)(32000.0f*square(sinf(2*PI*sineIdx)));
            sineIdx += incr;
            if (sineIdx > 1.0f) sineIdx -= 1.0f;
        }
    });
    PlayAudioStream(stream);

    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    rlImGuiSetup(true);
}

bool Interface::update() {
    mouseDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    rlImGuiBegin();
    bool open = true;
    ImGui::ShowDemoWindow(&open);
    rlImGuiEnd();

    DrawFPS(10, 10);
    EndDrawing();

    return WindowShouldClose();
}

void Interface::cleanup() {
    rlImGuiShutdown();
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
