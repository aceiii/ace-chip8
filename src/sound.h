#pragma once

#include <memory>
#include <raylib.h>
#include <array>
#include <vector>
#include <string>

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

class WaveFileSource final : public SoundSource {
public:
  virtual ~WaveFileSource();

  virtual void render() override;
  virtual void update(bool play_sound, double time) override;

  void cleanup();
  void load_wave(const char* filename);
  void open_load_wave_dialog();

private:
  float *wav_samples = nullptr;
  float wav_min = -1.5f;
  float wav_max = 1.5f;
  std::string wav_filename;
  int frame_count = 0;
  int current_frame = 0;
  bool force_play = false;
  float volume = 100.0f;
  float volume_mulitplier = 1.0f;
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
