#include <imgui.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// We modify styles here so it doesnt get messy in the main file
void setStyle() {
    ImGuiStyle * style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.34f);
}