#pragma once

#include <raylib.h>

struct screen_settings {
  bool show_grid_lines;
  Color grid_color;
  Color pixel_on;
  Color pixel_off;
};

class Screen {
public:
  void initialize(int width, int height, int pixel_size, const bool pixels[]);
  void update(screen_settings settings);
  void draw();
  void cleanup();

private:
  RenderTexture2D screen_texture;
  const bool *pixels = nullptr;
  int screen_width = 0, screen_height = 0, pixel_size = 0;
};
