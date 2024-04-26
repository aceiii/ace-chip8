#include "sound.h"

#include <array>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <raylib.h>
#include <imgui.h>
#include <random>

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

void WaveGeneratorSource::render() {
  static const char* const wave_type_names[] = {
    "Sine",
    "Square",
    "Triangle",
    "Sawtooth",
    "Noise",
  };

  if (ImGui::BeginCombo("Type", wave_type_names[wave_type])) {
    for (int n = 0; n < IM_ARRAYSIZE(wave_type_names); n += 1) {
      bool is_selected = wave_type == n;
      if (ImGui::Selectable(wave_type_names[n], is_selected)) {
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

  auto wave_func = ([this]() {
    switch (wave_type) {
      case 0: return sine_wave;
      case 1: return square_wave;
      case 2: return triangle_wave;
      case 3: return sawtooth_wave;
      default: return noise;
    }
  })();

  double incr = 1.0 / kAudioSampleRate;

  for (int i = 0; i < samples.size(); i++) {
    samples[i] = wave_func((time * frequency) + offset) * (volume / 100.0f);
    time += incr;
  }
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
    ImGui::PushID(sid);
    ImGui::BeginChild("##Sound", ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::Text("Sound #%d", (sid + 1));

    sources[sid]->render();

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

  for (auto &source : sources) {
    source->update(play_sound, time);
    const float* source_samples = source->get_samples();
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
  sources.push_back(std::move(source));
}

void SoundManager::remove_source_at(size_t index) {
  sources.erase(sources.begin() + index);
}
