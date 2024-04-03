#pragma once

#include <imgui.h>
#include <string>

class AppLog {
public:
  AppLog();

  void add_log(const std::string& string);
  void clear();
  void draw();

private:
  ImGuiTextBuffer buffer;
  ImGuiTextFilter filter;
  ImVector<int> line_offsets;
  bool auto_scroll;
};
