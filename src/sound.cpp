#include "sound.h"

#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <raylib.h>
#include <imgui.h>
#include <random>

void SoundSource::initialize() {
  SetAudioStreamBufferSizeDefault(kMaxSamplesPerUpdate);
  stream = LoadAudioStream(kAudioSampleRate, kAudioSampleSize, kAudioNumChannels);
  PlayAudioStream(stream);
}

void SoundSource::cleanup() {
  StopAudioStream(stream);
  UnloadAudioStream(stream);
}

void SoundSource::update(bool play_sound) {
  if (!IsAudioStreamProcessed(stream)) {
    return;
  }

  gen_sound_data(play_sound, buffer.data(), buffer.size());
  UpdateAudioStream(stream, buffer.data(), buffer.size());
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
  ImGui::PlotLines("Sound", samples.data(), samples.size(), 0, nullptr, -1.5f, 1.5f, ImVec2(0, 80.0f));
  ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f);
  ImGui::SliderFloat("Frequency", &frequency, 10.0f, 2048.0f);
  ImGui::Checkbox("Force Play", &force_play);
}

static inline float noise(float idx) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> dist(-1.0f, 1.0f);
  return dist(gen);
}

static inline float sine_wave(float idx) {
  return std::sin(2 * PI * idx);
}

static inline float square_wave(float idx) {
  float s = sine_wave(idx);
  if (std::abs(s) <= 0.001f) {
    return 0.f;
  }
  return std::abs(s) / s;
}

static inline float triangle_wave(float idx) {
  return std::abs(fmod(idx, 1.0f) - 0.5f) * 4 - 1;
}

static inline float sawtooth_wave(float idx) {
  return std::abs(fmod(idx, 1.0f)) * 2 - 1;
}

void WaveGeneratorSource::gen_sound_data(bool play_sound, short buffer[], size_t buffer_size) {
  if (!play_sound && !force_play) {
    memset(buffer, 0, sizeof(short) * buffer_size);
    memset(&samples, 0, sizeof(float) * samples.size());
    return;
  }

  float incr = frequency / static_cast<float>(kAudioSampleRate);

  auto wave_func = ([this]() {
    switch (wave_type) {
      case 0: return sine_wave;
      case 1: return square_wave;
      case 2: return triangle_wave;
      case 3: return sawtooth_wave;
      default: return noise;
    }
  })();

  for (int i = 0; i < buffer_size; i++) {
    float sample = wave_func(wave_idx) * (volume / 100.0f);
    buffer[i] = static_cast<short>(((1 << 15) - 1) * sample);

    if (i < samples.size()) {
      samples[i] = sample;
    }

    wave_idx += incr;
    if (wave_idx > 1.0f)
      wave_idx -= 1.0f;
  }
}
