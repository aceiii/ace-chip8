#include "sound.h"

#include <functional>
#include <spdlog/spdlog.h>
#include <raylib.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>
#include <cstring>
#include <array>
#include <cmath>
#include <random>
#include <algorithm>
#include <filesystem>
#include <utility>
#include <vector>

static inline float noise(double x) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> dist(-1.0f, 1.0f);
  return dist(gen);
}

static inline float sine_wave(double x) {
  return std::sin(2 * PI * x);
}

static inline float square_wave(double x) {
  float s = sine_wave(x);
  if (std::abs(s) <= 0.001f) {
    return 0.f;
  }
  return std::abs(s) / s;
}

static inline float triangle_wave(double x) {
  return std::abs(fmod(x, 1.0f) - 0.5f) * 4 - 1;
}

static inline float sawtooth_wave(double x) {
  return std::abs(fmod(x, 1.0f)) * 2 - 1;
}

std::vector<std::pair<const char*, std::function<float(double)>>> wave_funcs = {
  std::make_pair("Sine", sine_wave),
  std::make_pair("Square", square_wave),
  std::make_pair("Triangle", triangle_wave),
  std::make_pair("Sawtooth", sawtooth_wave),
  std::make_pair("Noise", noise),
};

void WaveGeneratorSource::render() {
  if (ImGui::BeginCombo("Type", wave_funcs[wave_type].first)) {
    for (int n = 0; n < wave_funcs.size(); n += 1) {
      bool is_selected = wave_type == n;
      if (ImGui::Selectable(wave_funcs[n].first, is_selected)) {
        wave_type = n;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PlotLines("Sound", samples.data(), samples.size(), 0, nullptr, -1.2f, 1.2f, ImVec2(0, 50.0f));
  ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f");
  ImGui::SliderFloat("Offset", &offset, 0.0f, 1.0f, "%0.2f");
  if (ImGui::Button("+0.0")) {
    offset = 0.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("+0.25")) {
    offset = 0.25f;
  }
  ImGui::SameLine();
  if (ImGui::Button("+0.5")) {
    offset = 0.50f;
  }
  ImGui::SameLine();
  if (ImGui::Button("+0.75")) {
    offset = 0.75f;
  }

  ImGui::SliderFloat("Frequency", &frequency, 20.0f, 15000.0f, "%.0f");
  if (ImGui::Button("100Hz")) {
    frequency = 100.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("220Hz")) {
    frequency = 220.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("440Hz")) {
    frequency = 440.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("1kHz")) {
    frequency = 1000.0f;
  }
  ImGui::SameLine();
    if (ImGui::Button("11.5kHz")) {
    frequency = 11500.0f;
  }
  ImGui::Checkbox("Force Play", &force_play);
}

void WaveGeneratorSource::update(bool play_sound, double time) {
  if (!play_sound && !force_play) {
    memset(&samples, 0, sizeof(float) * samples.size());
    return;
  }

  double incr = 1.0 / kAudioSampleRate;
  auto wave_func = wave_funcs[wave_type].second;

  for (int i = 0; i < samples.size(); i++) {
    samples[i] = wave_func((time * frequency) + offset) * (volume / 100.0f);
    time += incr;
  }
}

WaveFileSource::~WaveFileSource() {
  cleanup();
}

void WaveFileSource::render() {
  auto push_disabled_btn_flags = []() {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
  };

  auto pop_disabled_btn_flags = []() {
    ImGui::PopItemFlag();
    ImGui::PopStyleColor();
  };

  bool is_loaded = !wav_filename.empty();

  if (is_loaded) {
    push_disabled_btn_flags();
  }

  if (ImGui::Button("Load file")) {
    open_load_wave_dialog();
  }

  if (is_loaded) {
    pop_disabled_btn_flags();
  }

  ImGui::SameLine();

  if (!is_loaded) {
    push_disabled_btn_flags();
  }

  if (ImGui::Button("Unload file")) {
    cleanup();
  }

  if (!is_loaded) {
    pop_disabled_btn_flags();
  }

  ImGui::SameLine();
  ImGui::Text("%s", wav_filename.c_str());

  ImGui::PlotHistogram("Waveform", wav_samples, frame_count, 0, nullptr, wav_min, wav_max, ImVec2(0, 80.0f));
  ImGui::PlotLines("Samples", samples.data(), samples.size(), 0, nullptr, -1.0f, 1.0f, ImVec2(0, 50.0f));
  ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f");
  if (ImGui::Button("0%")) {
    volume = 0.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("25%")) {
    volume = 25.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("50%")) {
    volume = 50.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("75%")) {
    volume = 75.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("100%")) {
    volume = 100.0f;
  }

  if (ImGui::RadioButton("1x", volume_mulitplier == 1.0f)) {
    volume_mulitplier = 1.0f;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("2x", volume_mulitplier == 2.0f)) {
    volume_mulitplier = 2.0f;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("4x", volume_mulitplier == 4.0f)) {
    volume_mulitplier = 4.0f;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("8x", volume_mulitplier == 8.0f)) {
    volume_mulitplier = 8.0f;
  }

  ImGui::Checkbox("Force Play", &force_play);
}

void WaveFileSource::update(bool play_sound, double time) {
  if (!wav_samples || (!play_sound && !force_play)) {
    memset(&samples, 0, samples.size() * sizeof(float));
    return;
  }

  for (int i = 0; i < samples.size(); i += 1) {
    samples[i] = wav_samples[current_frame] * (volume / 100.0f) * volume_mulitplier;
    current_frame = (current_frame + 1) % frame_count;
  }
}

void WaveFileSource::cleanup() {
  UnloadWaveSamples(wav_samples);
  wav_samples = nullptr;
  wav_min = -1.5f;
  wav_max = 1.5f;
  wav_filename.clear();
  frame_count = 0;
  force_play = false;
}

void WaveFileSource::open_load_wave_dialog() {
  nfdchar_t *wav_path;
  nfdfilteritem_t filter_item[1] = {{"Wave", "wav"}};
  nfdresult_t result = NFD_OpenDialog(&wav_path, filter_item, 1, NULL);
  if (result == NFD_OKAY) {
    spdlog::debug("Opened file: {}", wav_path);

    cleanup();
    load_wave(wav_path);

    NFD_FreePath(wav_path);
  } else if (result == NFD_CANCEL) {
    spdlog::debug("User pressed cancel.");
  } else {
    spdlog::debug("Error: {}\n", NFD_GetError());
  }
}

void WaveFileSource::load_wave(const char *filename) {
  Wave wave = LoadWave(filename);
  WaveFormat(&wave, kAudioSampleRate, 16, 1);
  wav_samples = LoadWaveSamples(wave);
  frame_count = wave.frameCount;
  UnloadWave(wave);

  std::filesystem::path path(filename);
  wav_filename = path.filename();

  wav_min = -0.01f;
  wav_max = 0.01f;

  for (int i = 0; i < frame_count; i += 1) {
    wav_min = std::min(wav_min, wav_samples[i]);
    wav_max = std::max(wav_max, wav_samples[i]);
  }

  wav_min *= 1.1f;
  wav_max *= 1.1f;
}

void SoundManager::initialize() {
  spdlog::trace("Initializing SoundSource");

  memset(buffer.data(), 0, buffer.size() * sizeof(short));
  memset(samples.data(), 0, samples.size() * sizeof(float));

  SetAudioStreamBufferSizeDefault(kMaxSamplesPerUpdate);
  stream = LoadAudioStream(kAudioSampleRate, kAudioSampleSize, kAudioNumChannels);
  PlayAudioStream(stream);
}

void SoundManager::cleanup() {
  spdlog::trace("Destructing SoundSource");
  sources.clear();
  StopAudioStream(stream);
  UnloadAudioStream(stream);
}

void SoundManager::render() {
  ImGui::PlotLines("Sound", samples.data(), samples.size(), 0, nullptr, -1.5f, 1.5f, ImVec2(0, 80.0f));

  int sid_to_remove = -1;
  for (int sid = 0; sid < sources.size(); sid += 1) {
    auto &pair = sources[sid];
    ImGui::PushID(sid);
    ImGui::BeginChild("##Sound", ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::Text("Sound #%d (%s)", (sid + 1), pair.source->name());

    ImGui::Checkbox("Enable", &pair.enable);

    pair.source->render();

    if (ImGui::Button("Remove")) {
      sid_to_remove = sid;
    }

    ImGui::EndChild();
    ImGui::PopID();
  }

  if (sid_to_remove != -1) {
    remove_source_at(sid_to_remove);
  }
}

void SoundManager::update(bool play_sound) {
  if (!IsAudioStreamProcessed(stream)) {
    return;
  }

  memset(samples.data(), 0, samples.size() * sizeof(float));

  for (auto &pair : sources) {
    pair.source->update(play_sound && pair.enable, time);
    const float* source_samples = pair.source->get_samples();
    for (int i = 0; i < buffer.size(); i += 1) {
      samples[i] += source_samples[i];
    }
  }

  for (int i = 0; i < buffer.size(); i += 1) {
    buffer[i] = static_cast<short>(((1 << 15) - 1) * samples[i]);
  }

  time = fmod(time + (1.0 / kAudioSampleRate) * buffer.size(), 1.0);

  UpdateAudioStream(stream, buffer.data(), buffer.size());
}

void SoundManager::add_source(std::unique_ptr<SoundSource> source) {
  sources.push_back(sound_source_pair {
    std::move(source),
    true,
  });
}

void SoundManager::remove_source_at(size_t index) {
  sources.erase(sources.begin() + index);
}
