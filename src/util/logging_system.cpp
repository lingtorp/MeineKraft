#include "logging_system.hpp"

#include "imgui/imgui.h"

#define LOG_FILTER_ALL 0xFFFF
#define LOG_FILTER_DEBUG 0x0001
#define LOG_FILTER_WARN  0x0002
#define LOG_FILTER_ERROR 0x0004

void LoggingSystem::draw_gui(bool* open) {
  ImGui::SetWindowSize(ImVec2(500, 450), ImGuiSetCond_Once);

  if (ImGui::Begin("Logger system", open, ImGuiWindowFlags_MenuBar)) {

    char buf[1024] = {0};
    if (ImGui::Button("Clear")) {
      memset(buf, 0, 1024);
    }
    ImGui::SameLine();
    ImGui::InputText("Search ..", buf, sizeof(buf));

    ImGui::Separator();

    ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const LogMsg& log : buffer) {
      ImGui::Text("%s\n", log.msg.c_str());
    }

    static bool scroll_to_bottom = false;
    if (scroll_to_bottom) {
      ImGui::SetScrollHere();
    }

    ImGui::EndChild();

    if (ImGui::Button("Warnings")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("Errors")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("Debug")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("Clear flags")) {
      filter_flags &= LOG_FILTER_ALL;
    }

    ImGui::SameLine();
    if (ImGui::Button("Lock to bottom")) {
      scroll_to_bottom = !scroll_to_bottom;
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear log")) {
      buffer.clear();
    }

    ImGui::End();
  }
}
