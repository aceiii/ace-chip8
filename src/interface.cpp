#include "interface.h"

#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

constexpr int kMaxSamples = 512;
constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;

AudioStream stream;

bool play_sound = false;
float sine_idx = 0.0f;

float square(float val) {
    if (val > 0) {
        return 1.0f;
    } else if (val < 0) {
        return -1.0f;
    }
    return 0;
}

Interface::Interface(Interpreter* interpreter) : interpreter(interpreter) {}

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
            if (!play_sound) {
                d[i] = 0;
                continue;
            }

            d[i] = (short)(32000.0f*square(sinf(2*PI*sine_idx)));
            sine_idx += incr;
            if (sine_idx > 1.0f) sine_idx -= 1.0f;
        }
    });
    PlayAudioStream(stream);

    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    rlImGuiSetup(true);
}

bool Interface::update() {
    registers* regs = interpreter->get_registers();
    play_sound = regs->sound > 0;

    // mouseDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    rlImGuiBegin();
    bool open = true;
    ImGui::ShowDemoWindow(&open);

    ImGui::Begin("CHIP-8");
    if (ImGui::Button("Play Sound")) {
        regs->sound = 255;
    }
    ImGui::End();

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
