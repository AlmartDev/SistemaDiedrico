// ui.h
#pragma once

// Forward declaration
class App;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <string>

#include "dihedral.h"

#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
#include "ImGuiFileDialog.h"
#endif

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

    DihedralViewport dihedralViewport;
    void DrawDihedralViewport(App& app);

    // File dialogs
    void OpenFileDialog(App& app);
    void SaveFileDialog(App& app);

    // TRANSLATION ------------------
    void loadTranslations(const std::string& path);
    std::string SetText(const std::string& key, const std::string& language);

    // translation data
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translations;
    std::vector<std::string> availableLanguages;

    std::string currentLanguage = "EN";
};