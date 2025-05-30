// ui.h
#pragma once

// Forward declaration
class App;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

class UI {
public:
    void SetupImGui(App& app);
    void DrawUI(App& app);
    void ShutdownImGui();
    
    // Add empty virtual destructor
    virtual ~UI() = default;
    
private:
    void DrawMenuBar(App& app);
    void DrawSettingsWindow(App& app);
    void DrawPresetWindow(App& app);
    void DrawTabsWindow(App& app);
    void DrawPointsTab(App& app);
    void DrawLinesTab(App& app);
    void DrawPlanesTab(App& app);
    void DrawDihedralViewport(App& app);
};