#include "interface.h"
#include "random.h"
#include "spdlog/common.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/logger.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/callback_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <mutex>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>

constexpr int kMaxSamples = 512;
constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;
constexpr int kDefaultScreenPixelSize = 4;

AudioStream stream;

bool play_sound = false;
float sine_idx = 0.0f;

bool rom_loaded = false;
bool auto_play = true;

float square(float val) {
  if (val > 0) {
    return 1.0f;
  } else if (val < 0) {
    return -1.0f;
  }
  return 0;
}

Interface::Interface(std::shared_ptr<registers> regs, Interpreter *interpreter)
    : interpreter{interpreter}, regs(regs) {}

void Interface::initialize() {
  NFD_Init();

  int width = 1200;
  int height = 800;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(width, height, "CHIP-8");
  InitAudioDevice();

  stream = LoadAudioStream(44100, 16, 1);
  SetAudioStreamBufferSizeDefault(kMaxSamplesPerUpdate);

  SetAudioStreamCallback(stream, [](void *buffer, unsigned int frames) {
    const float frequency = 440.0f;

    float incr = frequency / float(kAudioSampleRate);
    short *d = static_cast<short*>(buffer);

    for (unsigned int i = 0; i < frames; i++) {
      if (!play_sound) {
        d[i] = 0;
        continue;
      }

      d[i] = static_cast<short>(((1<<15)-1) * square(sinf(2 * PI * sine_idx)));
      sine_idx += incr;
      if (sine_idx > 1.0f)
        sine_idx -= 1.0f;
    }
  });
  PlayAudioStream(stream);

  SetExitKey(KEY_NULL);
  rlImGuiSetup(true);

  while (!IsWindowReady()) {
  }

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  screen_texture = LoadRenderTexture(kScreenWidth * kDefaultScreenPixelSize,
                                     kScreenHeight * kDefaultScreenPixelSize);

  mem_editor.Cols = 8;

  auto level = spdlog::get_level();

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto formatter = std::make_shared<spdlog::pattern_formatter>();
  auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>([=](const spdlog::details::log_msg &msg) {
    spdlog::memory_buf_t formatted;
    formatter->format(msg, formatted);

    std::string formatted_string(formatted.begin(), formatted.size());
    app_log.add_log(formatted_string);
  });

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(console_sink);
  sinks.push_back(callback_sink);

  auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
  logger->set_level(level);

  spdlog::set_default_logger(logger);
  spdlog::info("Initialized interface");
}

bool Interface::update() {

  if (IsFileDropped()) {
    FilePathList droppedFiles = LoadDroppedFiles();
    if (droppedFiles.count > 0) {
      spdlog::info("Dropped file: {}", droppedFiles.paths[0]);
      load_rom(droppedFiles.paths[0]);
    }

    UnloadDroppedFiles(droppedFiles);
  }

  play_sound = regs->st > 0;

  BeginTextureMode(screen_texture);
  for (int y = 0; y < kScreenHeight; y += 1) {
    for (int x = 0; x < kScreenWidth; x += 1) {
      int idx = (y * kScreenWidth) + x;
      bool px = regs->screen[idx];
      DrawRectangle(x * kDefaultScreenPixelSize, y * kDefaultScreenPixelSize,
                    kDefaultScreenPixelSize, kDefaultScreenPixelSize,
                    px ? RAYWHITE : BLACK);
    }
  }
  EndTextureMode();

  BeginDrawing();
  ClearBackground(RAYWHITE);

  rlImGuiBegin();

  int dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  if (init_dock) {
    init_dock = false;

    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
    ImGui::DockBuilderAddNode(dockspace_id,
                              ImGuiDockNodeFlags_DockSpace); // Add empty node
    ImGui::DockBuilderSetNodeSize(dockspace_id,
                                  {static_cast<float>(GetScreenWidth()),
                                   static_cast<float>(GetScreenHeight())});

    ImGuiID dockspace_main_id = dockspace_id;
    ImGuiID bottom = ImGui::DockBuilderSplitNode(dockspace_main_id, ImGuiDir_Down, 0.2f, nullptr, &dockspace_main_id);

    ImGuiID right = ImGui::DockBuilderSplitNode(
        dockspace_main_id, ImGuiDir_Right, 0.28f, nullptr, &dockspace_main_id);

    ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down,
                                                       0.5f, nullptr, &right);

    ImGui::DockBuilderDockWindow("Memory", right);
    ImGui::DockBuilderDockWindow("Registers", right_bottom);
    ImGui::DockBuilderDockWindow("Screen", dockspace_main_id);
    ImGui::DockBuilderDockWindow("Logs", bottom);
    ImGui::DockBuilderFinish(dockspace_id);
  }

  render_main_menu();

  if (show_demo) {
    ImGui::ShowDemoWindow(&show_demo);
  }

  if (show_registers) {
    if (ImGui::Begin("Registers", &show_registers)) {
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
        open_load_rom_dialog();
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

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{16.0f, 8.0f});
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

      if (!rom_loaded) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
      }

      if (ImGui::Button(ICON_FA_PLAY)) {
        interpreter->play();
      }

      if (!rom_loaded) {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor();
      }

      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_PAUSE)) {
        interpreter->stop();
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_FORWARD_STEP)) {
        interpreter->step();
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_STOP)) {
        interpreter->stop();
        interpreter->reset();
        interpreter->load_rom_bytes(rom);
      }

      ImGui::PopStyleVar();
      ImGui::PopStyleVar();
    }
    ImGui::End();
  }

  if (show_screen) {
    if (ImGui::Begin("Screen", &show_screen)) {
      rlImGuiImageRenderTextureFit(&screen_texture, true);
    }
    ImGui::End();
  }

  if (show_memory) {
    if (ImGui::Begin("Memory", &show_memory)) {
      mem_editor.DrawContents(regs->mem.data(), regs->mem.size());
    }
    ImGui::End();
  }

  if (show_logs) {
    if (ImGui::Begin("Logs", &show_logs)) {
      app_log.draw();
    }
    ImGui::End();
  }

  rlImGuiEnd();

  if (show_fps) {
    DrawFPS(10, GetScreenHeight() - 24);
  }

  EndDrawing();

  return WindowShouldClose() || should_close;
}

void Interface::render_main_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {

      if (ImGui::MenuItem("Load ROM")) {
        open_load_rom_dialog();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
        should_close = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Emulation")) {
      ImGui::MenuItem("Auto Play", nullptr, &auto_play);
      ImGui::Separator();
      if (ImGui::MenuItem("Play", nullptr, false, !interpreter->is_playing())) {
        interpreter->play();
      }
      if (ImGui::MenuItem("Pause", nullptr, false, interpreter->is_playing())) {
        interpreter->stop();
      }
      if (ImGui::MenuItem("Step", nullptr, false, !interpreter->is_playing())) {
        interpreter->step();
      }
      if (ImGui::MenuItem("Reset", nullptr, false,
                          !interpreter->is_playing())) {
        interpreter->reset();
        interpreter->load_rom_bytes(rom);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Screen", nullptr, &show_screen);
      ImGui::MenuItem("Memory", nullptr, &show_memory);
      ImGui::MenuItem("Registers", nullptr, &show_registers);
      ImGui::Separator();
      ImGui::MenuItem("Show FPS", nullptr, &show_fps);
      ImGui::MenuItem("ImGui Demo", nullptr, &show_demo);
      ImGui::Separator();
      if (ImGui::MenuItem("Reset All Windows")) {
        reset_windows();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void Interface::reset_windows() {
  show_demo = false;
  show_fps = false;
  show_memory = true;
  show_registers = true;
  show_screen = true;
  init_dock = true;
}

void Interface::cleanup() {
  spdlog::info("Cleaning up interface");
  rlImGuiShutdown();
  UnloadAudioStream(stream);
  CloseAudioDevice();
  CloseWindow();
  NFD_Quit();
}

void Interface::open_load_rom_dialog() {
  nfdchar_t *rom_path;
  nfdfilteritem_t filter_item[2] = {{"ROM", "rom"}, {"CHIP-8", "ch8"}};
  nfdresult_t result = NFD_OpenDialog(&rom_path, filter_item, 2, NULL);
  if (result == NFD_OKAY) {
    spdlog::debug("Opened file: {}", rom_path);

    load_rom(rom_path);

    NFD_FreePath(rom_path);
  } else if (result == NFD_CANCEL) {
    spdlog::debug("User pressed cancel.");
  } else {
    spdlog::debug("Error: {}\n", NFD_GetError());
  }
}

void Interface::load_rom(const std::string &filename) {
  rom_loaded = true;
  std::filesystem::path rom_path = filename;
  SetWindowTitle(
      fmt::format("CHIP-8 - {}", rom_path.filename().string()).c_str());
  std::ifstream in(filename, std::ios::binary);
  rom = {std::istreambuf_iterator<char>(in), {}};

  interpreter->stop();
  interpreter->reset();
  interpreter->load_rom_bytes(rom);

  if (auto_play) {
    interpreter->play();
  }
}
