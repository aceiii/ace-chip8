#include "applog.h"

AppLog::AppLog() : auto_scroll(true) {
  clear();
}

void AppLog::add_log(const std::string& log) {
  int old_size = buffer.size();
  buffer.append(log.c_str());
  for (int new_size = buffer.size(); old_size < new_size; old_size += 1) {
    if (buffer[old_size] == '\n') {
      line_offsets.push_back(old_size + 1);
    }
  }
}

void AppLog::clear() {
  buffer.clear();
  line_offsets.clear();
  line_offsets.push_back(0);
}

void AppLog::draw() {
  if (ImGui::BeginPopup("Options")) {
      ImGui::Checkbox("Auto-scroll", &auto_scroll);
      ImGui::EndPopup();
  }

  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("Options");
  }

  ImGui::SameLine();
  bool should_clear = ImGui::Button("Clear");
  ImGui::SameLine();
  bool did_copy = ImGui::Button("Copy");
  ImGui::SameLine();
  filter.Draw("Filter", -100.0f);

  ImGui::Separator();

  if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
  {
      if (should_clear) {
          clear();
      }

      if (did_copy) {
        ImGui::LogToClipboard();
      }

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      const char* buf = buffer.begin();
      const char* buf_end = buffer.end();
      if (filter.IsActive()) {
        for (int line_no = 0; line_no < line_offsets.Size; line_no++) {
          const char* line_start = buf + line_offsets[line_no];
          const char* line_end = (line_no + 1 < line_offsets.Size) ? (buf + line_offsets[line_no + 1] - 1) : buf_end;
          if (filter.PassFilter(line_start, line_end)) {
            ImGui::TextUnformatted(line_start, line_end);
          }
        }
      } else {
        ImGuiListClipper clipper;
        clipper.Begin(line_offsets.Size);
        while (clipper.Step()) {
          for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
            const char* line_start = buf + line_offsets[line_no];
            const char* line_end = (line_no + 1 < line_offsets.Size) ? (buf + line_offsets[line_no + 1] - 1) : buf_end;
            ImGui::TextUnformatted(line_start, line_end);
          }
        }
        clipper.End();
      }
      ImGui::PopStyleVar();

      if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
      }
  }
  ImGui::EndChild();
};
