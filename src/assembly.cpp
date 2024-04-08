#include "assembly.h"

#include <cstdint>
#include <imgui.h>
#include <spdlog/spdlog.h>

#ifdef _MSC_VER
#define _PRISizeT "I"
#define ImSnprintf _snprintf
#else
#define _PRISizeT "z"
#define ImSnprintf snprintf
#endif

void AssemblyViewer::initialize(const registers *regs_) {
  spdlog::trace("Initializing AssemblyViewer");
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

  if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    ImGuiStyle &style = ImGui::GetStyle();

    auto line_total_count = static_cast<int>(regs->mem.size() / 2);
    float line_height = ImGui::GetTextLineHeight();

    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, line_height);

    const char *format_address = "%0*" _PRISizeT "X: ";
    const char *format_data = " %0*" _PRISizeT "X";

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

        ImGui::SameLine();
        ImGui::Text("  ");
        ImGui::SameLine();

        uint16_t instr = (hi << 8) | lo;
        ImGui::Text(disassembled_instruction(instr).c_str());

        if (is_greyed_out || is_current_line) {
          ImGui::PopStyleColor();
        }
      }
    }

    if (auto_scroll) {
      float scroll_y = line_height * static_cast<float>(regs->pc >> 1);
      ImGui::SetScrollY(scroll_y);
    }

    ImGui::PopStyleVar(2);
  }
  ImGui::EndChild();
}

void AssemblyViewer::cleanup() { spdlog::trace("Initializing AssemblyViewer"); }

std::string AssemblyViewer::disassembled_instruction(uint16_t instr) const {
  int nnn = instr & 0xfff;
  int n = instr & 0xf;
  int nn = instr & 0xff;
  int x = (instr >> 8) & 0xf;
  int y = (instr >> 4) & 0xf;

  switch ((instr >> 12) & 0xf) {
  case 0x0:
    if (instr == 0x00e0) {
      return "CLS";
    } else if (instr == 0x00ee) {
      return "RET";
    } else if (((n >> 4) & 0xf) == 0xc) {
      // Super Chip-48
      return fmt::format("SCD {:x}h", n);
    } else if (nn == 0xfb) {
      // Super Chip-48
      return "SCR";
    } else if (nn == 0xfc) {
      // Super Chip-48
      return "SCL";
    } else if (nn == 0xfd) {
      // Super Chip-48
      return "EXIT";
    } else if (nn == 0xfe) {
      // Super Chip-48
      return "LOW";
    } else if (nn == 0xff) {
      // Super Chip-48
      return "HIGH";
    } else {
      return fmt::format("SYS {:03x}h", nnn);
    }
  case 0x1:
    return fmt::format("JP {:03x}h", nnn);
  case 0x2:
    return fmt::format("CALL {:03x}h", nnn);
  case 0x3:
    return fmt::format("SE V{:x}, {:02x}h", x, nn);
  case 0x4:
    return fmt::format("SNE V{:x}, {:02x}h", x, nn);
  case 0x5:
    if (n == 0) {
      return fmt::format("SE V{:x}, V{:x}", x, y);
    }
    break;
  case 0x6:
    return fmt::format("LD V{:x}, {:02x}h", x, nn);
  case 0x7:
    return fmt::format("ADD V{:x}, {:02x}h", x, nn);
  case 0x8:
    switch (n) {
    case 0:
      return fmt::format("LD V{:x} V{:x}", x, y);
    case 1:
      return fmt::format("OR V{:x}, V{:x}", x, y);
    case 2:
      return fmt::format("AND V{:x}, V{:x}", x, y);
    case 3:
      return fmt::format("XOR V{:x}, V{:x}", x, y);
    case 4:
      return fmt::format("ADD V{:x}, V{:x}", x, y);
    case 5:
      return fmt::format("SUB V{:x}, V{:x}", x, y);
    case 6:
      return fmt::format("SHR V{:x}, V{:x}", x, y);
    case 7:
      return fmt::format("SUBN V{:x}, V{:x}", x, y);
    case 0xe:
      return fmt::format("SHL V{:x}, V{:x}", x, y);
    default:
      break;
    }
    break;
  case 0x9:
    if (n == 0) {
      return fmt::format("SNE V{:x}, V{:x}", x, y);
    }
    break;
  case 0xa:
    return fmt::format("LD I, {:03x}h", nnn);
  case 0xb:
    return fmt::format("JP V0, {:03x}h", nnn);
  case 0xc:
    return fmt::format("RND V{:x}, {:02x}h", x, nn);
  case 0xd:
    return fmt::format("DRW V{:x}, V{:x}, {:x}h", x, y, n);
  case 0xe:
    switch (nn) {
    case 0x9e:
      return fmt::format("SKP V{:x}", x);
    case 0xa1:
      return fmt::format("SKNP V{:x}", x);
    default:
      break;
    }
    break;
  case 0xf:
    switch (nn) {
    case 0x07:
      return fmt::format("LD V{:x}, DT", x);
    case 0x0a:
      return fmt::format("LD V{:x}, K", x);
    case 0x15:
      return fmt::format("LD DT, V{:x}", x);
    case 0x18:
      return fmt::format("LD ST, V{:x}", x);
    case 0x1e:
      return fmt::format("ADD I, V{:x}", x);
    case 0x29:
      return fmt::format("LD F, V{:x}", x);
    case 0x33:
      return fmt::format("LD B, V{:x}", x);
    case 0x55:
      return fmt::format("LD [I], V{:x}", x);
    case 0x65:
      return fmt::format("LD V{:x}, [I]", x);
    // Super Chip-48 Instructions
    case 0x30:
      return fmt::format("LD HF, V{:x}", x);
    case 0x75:
      return fmt::format("LD R, V{:x}", x);
    case 0x85:
      return fmt::format("LD V{:x}, R", x);
    default:
      break;
    }
    break;
  }

  return "";
}
