#include "interface.h"
#include "random.h"
#include "raylib.h"
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
#include <mach_debug/zone_info.h>
#include <memory>
#include <mutex>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>
#include <toml++/impl/table.hpp>

namespace fs = std::filesystem;

constexpr int kDefaultFPS = 60;
constexpr int kMaxSamples = 512;
constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;
constexpr int kDefaultScreenPixelSize = 4;
constexpr int kDefaultWindowWidth = 800;
constexpr int kDefaultWindowHeight = 600;

const char* const kWindowTitle = "CHIP-8";
const char* const kSettingsFile = "settings.toml";

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

static void deserialize_settings(const toml::table &table, interface_settings &settings) {
  settings.window_width = table["window"]["width"].value_or(settings.window_width);
  settings.window_height = table["window"]["height"].value_or(settings.window_height);
  settings.lock_fps = table["editor"]["lock_fps"].value_or(settings.lock_fps);
  settings.show_fps = table["editor"]["show_fps"].value_or(settings.show_fps);
  settings.show_demo = table["view"]["demo"].value_or(settings.show_demo);
  settings.show_screen = table["view"]["screen"].value_or(settings.show_screen);
  settings.show_memory = table["view"]["memory"].value_or(settings.show_memory);
  settings.show_registers = table["view"]["registers"].value_or(settings.show_registers);
  settings.show_logs = table["view"]["logs"].value_or(settings.show_logs);
  settings.show_emulation = table["view"]["emulation"].value_or(settings.show_emulation);
  settings.show_misc = table["view"]["misc"].value_or(settings.show_misc);
  settings.show_instructions = table["view"]["instructions"].value_or(settings.show_instructions);
  settings.show_keyboard = table["view"]["keyboard"].value_or(settings.show_keyboard);
}

static void serialize_settings(const interface_settings &settings, toml::table &table) {
  auto sub_table = [] (toml::table &table, const char* key) -> toml::table& {
    toml::table *sub;
    if (table.get(key)) {
      sub = table.get(key)->as_table();
    } else {
      table.insert(key, toml::table {});
      sub = table.get(key)->as_table();
    }
    return *sub;
  };

  toml::table& window = sub_table(table, "window");
  window.insert_or_assign("width", settings.window_width);
  window.insert_or_assign("height", settings.window_height);

  toml::table& editor = sub_table(table, "editor");
  editor.insert_or_assign("lock_fps", settings.lock_fps);
  editor.insert_or_assign("show_fps", settings.show_fps);

  toml::table& view = sub_table(table, "view");
  view.insert_or_assign("demo", settings.show_demo);
  view.insert_or_assign("screen", settings.show_screen);
  view.insert_or_assign("memory", settings.show_memory);
  view.insert_or_assign("registers", settings.show_registers);
  view.insert_or_assign("logs", settings.show_logs);
  view.insert_or_assign("emulation", settings.show_emulation);
  view.insert_or_assign("misc", settings.show_misc);
  view.insert_or_assign("instructions", settings.show_instructions);
  view.insert_or_assign("keyboard", settings.show_keyboard);
}

Interface::Interface(std::shared_ptr<registers> regs, Interpreter *interpreter)
    : interpreter{interpreter}, regs(std::move(regs)) {}

void Interface::initialize() {
  if (auto result = config.load(kSettingsFile); !result.has_value()) {
    spdlog::trace("Settings.toml does not exist, using defaults: {}", result.error());
    config.settings.reset();
    config.settings.window_width = kDefaultWindowWidth;
    config.settings.window_height = kDefaultWindowHeight;
    init_dock = true;
  } else {
    config.deserialize(deserialize_settings);
    init_dock = false;
  }

  interface_settings &settings = config.settings;

  NFD_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(settings.window_width, settings.window_height, kWindowTitle);
  InitAudioDevice();

  int monitor = GetCurrentMonitor();
  spdlog::trace("Current monitor: {}", monitor);

  int monitor_width = GetMonitorWidth(monitor);
  int monitor_height = GetMonitorHeight(monitor);
  spdlog::trace("Monitor resolution: {}x{}", monitor_width, monitor_height);

  stream = LoadAudioStream(44100, 16, 1);
  SetAudioStreamBufferSizeDefault(kMaxSamplesPerUpdate);

  SetAudioStreamCallback(stream, [](void *buffer, unsigned int frames) {
    const float frequency = 440.0f;

    float incr = frequency / float(kAudioSampleRate);
    auto d = static_cast<short *>(buffer);

    for (unsigned int i = 0; i < frames; i++) {
      if (!play_sound) {
        d[i] = 0;
        continue;
      }

      d[i] =
          static_cast<short>(((1 << 15) - 1) * square(sinf(2 * PI * sine_idx)));
      sine_idx += incr;
      if (sine_idx > 1.0f)
        sine_idx -= 1.0f;
    }
  });
  PlayAudioStream(stream);

  if (settings.lock_fps) {
    SetTargetFPS(kDefaultFPS);
  }

  SetExitKey(KEY_NULL);
  rlImGuiSetup(true);

  while (!IsWindowReady()) {
  }

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  mem_editor.Cols = 8;

  auto level = spdlog::get_level();

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto formatter = std::make_shared<spdlog::pattern_formatter>();
  auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
      [=](const spdlog::details::log_msg &msg) {
        spdlog::memory_buf_t formatted;
        formatter->format(msg, formatted);

        std::string formatted_string(formatted.begin(), formatted.size());
        app_log.add_log(formatted_string);
      });

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(console_sink);
  sinks.push_back(callback_sink);

  auto logger =
      std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
  logger->set_level(level);

  spdlog::set_default_logger(logger);
  spdlog::info("Initialized interface");

  screen.initialize(kScreenWidth, kScreenHeight, kDefaultScreenPixelSize,
                    regs->screen.data());
  assembly.initialize(regs.get());

  keyboard.initialize(regs);
}

bool Interface::update() {
  interface_settings &settings = config.settings;

  if (IsFileDropped()) {
    FilePathList droppedFiles = LoadDroppedFiles();
    if (droppedFiles.count > 0) {
      spdlog::info("Dropped file: {}", droppedFiles.paths[0]);

      fs::path filepath(droppedFiles.paths[0]);

      if (fs::is_symlink(filepath)) {
        filepath.replace_filename(fs::read_symlink(filepath));
        filepath = fs::absolute(filepath);
      }

      const std::string ext = filepath.extension().string();

      if (fs::is_regular_file(filepath) && (ext == ".ch8" || ext == ".c8")) {
        load_rom(filepath.string());
      } else {
        spdlog::warn("Invalid file type: {}", filepath.string());
      }
    }

    UnloadDroppedFiles(droppedFiles);
  }

  keyboard.update();

  play_sound = regs->st > 0;

  screen.update();

  BeginDrawing();
  ClearBackground(RAYWHITE);

  rlImGuiBegin();

  ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  if (init_dock) {
    init_dock = false;

    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
    ImGui::DockBuilderAddNode(dockspace_id,
                              ImGuiDockNodeFlags_DockSpace); // Add empty node
    ImGui::DockBuilderSetNodeSize(dockspace_id,
                                  {static_cast<float>(GetScreenWidth()),
                                   static_cast<float>(GetScreenHeight())});

    ImGuiID dockspace_main_id = dockspace_id;
    ImGuiID bottom = ImGui::DockBuilderSplitNode(
        dockspace_main_id, ImGuiDir_Down, 0.2f, nullptr, &dockspace_main_id);

    ImGuiID right = ImGui::DockBuilderSplitNode(
        dockspace_main_id, ImGuiDir_Right, 0.5f, nullptr, &dockspace_main_id);

    ImGuiID center_right = ImGui::DockBuilderSplitNode(right, ImGuiDir_Left,
                                                       0.60, nullptr, &right);

    ImGuiID center_right_bottom = ImGui::DockBuilderSplitNode(
        center_right, ImGuiDir_Down, 0.5f, nullptr, &center_right);

    ImGuiID main_bottom = ImGui::DockBuilderSplitNode(
        dockspace_main_id, ImGuiDir_Down, 0.1f, nullptr, &dockspace_main_id);

    ImGui::DockBuilderDockWindow("Instructions", right);
    ImGui::DockBuilderDockWindow("Memory", center_right);
    ImGui::DockBuilderDockWindow("Registers", center_right_bottom);
    ImGui::DockBuilderDockWindow("Screen", dockspace_main_id);
    ImGui::DockBuilderDockWindow("Logs", bottom);
    ImGui::DockBuilderDockWindow("Emulation", main_bottom);
    ImGui::DockBuilderFinish(dockspace_id);
  }

  render_main_menu();

  if (settings.show_demo) {
    ImGui::ShowDemoWindow(&settings.show_demo);
  }

  if (settings.show_registers) {
    if (ImGui::Begin("Registers", &settings.show_registers)) {
      ImGui::BeginTable("general_registers", 4, ImGuiTableFlags_Borders);
      {

        for (int y = 0; y < 4; y += 1) {
          ImGui::TableNextRow();
          for (int x = 0; x < 4; x += 1) {
            int idx = (y * 4) + x;

            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
            ImGui::Text("v%X", idx);
            ImGui::PopStyleColor();

            ImGui::Text("0x%02x", regs->v[idx]);
            ImGui::Text("%04d", regs->v[idx]);
          }
        }
      }
      ImGui::EndTable();

      ImGui::BeginTable("specific_registers", 4, ImGuiTableFlags_Borders);
      {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          ImGui::Text("PC");
          ImGui::PopStyleColor();

          ImGui::Text("0x%02x", regs->pc);
          ImGui::Text("%04d", regs->pc);
        }

        ImGui::TableNextColumn();
        {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          ImGui::Text("I");
          ImGui::PopStyleColor();

          ImGui::Text("0x%02x", regs->i);
          ImGui::Text("%04d", regs->i);
        }

        ImGui::TableNextColumn();
        {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          ImGui::Text("DT");
          ImGui::PopStyleColor();

          ImGui::Text("0x%02x", regs->dt);
          ImGui::Text("%04d", regs->dt);
        }

        ImGui::TableNextColumn();
        {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          ImGui::Text("ST");
          ImGui::PopStyleColor();

          ImGui::Text("0x%02x", regs->st);
          ImGui::Text("%04d", regs->st);
        }
      }
      ImGui::EndTable();
    }
    ImGui::End();
  }

  if (settings.show_misc) {
    if (ImGui::Begin("Miscellaneous", &settings.show_misc)) {
      ImGui::Text("ST: %04x (%05d)", regs->st, regs->st);
      ImGui::Text("DT: %04x (%05d)", regs->dt, regs->dt);
      ImGui::Text("I:  %04x (%05d)", regs->i, regs->i);
      ImGui::Text("PC: %04x (%05d)", regs->pc, regs->pc);

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
    }
    ImGui::End();
  }

  if (settings.show_emulation) {
    if (ImGui::Begin("Emulation", &settings.show_emulation)) {
      ImGuiStyle &style = ImGui::GetStyle();

      auto push_disabled_btn_flags = []() {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
      };

      auto pop_disabled_btn_flags = []() {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor();
      };

      ImVec2 size = ImGui::GetWindowSize();
      ImVec2 frame_padding{16.0f, 8.0f};
      float num_buttons = 4.0f;
      float button_width = ImGui::CalcTextSize(ICON_FA_PLAY).x;
      float buttons_width =
          (button_width + (2 * frame_padding.x) + style.ItemSpacing.x) *
          num_buttons;
      float slider_width = 192.0f;

      ImGui::SameLine((size.x - buttons_width - slider_width) / 2);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, frame_padding);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

      bool is_playing = interpreter->is_playing();

      if (!rom_loaded || is_playing) {
        push_disabled_btn_flags();
      }

      if (ImGui::Button(ICON_FA_PLAY)) {
        interpreter->play();
      }

      if (!rom_loaded || is_playing) {
        pop_disabled_btn_flags();
      }

      ImGui::SameLine();

      if (!rom_loaded || !is_playing) {
        push_disabled_btn_flags();
      }

      if (ImGui::Button(ICON_FA_PAUSE)) {
        interpreter->stop();
      }

      if (!rom_loaded || !is_playing) {
        pop_disabled_btn_flags();
      }

      ImGui::SameLine();

      if (!rom_loaded) {
        push_disabled_btn_flags();
      }

      ImGui::PushButtonRepeat(true);
      if (ImGui::Button(ICON_FA_FORWARD_STEP)) {
        spdlog::debug("Single step");
        interpreter->step();
      }
      ImGui::PopButtonRepeat();

      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_STOP)) {
        interpreter->stop();
        interpreter->reset();
        interpreter->load_rom_bytes(rom);
      }

      if (!rom_loaded) {
        pop_disabled_btn_flags();
      }

      ImGui::PopStyleVar(2);

      ImGui::SameLine();

      ImGui::SetNextItemWidth(slider_width);
      ImGui::SetCursorPosY(32);
      ImGui::SliderInt("IPS", &interpreter->update_play_rate, 10, 3200);
    }
    ImGui::End();
  }

  if (settings.show_screen) {
    if (ImGui::Begin("Screen", &settings.show_screen)) {
      screen.draw();
    }
    ImGui::End();
  }

  if (settings.show_memory) {
    if (ImGui::Begin("Memory", &settings.show_memory)) {
      mem_editor.DrawContents(regs->mem.data(), regs->mem.size());
    }
    ImGui::End();
  }

  if (settings.show_instructions) {
    if (ImGui::Begin("Instructions", &settings.show_instructions)) {
      assembly.draw();
    }
    ImGui::End();
  }

  if (settings.show_logs) {
    if (ImGui::Begin("Logs", &settings.show_logs)) {
      app_log.draw();
    }
    ImGui::End();
  }

  if (settings.show_keyboard) {
    if (ImGui::Begin("Keyboard", &settings.show_keyboard)) {
      keyboard.draw();
    }
    ImGui::End();
  }

  rlImGuiEnd();

  if (settings.show_fps) {
    DrawFPS(10, GetScreenHeight() - 24);
  }

  EndDrawing();

  return WindowShouldClose() || should_close;
}

void Interface::render_main_menu() {
  interface_settings &settings = config.settings;

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
      bool is_playing = interpreter->is_playing();
      if (ImGui::MenuItem("Play", nullptr, false, !is_playing)) {
        interpreter->play();
      }
      if (ImGui::MenuItem("Pause", nullptr, false, is_playing)) {
        interpreter->stop();
      }
      if (ImGui::MenuItem("Step", nullptr, false, !is_playing)) {
        interpreter->step();
      }
      if (ImGui::MenuItem("Reset", nullptr, false, !is_playing)) {
        interpreter->reset();
        interpreter->load_rom_bytes(rom);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Emulation", nullptr, &settings.show_emulation);
      ImGui::MenuItem("Keyboard", nullptr, &settings.show_keyboard);
      ImGui::MenuItem("Screen", nullptr, &settings.show_screen);
      ImGui::MenuItem("Memory", nullptr, &settings.show_memory);
      ImGui::MenuItem("Registers", nullptr, &settings.show_registers);
      ImGui::MenuItem("Miscellaneous", nullptr, &settings.show_misc);
      ImGui::Separator();
      if (ImGui::MenuItem("Lock FPS", nullptr, &settings.lock_fps)) {
        if (settings.lock_fps) {
          spdlog::debug("Locking FPS");
          SetTargetFPS(kDefaultFPS);
        } else {
          spdlog::debug("Unlocking FPS");
          SetTargetFPS(0);
        }
      }
      ImGui::MenuItem("Show FPS", nullptr, &settings.show_fps);
      ImGui::MenuItem("ImGui Demo", nullptr, &settings.show_demo);
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
  config.settings.reset();
  init_dock = true;
}

void Interface::cleanup() {
  interface_settings &settings = config.settings;
  settings.window_width = GetScreenWidth();
  settings.window_height = GetScreenHeight();

  spdlog::info("Cleaning up interface");
  rlImGuiShutdown();
  UnloadAudioStream(stream);
  CloseAudioDevice();
  CloseWindow();
  NFD_Quit();

  spdlog::info("Saving settings to file");

  if (auto result = config.serialize(serialize_settings); !result.has_value()) {
    spdlog::warn("Failed to serialize settings: {}", result.error());
  }

  if (auto result = config.save(kSettingsFile); !result.has_value()) {
    spdlog::warn("Failed to write settings.toml: {}", result.error());
  }
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
  spdlog::debug("Loading rom: {}", filename);

  rom_loaded = true;
  fs::path rom_path = filename;
  set_window_title(rom_path.filename());

  std::ifstream in(filename, std::ios::binary | std::ifstream::ate);
  size_t pos = in.tellg();
  if (pos > (kMemSize - kRomStartIndex)) {
    spdlog::error("File exceeds memory size, NOT loading rom");
    return;
  }

  in.seekg(0, std::ifstream::beg);
  rom = {std::istreambuf_iterator<char>(in), {}};

  interpreter->stop();
  interpreter->reset();
  interpreter->load_rom_bytes(rom);

  if (auto_play) {
    interpreter->play();
  }
}

void Interface::set_window_title(const std::string &title) {
  if (title.empty()) {
    SetWindowTitle(kWindowTitle);
  } else {
    SetWindowTitle(fmt::format("{} - {}", kWindowTitle, title).c_str());
  }
}
