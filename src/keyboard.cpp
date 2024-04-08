#include "keyboard.h"

#include <spdlog/spdlog.h>
#include <raylib.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>

Keyboard::Keyboard() {
  mapping.push_back({"1", KEY_ONE, 0x1});
  mapping.push_back({"2", KEY_TWO, 0x2});
  mapping.push_back({"3", KEY_THREE, 0x3});
  mapping.push_back({"C", KEY_FOUR, 0xc});

  mapping.push_back({"4", KEY_Q, 0x4});
  mapping.push_back({"5", KEY_W, 0x5});
  mapping.push_back({"6", KEY_E, 0x6});
  mapping.push_back({"D", KEY_R, 0xd});

  mapping.push_back({"7", KEY_A, 0x7});
  mapping.push_back({"8", KEY_S, 0x8});
  mapping.push_back({"9", KEY_D, 0x9});
  mapping.push_back({"E", KEY_F, 0xe});

  mapping.push_back({"A", KEY_Z, 0xa});
  mapping.push_back({"0", KEY_X, 0x0});
  mapping.push_back({"B", KEY_C, 0xb});
  mapping.push_back({"F", KEY_V, 0xf});
}

void Keyboard::initialize(std::shared_ptr<registers> regs_) {
  regs = std::move(regs_);
}

void Keyboard::update() {
  for (auto &m : mapping) {
    regs->kbd[m.key] = IsKeyDown(m.keycode);
  }
}

void Keyboard::draw() {
  auto push_disabled_flags = [] () {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  };

  auto pop_disabled_flags = [] () {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  };

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(24, 18));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

  int n = 0;
  for (auto &m : mapping) {
    if (n % 4 != 0) {
      ImGui::SameLine();
    }

    bool is_disabled = regs->kbd[m.key];

    if (is_disabled) {
      push_disabled_flags();
    }

    if (ImGui::Button(m.label.c_str())) {
      regs->kbd[m.key];
    }

    if (is_disabled) {
      pop_disabled_flags();
    }

    n += 1;
  }

  ImGui::PopStyleVar(3);
}
