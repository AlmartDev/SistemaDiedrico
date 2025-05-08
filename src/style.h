// style.h
#pragma once

#include <imgui.h>

inline void SetCustomStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.34f);
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
}