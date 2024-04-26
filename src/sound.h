#pragma once

#include <memory>
#include <raylib.h>
#include <array>
#include <vector>

constexpr int kMaxSamplesPerUpdate = 1024;
constexpr int kAudioSampleRate = 44100;
constexpr int kAudioSampleSize = 16;
constexpr int kAudioNumChannels = 1;

class SoundSource {
public:
  virtual ~SoundSource() = default;

  virtual void render() = 0;
  virtual void update(bool play_sound, double time) = 0;

  auto get_samples() const {
    return samples.data();
  }

protected:
  std::array<float, kMaxSamplesPerUpdate> samples;
};

class WaveGeneratorSource final : public SoundSource {
public:
  virtual ~WaveGeneratorSource() = default;

  virtual void render() override;
  virtual void update(bool play_sound, double time) override;

private:
  bool force_play = false;
  float frequency = 440.0f;
  float offset = 0.0f;
  float volume = 50.0f;
  int wave_type = 0;
};

class SoundManager final {
public:
  void initialize();
  void render();
  void update(bool play_sound);
  void cleanup();

  void add_source(std::unique_ptr<SoundSource> source);
  void remove_source_at(size_t index);

private:
  double time = 0;
  AudioStream stream;
  std::array<float, kMaxSamplesPerUpdate> samples;
  std::array<short, kMaxSamplesPerUpdate> buffer;
  std::vector<std::unique_ptr<SoundSource>> sources;
};
