#include "assembly.h"

#include <spdlog/spdlog.h>
#include <cstdint>
#include <imgui.h>

#ifdef _MSC_VER
#define _PRISizeT   "I"
#define ImSnprintf  _snprintf
#else
#define _PRISizeT   "z"
#define ImSnprintf  snprintf
#endif

void AssemblyViewer::initialize(const registers *regs_) {
  spdlog::trace("Initialzing AssemblyViewer");
  regs = regs_;
}

void AssemblyViewer::draw() {
  if (ImGui::BeginPopup("Options")) {
      ImGui::Checkbox("Auto-scroll", &auto_scroll);
      ImGui::EndPopup();
  }

  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("Options");
  }

  if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
  {
    ImGuiStyle &style = ImGui::GetStyle();

    const int line_total_count = regs->mem.size() / 2;
    float line_height = ImGui::GetTextLineHeight();

    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, line_height);

    const char* format_address = "%0*" _PRISizeT "X: ";
    const char* format_data = " %0*" _PRISizeT "X";

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    while (clipper.Step()) {
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i += 1) {
        size_t addr = static_cast<uint16_t>(i * 2);
        bool is_current_line = addr == regs->pc;

        uint8_t hi = regs->mem[addr];
        uint8_t lo = regs->mem[addr + 1];

        bool is_greyed_out = (addr < kRomStartIndex) && (hi == 0 && lo == 0);

        if (is_current_line) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0, 1.0));
        }

        ImGui::Text(format_address, 3, addr);

        if (is_current_line) {
          ImGui::PopStyleColor();
        }

        if (is_greyed_out) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
        } else if (is_current_line) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0, 1.0));
        }

        ImGui::SameLine();
        ImGui::Text(format_data, 2, hi);
        ImGui::SameLine();
        ImGui::Text(format_data, 2, lo);

        if (is_greyed_out || is_current_line) {
          ImGui::PopStyleColor();
        }
      }
    }

    if (auto_scroll) {
      float scroll_y = line_height * (regs->pc >> 1);
      ImGui::SetScrollY(scroll_y);
    }

    ImGui::PopStyleVar(2);
  }
  ImGui::EndChild();
}

void AssemblyViewer::cleanup() {
  spdlog::trace("Initialzing AssemblyViewer");
}
