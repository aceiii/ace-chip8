#include "screen.h"

#include <spdlog/spdlog.h>
#include <rlImGui.h>


void Screen::initialize(int width, int height, int pixel_size_, const bool pixels_[]) {
  spdlog::debug("Initializing screen {}x{}", width, height);
  screen_width = width;
  screen_height = height;
  pixel_size = pixel_size_;
  pixels = pixels_;
  screen_texture = LoadRenderTexture(screen_width * pixel_size, screen_height * pixel_size);
}

void Screen::update() {
  BeginTextureMode(screen_texture);
  for (int y = 0; y < screen_height; y += 1) {
    for (int x = 0; x < screen_width; x += 1) {
      int idx = (y * screen_width) + x;
      bool px = pixels[idx];
      DrawRectangle(x * pixel_size, y * pixel_size, pixel_size, pixel_size, px ? RAYWHITE : BLACK);
    }
  }
  EndTextureMode();
}

void Screen::draw() {
  rlImGuiImageRenderTextureFit(&screen_texture, true);
}

void Screen::cleanup() {
  spdlog::debug("Cleaning up screen");
  UnloadRenderTexture(screen_texture);
}
