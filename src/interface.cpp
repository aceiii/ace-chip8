#include "interface.h"

#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nfd.h>

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

Interface::Interface(std::shared_ptr<registers> regs) : regs(regs) {}

void Interface::initialize() {
    NFD_Init();

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
    play_sound = regs->st > 0;

    // mouseDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    rlImGuiBegin();
    bool open = true;
    ImGui::ShowDemoWindow(&open);

    ImGui::Begin("CHIP-8");
    {
        ImGui::Text("ST: %d", regs->st);

        static float volume = GetMasterVolume() * 100.0f;
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f")) {
            spdlog::debug("Set volume: {}", volume);
            SetMasterVolume(volume / 100.0f);
        }

        if (ImGui::Button("Play Sound")) {
            regs->st = 255;
        }

        ImGui::SameLine();
        if (ImGui::Button("Stop Sound")) {
            regs->st = 0;
        }

        if (ImGui::Button("Open file")) {
            nfdchar_t *out_path;
            nfdfilteritem_t filter_item[2] = { { "Source code", "c,cpp,cc" }, { "Headers", "h,hpp" } };
            nfdresult_t result = NFD_OpenDialog(&out_path, filter_item, 2, NULL);
            if (result == NFD_OKAY) {
                spdlog::debug("Opened file: {}", out_path);
                NFD_FreePath(out_path);
            } else if (result == NFD_CANCEL) {
                spdlog::debug("User pressed cancel.");
            } else {
                spdlog::debug("Error: {}\n", NFD_GetError());
            }
        }
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
    NFD_Quit();
}
