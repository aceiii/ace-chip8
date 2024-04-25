#pragma once

#include <raylib.h>
#include <array>

constexpr int kMaxSamplesPerUpdate = 4096;
constexpr int kAudioSampleRate = 44100;
constexpr int kAudioSampleSize = 16;
constexpr int kAudioNumChannels = 1;

class SoundSource {
public:
  virtual void render() = 0;
  virtual void gen_sound_data(bool play_sound, short buffer[], size_t buffer_size) = 0;

  virtual void initialize();
  void update(bool play_sound);
  virtual void cleanup();

private:
  AudioStream stream;
  std::array<short, kMaxSamplesPerUpdate> buffer;
};

class WaveGeneratorSource : public SoundSource {
public:
  virtual void render() override;
  virtual void gen_sound_data(bool play_sound, short buffer[], size_t buffer_size) override;

private:
  bool force_play = false;
  float frequency = 440.0f;
  float volume = 100.0f;
  float wave_idx = 0.0f;
  int wave_type = 0;
};
