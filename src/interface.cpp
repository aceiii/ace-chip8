#include "interface.h"
#include "random.h"

#include <rlImGui.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nfd.h>
#include <fstream>

constexpr int kMaxSamples = 512;
constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;
constexpr int kDefaultScreenPixelSize = 4;

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

Interface::Interface(std::shared_ptr<registers> regs, Interpreter* interpreter) : interpreter{interpreter}, regs(regs)
{}

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

    screen_texture = LoadRenderTexture(kScreenWidth * kDefaultScreenPixelSize, kScreenHeight * kDefaultScreenPixelSize);
}

bool Interface::update() {
    play_sound = regs->st > 0;

    BeginTextureMode(screen_texture);
    for (int y = 0; y < kScreenHeight; y += 1) {
        for (int x = 0; x < kScreenWidth; x += 1) {
            int idx = (y * kScreenWidth) + x;
            bool px = regs->screen[idx];
            DrawRectangle(x * kDefaultScreenPixelSize, y * kDefaultScreenPixelSize, kDefaultScreenPixelSize, kDefaultScreenPixelSize, px ? RAYWHITE : BLACK);
        }
    }
    EndTextureMode();

    BeginDrawing();
    ClearBackground(RAYWHITE);

    rlImGuiBegin();
    bool open = true;
    ImGui::ShowDemoWindow(&open);

    ImGui::Begin("CHIP-8");
    {
        ImGui::Text("ST: %04x (%05d)", regs->st, regs->st);
        ImGui::Text("DT: %04x (%05d)", regs->dt, regs->dt);
        ImGui::Text("I:  %04x (%05d)", regs->i, regs->i);
        ImGui::Text("PC: %04x (%05d)", regs->pc, regs->pc);
        ImGui::Text("SP: %04x (%05d)", regs->sp, regs->sp);

        static float volume = GetMasterVolume() * 100.0f;
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f")) {
            spdlog::debug("Set volume: {}", volume);
            SetMasterVolume(volume / 100.0f);
        }

        static int sound_val;
        if (ImGui::Button("Play Sound")) {
            regs->st = sound_val;
        }
        ImGui::SameLine();
        ImGui::SliderInt("##Sound Val", &sound_val, 1, 255);

        if (ImGui::Button("Stop Sound")) {
            regs->st = 0;
        }

        if (ImGui::Button("Open file")) {
            nfdchar_t *rom_path;
            nfdfilteritem_t filter_item[2] = { { "ROM", "rom" }, { "CHIP-8", "ch8" } };
            nfdresult_t result = NFD_OpenDialog(&rom_path, filter_item, 2, NULL);
            if (result == NFD_OKAY) {
                spdlog::debug("Opened file: {}", rom_path);

                load_rom(rom_path);
                interpreter->load_rom_bytes(rom);

                NFD_FreePath(rom_path);
            } else if (result == NFD_CANCEL) {
                spdlog::debug("User pressed cancel.");
            } else {
                spdlog::debug("Error: {}\n", NFD_GetError());
            }
        }

        static int random_pixel_count = 1;
        if (ImGui::Button("Toggle Random Pixel")) {
            for (int i = 0; i < random_pixel_count; i += 1) {
                uint8_t x = random_byte() % kScreenWidth;
                uint8_t y = random_byte() % kScreenHeight;
                int idx = (y * kScreenWidth) + x;
                regs->screen[idx] = !regs->screen[idx];
            }
        }
        ImGui::SameLine();
        ImGui::SliderInt("##Pixel Count", &random_pixel_count, 1, 100);

        if (ImGui::Button("Play")) {
            interpreter->play();
        }
    }
    ImGui::End();

    ImGui::Begin("SCREEN");
    {
        rlImGuiImageRenderTextureFit(&screen_texture, true);
    }
    ImGui::End();

    ImGui::Begin("MEMORY");
    {
        for (int i = 0; i < regs->mem.size(); i += 2) {
            uint8_t hi = regs->mem[i];
            uint8_t lo = regs->mem[i + 1];
            ImGui::Text("%04x: %02x %02x", i, hi, lo);
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

void Interface::load_rom(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    rom = { std::istreambuf_iterator<char>(in), {} };
}
