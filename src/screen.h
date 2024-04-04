#pragma once

#include <raylib.h>

class Screen {
public:
  void initialize(int width, int height, int pixel_size, const bool pixels[]);
  void update();
  void draw();
  void cleanup();

private:
  RenderTexture2D screen_texture;
  const bool* pixels = nullptr;
  int screen_width = 0, screen_height = 0, pixel_size = 0;
};
