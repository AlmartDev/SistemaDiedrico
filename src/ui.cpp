#include "ui.h"
#include "app.h"

#include <algorithm>
#include <string>
#include <string>
#include <sstream>

#include "style.h"

#define PROGRAM_VERSION "0.16.8"

#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
    #include "ImGuiFileDialog.h"
    #include "ImGuiFileDialogConfig.h"
#endif

// all text except for debug stuff is in this file

void UI::SetupImGui(App& app) {
    auto& sceneData = app.GetSceneData();
    auto window = app.GetWindow();

    // Setup Dear ImGui context
IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetCustomStyle();

    ImGuiIO& io = ImGui::GetIO();
    
#ifdef __EMSCRIPTEN__
    const char* fontPath = "/assets/fonts/Roboto-Regular.ttf"; 
    const char* iconFontPath = "/assets/fonts/MaterialIcons-Regular.ttf";
    const char* languagePath = "/assets/languages.csv";
#else
    const char* fontPath = "./assets/fonts/Roboto-Regular.ttf"; 
    const char* iconFontPath = "./assets/fonts/MaterialIcons-Regular.ttf";
    const char* languagePath = "./assets/languages.csv";
#endif
    // load fonts
    io.Fonts->AddFontFromFileTTF(fontPath, sceneData.settings.fontSize);
    ImFontConfig iconFontConfig;
    iconFontConfig.MergeMode = true; 
    iconFontConfig.PixelSnapH = true; 
    io.Fonts->AddFontFromFileTTF(iconFontPath, sceneData.settings.fontSize - 1.0f, &iconFontConfig, io.Fonts->GetGlyphRangesDefault());

    
    loadTranslations(languagePath);

    // check if default languae is inside the avaliable languages
    if (std::find(availableLanguages.begin(), availableLanguages.end(), sceneData.settings.defaultLanguage) != availableLanguages.end()) {
        currentLanguage = sceneData.settings.defaultLanguage;
    } else {
        std::cerr << (sceneData.settings.defaultLanguage == availableLanguages[2]) << std::endl;
    }
    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 100");
#endif
}

// TRANSLATIONS -----------------------------------------------------
void UI::loadTranslations(const std::string& path) {
    translations.clear(); // Clear existing translations
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening translation file: " << path << std::endl;
        return;
    }

    std::string line;
    
    // Read header line to get languages
    if (!std::getline(file, line)) {
        std::cerr << "Translation file is empty" << std::endl;
        return;
    }
    
    std::vector<std::string> languages;
    std::istringstream headerStream(line);
    std::string cell;
    
    // Skip first two columns (id and key)
    std::getline(headerStream, cell, ',');
    std::getline(headerStream, cell, ',');
    
    // Get language codes from remaining columns
    while (std::getline(headerStream, cell, ',')) {
        languages.push_back(cell);
    }

    // Process each line of translations
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines
        
        std::istringstream lineStream(line);
        std::string id, key;
        
        // Get id and key
        if (!std::getline(lineStream, id, ',')) continue;
        if (!std::getline(lineStream, key, ',')) continue;
        
        if (key.empty()) continue; // Skip lines without keys
        
        // Get translations for each language
        std::vector<std::string> texts;
        while (std::getline(lineStream, cell, ',')) {
            texts.push_back(cell);
        }
        
        // Store translations in the map
        for (size_t i = 0; i < texts.size() && i < languages.size(); ++i) {
            if (!texts[i].empty()) { // Only store non-empty translations
                translations[key][languages[i]] = texts[i];
            }
        }
    }

    availableLanguages.clear();
    for (const auto& lang : languages) {
        if (!lang.empty()) {
            std::cout << "Loaded language: " << lang << std::endl;
            std::string trimmedLang = lang;
            trimmedLang.erase(trimmedLang.find_last_not_of(" \t\n\r\f\v") + 1);
            trimmedLang.erase(0, trimmedLang.find_first_not_of(" \t\n\r\f\v"));
            availableLanguages.push_back(trimmedLang);
        }
    }

    file.close();
}

std::string UI::SetText(const std::string& key, const std::string& language) {

    auto it = translations.find(key);
    if (it != translations.end()) {
        // Trim whitespace from language code before lookup
        std::string trimmedLang = language;
        trimmedLang.erase(trimmedLang.find_last_not_of(" \t\n\r\f\v") + 1);
        trimmedLang.erase(0, trimmedLang.find_first_not_of(" \t\n\r\f\v"));
        auto langIt = it->second.find(trimmedLang);
        if (langIt != it->second.end()) {
            return langIt->second;
        }
    }

    // Return the key itself if no translation found
    return key;
}
// TRANSLATIONS -----------------------------------------------------

void UI::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::DrawUI(App& app) {
    app.UpdateWindowTitle(std::string(" - ") + PROGRAM_VERSION);

    int width = app.GetWindowWidth();
    int height = app.GetWindowHeight();

    windowPositions.settings = ImVec2(60, 60);
    windowPositions.tabs = ImVec2(60, 305);
    windowPositions.presets = ImVec2(60, height - 60 - ImGui::GetTextLineHeightWithSpacing() * 10);

    DrawMenuBar(app);
    DrawSettingsWindow(app);
    DrawPresetWindow(app);
    DrawTabsWindow(app);
}

void OpenURL(const std::string& url) {
#ifdef __EMSCRIPTEN__
    emscripten_run_script(("window.open('" + url + "', '_blank');").c_str());
#elif _WIN32
    std::string command = "start " + url; // For Windows
    system(command.c_str());
#else
    std::string command = "xdg-open " + url; // For Linux
    system(command.c_str());
#endif
}



// IMGUI DIALOGS -----------------------------------------
void UI::OpenFileDialog(App& app) {
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
    // Set filters
    const char* filters = "JSON files (*.json){.json},.*";
    
    // Open file dialog
    IGFD::FileDialogConfig config;
    config.flags = ImGuiFileDialogFlags_Default;
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", filters, config);
#endif
}

void UI::SaveFileDialog(App& app) {
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
    // Set filters
    const char* filters = "JSON files (*.json){.json},.*";

    IGFD::FileDialogConfig config;
    config.flags = ImGuiFileDialogFlags_Default;    
    ImGuiFileDialog::Instance()->OpenDialog("SaveFileDlgKey", "Save File", filters, config);
#endif
}

void UI::SetIcon(const std::string& iconName) {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* iconFont = io.Fonts->Fonts.back(); // Get the last font, which should be the icon font
    ImGui::PushFont(iconFont);
    ImGui::TextUnformatted(iconName.c_str());
    ImGui::PopFont();
    ImGui::SameLine(0, 4); // Add a small space after the icon, before the next element
}

void UI::DrawMenuBar(App& app) {
    auto& sceneData = app.GetSceneData();
    int width = app.GetWindowWidth();
    int height = app.GetWindowHeight();

    if (ImGui::BeginMainMenuBar()) {
        bool fileMenuOpen = ImGui::BeginMenu((SetText("menu_file", currentLanguage)).c_str());
        if (fileMenuOpen) {
            SetIcon(u8"\uE2C7");
            if (ImGui::MenuItem(SetText("menu_open", currentLanguage).c_str(), "Ctrl+O")) {
                #ifdef __EMSCRIPTEN__
                    JsonHandler* handler = JsonHandlerInstance();
                    handler->OpenFileDialog();
                #elif _WIN32
                    std::string path = app.GetJsonHandler().OpenFileDialog();
                    std::vector<nlohmann::json> data = app.GetJsonHandler().Load(path);
                    if (!data.empty()) {
                        std::string fileName = path.substr(path.find_last_of('\\') + 1);
                        sceneData.settings.loadedFileName = fileName;
                        app.LoadProject(data);
                    }
                #else
                    OpenFileDialog(app);
                #endif
                }

            #ifdef __EMSCRIPTEN__
                SetIcon(u8"\uF090"); // download icon
            #else
                SetIcon(u8"\uE161"); // save icon 
            #endif

            if (ImGui::MenuItem(SetText("menu_save", currentLanguage).c_str(), "Ctrl+S")) {
            #if defined(__EMSCRIPTEN__) || defined(_WIN32)
                std::string path = app.GetJsonHandler().SaveFileDialog();
                app.GetJsonHandler().Save(path, app.GetSceneData());
            #else
                SaveFileDialog(app);
            #endif
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(SetText("menu_app", currentLanguage).c_str())) {
            if (availableLanguages.size() > 1) {
                SetIcon(u8"\uE894");
                if (ImGui::BeginMenu(SetText("menu_lang", currentLanguage).c_str())) {
                    ImGui::TextColored(ImVec4(0.2f, 0.5f, 1.0f, 1.0f), "./languages.csv");
                    for (const std::string& lang : availableLanguages) {
                        bool isCurrent = (currentLanguage == lang); // Case-sensitive comparison is fine if data is consistent
                        
                        if (ImGui::MenuItem(lang.c_str(), nullptr, isCurrent)) {
                            if (!isCurrent) {  // Only change if it's actually different
                                currentLanguage = lang;
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
            }
            SetIcon(u8"\uEFE9"); 
            if (ImGui::MenuItem(SetText("menu_clear_scene", currentLanguage).c_str())) {
                app.GetCamera().ResetPosition();
                sceneData.points.clear();
                sceneData.lines.clear();
                sceneData.planes.clear();
                sceneData.settings = SceneData::Settings();
            }
            SetIcon(u8"\uE04B"); 
            if (ImGui::MenuItem(SetText("menu_reset_cam", currentLanguage).c_str())) {
                app.GetCamera().ResetPosition();
            }
            SetIcon(u8"\uE8B8"); 
            if (ImGui::MenuItem(SetText("menu_reset_settings", currentLanguage).c_str())) {
                sceneData.settings = SceneData::Settings();
            }
            
            #ifndef __EMSCRIPTEN__
            ImGui::Separator();
            SetIcon(u8"\uE5CD");
            if (ImGui::MenuItem(SetText("menu_exit", currentLanguage).c_str())) {
                glfwSetWindowShouldClose(app.GetWindow(), true);
            }
            #endif
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(SetText("menu_help", currentLanguage).c_str())) {
            SetIcon(u8"\uE853");
            if (ImGui::MenuItem("GitHub")) {
                OpenURL("https://github.com/AlmartDev/SistemaDiedrico");
            }
            SetIcon(u8"\uE873");
            if (ImGui::MenuItem(SetText("menu_docs", currentLanguage).c_str())) {
                OpenURL("https://github.com/AlmartDev/SistemaDiedrico/wiki");
            }
            SetIcon(u8"\uE88E");
            if (ImGui::BeginMenu(SetText("menu_about", currentLanguage).c_str())) {
                ImGui::Text("v%s", PROGRAM_VERSION);
                ImGui::Text(SetText("about_author", currentLanguage).c_str());
                ImGui::Text(SetText("about_username", currentLanguage).c_str());
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (sceneData.settings.loadedFileName != "") {
            float textWidth = ImGui::CalcTextSize(sceneData.settings.loadedFileName.c_str()).x;
            float menuBarWidth = ImGui::GetWindowWidth();
            float cursorX = (menuBarWidth - textWidth) / 2.0f;
            ImGui::SetCursorPosX(cursorX);
            ImGui::TextColored(ImVec4(0.2f, 0.5f, 1.0f, 1.0f), "%s", sceneData.settings.loadedFileName.c_str());
            app.UpdateWindowTitle(std::string("Sistema Diedrico") + " - " + PROGRAM_VERSION + " - " + sceneData.settings.loadedFileName);
        }

        ImGui::EndMainMenuBar();
    }

#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
    // Open file dialog
    ImGui::SetNextWindowSize(ImVec2(900, 750), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::vector<nlohmann::json> data = app.GetJsonHandler().Load(filePath);
            std::cout << "Loaded file: " << filePath << std::endl;
            if (!data.empty()) {
                std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);
                sceneData.settings.loadedFileName = fileName;
                app.LoadProject(data);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Save file dialog
    ImGui::SetNextWindowSize(ImVec2(900, 750), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::cout << "Saving file: " << filePath << std::endl;
            app.GetJsonHandler().Save(filePath, app.GetSceneData());
        }
        ImGuiFileDialog::Instance()->Close();
    }
#endif

#ifdef __EMSCRIPTEN__
    if (sceneData.settings.showWelcomeWindow) {
        // it has to be exacly in the middle of the screen
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);

        if (ImGui::Begin(SetText("welcome_title", currentLanguage).c_str(), &sceneData.settings.showWelcomeWindow,
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoResize)) {
            SetIcon(u8"\uE88E"); 
            ImGui::Text(SetText("welcome_message", currentLanguage).c_str());
            ImGui::Text(SetText("welcome_importance", currentLanguage).c_str());
            ImGui::Separator();
            ImGui::Text("v%s - WEB", PROGRAM_VERSION);
            ImGui::Text(SetText("about_author", currentLanguage).c_str());
            ImGui::Text(SetText("about_username", currentLanguage).c_str());
        }
        ImGui::End();
    }
#endif
}


void UI::DrawSettingsWindow(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& camera = app.GetCamera();
    auto& renderer = app.GetRenderer();

    ImGui::SetNextWindowPos(windowPositions.settings, ImGuiCond_FirstUseEver);
    ImGui::Begin(SetText("settings_title", currentLanguage).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Store translated strings to keep them alive
    std::vector<std::string> axesTypeStrings = {
        SetText("settings_axes_3d", currentLanguage),
        SetText("settings_axes_cartesian", currentLanguage),
        SetText("settings_axes_dihedral", currentLanguage),
        SetText("settings_axes_none", currentLanguage)
    };
    std::vector<const char*> axesTypes;
    for (const auto& s : axesTypeStrings) axesTypes.push_back(s.c_str());
    ImGui::Combo(SetText("settings_axes_type", currentLanguage).c_str(), &sceneData.settings.axesType, axesTypes.data(), static_cast<int>(axesTypes.size()));
    renderer.SetAxesType(sceneData.settings.axesType);

    if (sceneData.settings.axesType != 2) {
        ImGui::Checkbox(SetText("settings_show_dihedral", currentLanguage).c_str(), &sceneData.settings.showDihedralSystem);
        renderer.SetDihedralsVisible(sceneData.settings.showDihedralSystem);
    } else {
        ImGui::Checkbox(SetText("settings_show_quadrant_labels", currentLanguage).c_str(), &sceneData.settings.showQuadrantLabels);
    }

    //ImGui::SameLine();
    //ImGui::Checkbox(SetText("settings_guizmos", currentLanguage).c_str(), &sceneData.settings.showGuizmos);
    //renderer.SetGuizmosVisible(sceneData.settings.showGuizmos);

    ImGui::SliderFloat(SetText("settings_mouse_sens", currentLanguage).c_str(), &sceneData.settings.mouseSensitivity, 0.0f, 2.0f);
    camera.SetSensitivity(sceneData.settings.mouseSensitivity);

    //ImGui::SliderFloat("Camera Distance", &sceneData.settings.cameraDistance, 0.1f, 25.0f);
    //camera.SetDistance(sceneData.settings.cameraDistance);

    ImGui::DragFloat2("Offset (X, Y)", sceneData.settings.offset, 0.75f);

    // Show scale only while the slider is being actively dragged
    bool scaleChanged = ImGui::SliderFloat(SetText("settings_scale", currentLanguage).c_str(), &sceneData.settings.worldScale, 25.0f, 125.0f);
    if (ImGui::IsItemActive()) {
        renderer.SetShowScale(true, sceneData.settings.worldScale);
    } else {
        renderer.SetShowScale(false, sceneData.settings.worldScale);
    }

    // Make "More Settings" button span the width of the window
    float buttonWidth = ImGui::GetContentRegionAvail().x;
    if (buttonWidth > 350.0f) buttonWidth = 350.0f;
    if (ImGui::Button(SetText("settings_more_settings", currentLanguage).c_str(), ImVec2(buttonWidth, 0))) {
        ImGui::OpenPopup("More Settings");
    }

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 0.35f), "FPS: %.1f", ImGui::GetIO().Framerate);

    // popup
    if (ImGui::BeginPopupModal("More Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::ColorEdit3(SetText("settings_bg_color", currentLanguage).c_str(), sceneData.settings.backgroundColor);
        ImGui::ColorEdit3(SetText("settings_dihedral_bg_color", currentLanguage).c_str(), sceneData.settings.dihedralBackgroundColor);
        ImGui::ColorEdit3(SetText("settings_dihedral_line_color", currentLanguage).c_str(), sceneData.settings.dihedralLineColor);

        ImGui::Checkbox(SetText("settings_vsync", currentLanguage).c_str(), &sceneData.settings.VSync);

        ImGui::Checkbox(SetText("settings_invert_x", currentLanguage).c_str(), &sceneData.settings.invertMouse[0]);
        ImGui::SameLine();
        ImGui::Checkbox(SetText("settings_invert_y", currentLanguage).c_str(), &sceneData.settings.invertMouse[1]);

        ImGui::Separator();
        float buttonWidth = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button(SetText("multi_close", currentLanguage).c_str(), ImVec2(buttonWidth, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 0.35f), "Camera Position: (%.2f, %.2f, %.2f)", 
                camera.GetPosition().x, 
                camera.GetPosition().y, 
                camera.GetPosition().z);

        ImGui::EndPopup();
    }
    
    ImGui::End();
}

void UI::DrawTabsWindow(App& app) {
    ImGui::SetNextWindowPos(windowPositions.tabs, ImGuiCond_FirstUseEver);
    ImGui::Begin(SetText("tabs_title", currentLanguage).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);


    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem(SetText("multi_points", currentLanguage).c_str())) {
            DrawPointsTab(app);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(SetText("multi_lines", currentLanguage).c_str())) {
            DrawLinesTab(app);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(SetText("multi_planes", currentLanguage).c_str())) {
            DrawPlanesTab(app);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void UI::DrawPointsTab(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& renderer = app.GetRenderer();
    auto& camera = app.GetCamera();

    static char pointName[128] = "";
    static float pointCoords[3] = {0.0f, 0.0f, 0.0f};

    ImGui::InputText(SetText("multi_name", currentLanguage).c_str(), pointName, sizeof(pointName), ImGuiInputTextFlags_CallbackCharFilter,
        [](ImGuiInputTextCallbackData* data) -> int {
            if (data->EventChar >= 'a' && data->EventChar <= 'z') {
                data->EventChar = data->EventChar - 'a' + 'A';
            }
            return 0;
        });
    ImGui::InputFloat3(SetText("tabs_coords", currentLanguage).c_str(), pointCoords);

    if (ImGui::Button(SetText("tabs_add_point", currentLanguage).c_str())) {
        std::string name = pointName;
        name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
        name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

        if (name.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
        } 
        else if (std::any_of(sceneData.points.begin(), sceneData.points.end(), 
                            [&](const Point& p) { return p.name == name; })) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
        } 
        else {
            sceneData.points.push_back({name, {pointCoords[0], pointCoords[1], pointCoords[2]}, false, true});
            pointName[0] = '\0';
            memset(pointCoords, 0, sizeof(pointCoords));
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox(SetText("tabs_cuts", currentLanguage).c_str(), &sceneData.settings.showCutPoints);
    renderer.SetCutPointVisible(sceneData.settings.showCutPoints);

    ImGui::DragFloat(SetText("tabs_point_size", currentLanguage).c_str(), &sceneData.settings.pointSize, 0.1f, 0.1f, 100.0f);

    ImGui::Separator();
    
    // Draw point list with better styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    static int selectedPointIndex = -1; // Track selected point index
    static int lastSelectedPointIndex = -1; // Track last selected point index
    if (ImGui::BeginTable("PointTable", 4, ImGuiTableFlags_NoHostExtendX                
                                     | ImGuiTableFlags_RowBg  
                                     | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn(" Name", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Coords", ImGuiTableColumnFlags_WidthStretch, 0.0f);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sceneData.points.size(); ++i) {
            if (sceneData.points[i].hidden) continue;
            auto& point = sceneData.points[i];
            ImVec4 color(point.color[0], point.color[1], point.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImVec2 cellMin = ImGui::GetCursorScreenPos();
            ImVec2 cellMax = ImVec2(cellMin.x + ImGui::GetColumnWidth(), cellMin.y + ImGui::GetTextLineHeightWithSpacing());
            ImVec2 buttonSize = ImVec2(cellMax.x - cellMin.x, cellMax.y - cellMin.y);

            if (ImGui::InvisibleButton("##select", buttonSize)) {
                if (selectedPointIndex != static_cast<int>(i)) {
                    selectedPointIndex = static_cast<int>(i);
                    // Reset guizmo position to the new point's position
                    auto& point = sceneData.points[i];
                    renderer.SetInitialGuizmoPosition(glm::vec3(point.coords[0], point.coords[2], point.coords[1]));
                }
                else {
                    selectedPointIndex = -1;
                }
            }

            // In the rendering of selected point:
            if (static_cast<int>(i) == selectedPointIndex) {
                ImU32 highlightColor = ImGui::GetColorU32(ImGuiCol_Header);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, highlightColor);

                // Get updated position from guizmo
                glm::vec3 pos = renderer.SetPositionWithGuizmo(camera);
                point.coords[0] = pos.x;
                point.coords[1] = pos.y;
                point.coords[2] = pos.z;
            }

            // Draw the visible name text
            ImGui::SetCursorScreenPos(cellMin);
            ImGui::Text("  %s", point.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(static_cast<int>(i));
            ImGui::SetNextItemWidth(-FLT_MIN); // Use all available width in the cell

            if (ImGui::IsWindowFocused() && selectedPointIndex != static_cast<int>(i)) { // fix this later
                ImGui::DragFloat3("", point.coords, 0.1f);
            }
            else {
                ImGui::Text("%.2f, %.2f, %.2f", point.coords[0], point.coords[1], point.coords[2]);
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(2);
            if (ImGui::ColorEdit4("##Color", (float*)&color,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                point.color[0] = color.x;
                point.color[1] = color.y;
                point.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(3);
                if (ImGui::Button("X")) {
                app.DeletePoint(point);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
}

void UI::DrawLinesTab(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& renderer = app.GetRenderer();
    auto& camera = app.GetCamera();

    static char lineName[128] = "";
    ImGui::InputText(SetText("multi_name", currentLanguage).c_str(), lineName, sizeof(lineName), ImGuiInputTextFlags_CallbackCharFilter,
        [](ImGuiInputTextCallbackData* data) -> int {
            if (data->EventChar >= 'A' && data->EventChar <= 'Z') {
                data->EventChar = data->EventChar - 'A' + 'a';
            }
            return 0;
        });

    static int selectedPoint1 = -1;
    static int selectedPoint2 = -1;

    // filtered list of visible points
    std::vector<int> visiblePointIndices;
    std::vector<const char*> visiblePointNames;
    for (size_t i = 0; i < sceneData.points.size(); ++i) {
        if (!sceneData.points[i].hidden) {
            visiblePointIndices.push_back(static_cast<int>(i));
            visiblePointNames.push_back(sceneData.points[i].name.c_str());
        }
    }

    auto getOriginalIndex = [&](int selected) -> int {
        if (selected >= 0 && selected < static_cast<int>(visiblePointIndices.size()))
            return visiblePointIndices[selected];
        return -1;
    };

    int selectedIdx1 = -1, selectedIdx2 = -1;
    for (size_t i = 0; i < visiblePointIndices.size(); ++i) {
        if (visiblePointIndices[i] == selectedPoint1) selectedIdx1 = static_cast<int>(i);
        if (visiblePointIndices[i] == selectedPoint2) selectedIdx2 = static_cast<int>(i);
    }

    ImGui::Combo((SetText("multi_point", currentLanguage) + " 1").c_str(), &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
    ImGui::Combo((SetText("multi_point", currentLanguage) + " 2").c_str(), &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

    selectedPoint1 = getOriginalIndex(selectedIdx1);
    selectedPoint2 = getOriginalIndex(selectedIdx2);

    ImGui::DragFloat((SetText("tabs_thickness", currentLanguage) + "##Thickness").c_str(), &sceneData.settings.lineThickness, 0.1f, 0.1f, 100.0f);

    if (ImGui::Button(SetText("tabs_add_line", currentLanguage).c_str())) {
        // Check that both points are valid and not the same
        if (selectedPoint1 != selectedPoint2 &&
            selectedPoint1 >= 0 && selectedPoint2 >= 0 &&
            selectedPoint1 < static_cast<int>(sceneData.points.size()) &&
            selectedPoint2 < static_cast<int>(sceneData.points.size())) {

            std::string name = lineName;
            name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
            name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

            if (name.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
            } else if (std::any_of(sceneData.lines.begin(), sceneData.lines.end(),
                [&](const Line& l) { return l.name == name; })) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
            } else if (sceneData.points[selectedPoint1].coords == sceneData.points[selectedPoint2].coords) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Points are the same");
            } else {
                sceneData.lines.push_back({ lineName, selectedPoint1, selectedPoint2 });
                lineName[0] = '\0';
            }
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox(SetText("tabs_cuts", currentLanguage).c_str(), &sceneData.settings.showCutLines);
    renderer.SetCutLineVisible(sceneData.settings.showCutLines);

    ImGui::Separator();
    // Draw line list with better styling and selection
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    static int selectedLineIndex = -1; // Track selected line index
    if (ImGui::BeginTable("LineTable", 5, 
        ImGuiTableFlags_NoHostExtendX | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_SizingStretchSame)) {
        
        // Set column widths - first column stretches, others fixed
        ImGui::TableSetupColumn(" Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Hide Points", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Visibility**", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        // Move these static maps outside the loop so they persist across frames
        static std::unordered_map<size_t, glm::vec3> initialMidMap;
        static std::unordered_map<size_t, std::pair<glm::vec3, glm::vec3>> initialPMap;

        for (size_t i = 0; i < sceneData.lines.size(); ++i) {
            auto& line = sceneData.lines[i];
            ImVec4 color(line.color[0], line.color[1], line.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();

            // Highlight entire row if selected
            if (static_cast<int>(i) == selectedLineIndex) {
                ImU32 highlightColor = ImGui::GetColorU32(ImGuiCol_Header);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, highlightColor);

                if (line.point1index >= 0 && line.point1index < static_cast<int>(sceneData.points.size()) &&
                    line.point2index >= 0 && line.point2index < static_cast<int>(sceneData.points.size())) {
                    auto& p1 = sceneData.points[line.point1index];
                    auto& p2 = sceneData.points[line.point2index];

                    glm::vec3 mid{
                        (p1.coords[0] + p2.coords[0]) * 0.5f,
                        (p1.coords[1] + p2.coords[1]) * 0.5f,
                        (p1.coords[2] + p2.coords[2]) * 0.5f
                    };

                    // Only set initial values if just selected (not every frame)
                    if (initialMidMap.find(i) == initialMidMap.end()) {
                        renderer.SetInitialGuizmoPosition(mid);
                        initialMidMap[i] = mid;
                        initialPMap[i] = { glm::vec3(p1.coords[0], p1.coords[1], p1.coords[2]),
                                           glm::vec3(p2.coords[0], p2.coords[1], p2.coords[2]) };
                    }

                    glm::vec3 newMid = renderer.SetPositionWithGuizmo(camera);

                    // Only update if guizmo moved
                    if (newMid != initialMidMap[i]) {
                        glm::vec3 delta = newMid - initialMidMap[i];
                        glm::vec3 p1new = initialPMap[i].first + delta;
                        glm::vec3 p2new = initialPMap[i].second + delta;
                        p1.coords[0] = p1new.x;
                        p1.coords[1] = p1new.y;
                        p1.coords[2] = p1new.z;
                        p2.coords[0] = p2new.x;
                        p2.coords[1] = p2new.y;
                        p2.coords[2] = p2new.z;
                    }
                }
            } else {
                // Clear guizmo state if not selected
                initialMidMap.erase(i);
                initialPMap.erase(i);
            }

            ImGui::TableSetColumnIndex(0);
            ImVec2 cellMin = ImGui::GetCursorScreenPos();
            ImVec2 cellMax = ImVec2(cellMin.x + ImGui::GetColumnWidth(), cellMin.y + ImGui::GetTextLineHeightWithSpacing());
            ImVec2 buttonSize = ImVec2(cellMax.x - cellMin.x, cellMax.y - cellMin.y);

            // Click area (invisible button)
            if (ImGui::InvisibleButton("##select", buttonSize)) {
                if (selectedLineIndex == static_cast<int>(i)) {
                    selectedLineIndex = -1; // Unselect
                } else {
                    selectedLineIndex = static_cast<int>(i); // Select
                    // Set guizmo to midpoint and only set initial maps if not already present
                    if (line.point1index >= 0 && line.point1index < static_cast<int>(sceneData.points.size()) &&
                        line.point2index >= 0 && line.point2index < static_cast<int>(sceneData.points.size())) {
                        auto& p1 = sceneData.points[line.point1index];
                        auto& p2 = sceneData.points[line.point2index];
                        glm::vec3 mid{
                            (p1.coords[0] + p2.coords[0]) * 0.5f,
                            (p1.coords[1] + p2.coords[1]) * 0.5f,
                            (p1.coords[2] + p2.coords[2]) * 0.5f
                        };
                        renderer.SetInitialGuizmoPosition(mid);
                        if (initialMidMap.find(i) == initialMidMap.end()) {
                            initialMidMap[i] = mid;
                            initialPMap[i] = { glm::vec3(p1.coords[0], p1.coords[1], p1.coords[2]),
                                               glm::vec3(p2.coords[0], p2.coords[1], p2.coords[2]) };
                        }
                    }
                }
            }

            // Draw the visible name text
            ImGui::SetCursorScreenPos(cellMin);
            ImGui::Text("%s", line.name.c_str());

            ImGui::TableSetColumnIndex(1);
            if (line.point1index >= 0 && line.point1index < static_cast<int>(sceneData.points.size()) &&
                line.point2index >= 0 && line.point2index < static_cast<int>(sceneData.points.size())) {
                bool pointsHidden = sceneData.points[line.point1index].hidden;
                if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                    sceneData.points[line.point1index].hidden = pointsHidden;
                    sceneData.points[line.point2index].hidden = pointsHidden;
                }
                ImGui::SameLine();
                ImGui::Text("Hide");
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Checkbox("##Visibility", &line.showVisibility);
            ImGui::SameLine();
            ImGui::Text("Visibility Study");

            ImGui::TableSetColumnIndex(3);
            if (ImGui::ColorEdit3("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                line.color[0] = color.x;
                line.color[1] = color.y;
                line.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("X", ImVec2(-FLT_MIN, 0))) {
                int p1 = line.point1index;
                int p2 = line.point2index;

                bool p1User = (p1 >= 0 && p1 < static_cast<int>(sceneData.points.size())) ?
                    sceneData.points[p1].userCreated : false;
                bool p2User = (p2 >= 0 && p2 < static_cast<int>(sceneData.points.size())) ?
                    sceneData.points[p2].userCreated : false;

                sceneData.lines.erase(sceneData.lines.begin() + i);

                if (p1 >= 0 && p1 < static_cast<int>(sceneData.points.size())) {
                    if (!p1User) {
                        sceneData.points[p1].hidden = true;
                        sceneData.points[p1].name = "deleted";
                    } else {
                        sceneData.points[p1].hidden = false;
                    }
                }
                if (p2 >= 0 && p2 < static_cast<int>(sceneData.points.size())) {
                    if (!p2User) {
                        sceneData.points[p2].hidden = true;
                        sceneData.points[p2].name = "deleted";
                    } else {
                        sceneData.points[p2].hidden = false;
                    }
                }
                // Clean up guizmo state for this line
                static std::unordered_map<size_t, glm::vec3> initialMidMap;
                static std::unordered_map<size_t, std::pair<glm::vec3, glm::vec3>> initialPMap;
                initialMidMap.erase(i);
                initialPMap.erase(i);

                --i; // Re-adjust index
                ImGui::PopID();
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // Show coordinate editing for selected line
    if (selectedLineIndex >= 0 && selectedLineIndex < static_cast<int>(sceneData.lines.size())) {
        auto& line = sceneData.lines[selectedLineIndex];
        if (line.point1index >= 0 && line.point1index < static_cast<int>(sceneData.points.size()) &&
            line.point2index >= 0 && line.point2index < static_cast<int>(sceneData.points.size())) {
            auto& p1 = sceneData.points[line.point1index];
            auto& p2 = sceneData.points[line.point2index];

            ImGui::InputFloat3("P1", p1.coords);
            ImGui::InputFloat3("P2", p2.coords);
        }
    }


    ImGui::PopStyleVar(); 
}

void UI::DrawPlanesTab(App& app) {
    auto& sceneData = app.GetSceneData();
    
    static char planeName[128] = "";
    ImGui::InputText(SetText("multi_name", currentLanguage).c_str(), planeName, sizeof(planeName));
    ImGui::DragFloat(SetText("tabs_opacity", currentLanguage).c_str(), &sceneData.settings.planeOpacity, 0.01f, 0.1f, 1.0f);

    // tabs to create point with coords or select existing points
    if (ImGui::BeginTabBar("PlaneTabs")) {
        if (ImGui::BeginTabItem(SetText("tabs_select_points", currentLanguage).c_str())) {
            static int selectedPoint1 = -1;
            static int selectedPoint2 = -1;
            static int selectedPoint3 = -1;

            // filtered list of visible points
            std::vector<int> visiblePointIndices;
            std::vector<const char*> visiblePointNames;
            for (size_t i = 0; i < sceneData.points.size(); ++i) {
                if (!sceneData.points[i].hidden) {
                    visiblePointIndices.push_back(static_cast<int>(i));
                    visiblePointNames.push_back(sceneData.points[i].name.c_str());
                }
            }

            auto getOriginalIndex = [&](int selected) -> int {
                if (selected >= 0 && selected < static_cast<int>(visiblePointIndices.size()))
                    return visiblePointIndices[selected];
                return -1;
            };

            int selectedIdx1 = -1, selectedIdx2 = -1, selectedIdx3 = -1;
            for (size_t i = 0; i < visiblePointIndices.size(); ++i) {
                if (visiblePointIndices[i] == selectedPoint1) selectedIdx1 = static_cast<int>(i);
                if (visiblePointIndices[i] == selectedPoint2) selectedIdx2 = static_cast<int>(i);
                if (visiblePointIndices[i] == selectedPoint3) selectedIdx3 = static_cast<int>(i);
            }

            ImGui::Combo((SetText("multi_point", currentLanguage) + " 1").c_str(), &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo((SetText("multi_point", currentLanguage) + " 2").c_str(), &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo((SetText("multi_point", currentLanguage) + " 3").c_str(), &selectedIdx3, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

            selectedPoint1 = getOriginalIndex(selectedIdx1);
            selectedPoint2 = getOriginalIndex(selectedIdx2);
            selectedPoint3 = getOriginalIndex(selectedIdx3);

            if (ImGui::Button("Add Plane")) {
                if (selectedPoint1 != selectedPoint2 && selectedPoint1 != selectedPoint3 && selectedPoint2 != selectedPoint3) {
                    std::string name = planeName;
                    name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                    name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                    if (name.empty()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                    } else if (std::any_of(sceneData.planes.begin(), sceneData.planes.end(),
                        [&](const Plane& p) { return p.name == name; })) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                    } else {
                        sceneData.planes.push_back({ planeName, selectedPoint1, selectedPoint2, selectedPoint3 });
                        planeName[0] = '\0';
                    }
                }
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(SetText("tabs_add_coords", currentLanguage).c_str())) {
            static float planeCoords[3] = {0.0f, 0.0f, 0.0f};
            ImGui::InputFloat3(SetText("tabs_coords", currentLanguage).c_str(), planeCoords);

            if (ImGui::Button(SetText("tabs_add_plane", currentLanguage).c_str())) {
                std::string name = planeName;
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                if (name.empty()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                } else if (std::any_of(sceneData.planes.begin(), sceneData.planes.end(),
                    [&](const Plane& p) { return p.name == name; })) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
                } else {
                    // we add the 3 points
                    sceneData.points.push_back({ name + "1", { planeCoords[0], 0.0f, 0.0f }, true });
                    sceneData.points.push_back({ name + "2", { 0.0f, planeCoords[1], 0.0f }, true });
                    sceneData.points.push_back({ name + "3", { 0.0f, 0.0f, planeCoords[2] }, true });

                    sceneData.planes.push_back({ name, static_cast<int>(sceneData.points.size() - 3), 
                                                static_cast<int>(sceneData.points.size() - 2), 
                                                static_cast<int>(sceneData.points.size() - 1) });
                    planeName[0] = '\0';
                }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    static int selectedPlaneIndex = -1;
    // For guizmo-like movement: store initial midpoint and initial positions for each plane
    static std::unordered_map<size_t, glm::vec3> initialMidMap;
    static std::unordered_map<size_t, std::tuple<glm::vec3, glm::vec3, glm::vec3>> initialPMap;

    if (ImGui::BeginTable("PlaneTable", 5,
        ImGuiTableFlags_NoHostExtendX |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchSame)) {

        ImGui::TableSetupColumn(" Name", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("Hide Points", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Expand", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sceneData.planes.size(); ++i) {
            auto& plane = sceneData.planes[i];
            ImVec4 color(plane.color[0], plane.color[1], plane.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();

            // Highlight row if selected
            if (static_cast<int>(i) == selectedPlaneIndex) {
                ImU32 highlightColor = ImGui::GetColorU32(ImGuiCol_Header);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, highlightColor);

                if (plane.point1index >= 0 && plane.point1index < static_cast<int>(sceneData.points.size()) &&
                    plane.point2index >= 0 && plane.point2index < static_cast<int>(sceneData.points.size()) &&
                    plane.point3index >= 0 && plane.point3index < static_cast<int>(sceneData.points.size())) {
                    auto& p1 = sceneData.points[plane.point1index];
                    auto& p2 = sceneData.points[plane.point2index];
                    auto& p3 = sceneData.points[plane.point3index];

                    glm::vec3 mid{
                        (p1.coords[0] + p2.coords[0] + p3.coords[0]) / 3.0f,
                        (p1.coords[1] + p2.coords[1] + p3.coords[1]) / 3.0f,
                        (p1.coords[2] + p2.coords[2] + p3.coords[2]) / 3.0f
                    };

                    // Set initial guizmo position if just selected
                    if (initialMidMap.find(i) == initialMidMap.end()) {
                        // Set guizmo to midpoint
                        app.GetRenderer().SetInitialGuizmoPosition(mid);
                        initialMidMap[i] = mid;
                        initialPMap[i] = std::make_tuple(
                            glm::vec3(p1.coords[0], p1.coords[1], p1.coords[2]),
                            glm::vec3(p2.coords[0], p2.coords[1], p2.coords[2]),
                            glm::vec3(p3.coords[0], p3.coords[1], p3.coords[2])
                        );
                    }

                    glm::vec3 newMid = app.GetRenderer().SetPositionWithGuizmo(app.GetCamera());
                    if (newMid != initialMidMap[i]) {
                        glm::vec3 delta = newMid - initialMidMap[i];
                        auto [p1init, p2init, p3init] = initialPMap[i];
                        glm::vec3 p1new = p1init + delta;
                        glm::vec3 p2new = p2init + delta;
                        glm::vec3 p3new = p3init + delta;
                        p1.coords[0] = p1new.x; p1.coords[1] = p1new.y; p1.coords[2] = p1new.z;
                        p2.coords[0] = p2new.x; p2.coords[1] = p2new.y; p2.coords[2] = p2new.z;
                        p3.coords[0] = p3new.x; p3.coords[1] = p3new.y; p3.coords[2] = p3new.z;
                    }
                }
            } else {
                // Clear guizmo state if not selected
                initialMidMap.erase(i);
                initialPMap.erase(i);
            }

            ImGui::TableSetColumnIndex(0);
            ImVec2 cellMin = ImGui::GetCursorScreenPos();
            ImVec2 cellMax = ImVec2(cellMin.x + ImGui::GetColumnWidth(), cellMin.y + ImGui::GetTextLineHeightWithSpacing());
            ImVec2 buttonSize = ImVec2(cellMax.x - cellMin.x, cellMax.y - cellMin.y);

            // Click area (invisible button)
            if (ImGui::InvisibleButton("##select", buttonSize)) {
                if (selectedPlaneIndex == static_cast<int>(i)) {
                    selectedPlaneIndex = -1;
                } else {
                    selectedPlaneIndex = static_cast<int>(i);
                    // Set guizmo to midpoint
                    if (plane.point1index >= 0 && plane.point1index < static_cast<int>(sceneData.points.size()) &&
                        plane.point2index >= 0 && plane.point2index < static_cast<int>(sceneData.points.size()) &&
                        plane.point3index >= 0 && plane.point3index < static_cast<int>(sceneData.points.size())) {
                        auto& p1 = sceneData.points[plane.point1index];
                        auto& p2 = sceneData.points[plane.point2index];
                        auto& p3 = sceneData.points[plane.point3index];
                        glm::vec3 mid{
                            (p1.coords[0] + p2.coords[0] + p3.coords[0]) / 3.0f,
                            (p1.coords[1] + p2.coords[1] + p3.coords[1]) / 3.0f,
                            (p1.coords[2] + p2.coords[2] + p3.coords[2]) / 3.0f
                        };
                        app.GetRenderer().SetInitialGuizmoPosition(mid);
                        initialMidMap[i] = mid;
                        initialPMap[i] = std::make_tuple(
                            glm::vec3(p1.coords[0], p1.coords[1], p1.coords[2]),
                            glm::vec3(p2.coords[0], p2.coords[1], p2.coords[2]),
                            glm::vec3(p3.coords[0], p3.coords[1], p3.coords[2])
                        );
                    }
                }
            }
            ImGui::SetCursorScreenPos(cellMin);
            ImGui::Text("  %s", plane.name.c_str());

            ImGui::TableSetColumnIndex(1);
            bool pointsHidden = false;
            if (plane.point1index >= 0 && plane.point1index < static_cast<int>(sceneData.points.size()) &&
                plane.point2index >= 0 && plane.point2index < static_cast<int>(sceneData.points.size()) &&
                plane.point3index >= 0 && plane.point3index < static_cast<int>(sceneData.points.size())) {
                pointsHidden = sceneData.points[plane.point1index].hidden;
                if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                    sceneData.points[plane.point1index].hidden = pointsHidden;
                    sceneData.points[plane.point2index].hidden = pointsHidden;
                    sceneData.points[plane.point3index].hidden = pointsHidden;
                }
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Checkbox("##Expand", &plane.expand);

            ImGui::TableSetColumnIndex(3);
            if (ImGui::ColorEdit3("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                plane.color[0] = color.x;
                plane.color[1] = color.y;
                plane.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("X")) {
                int p1 = plane.point1index;
                int p2 = plane.point2index;
                int p3 = plane.point3index;

                bool p1User = (p1 >= 0 && p1 < static_cast<int>(sceneData.points.size())) ?
                    sceneData.points[p1].userCreated : false;
                bool p2User = (p2 >= 0 && p2 < static_cast<int>(sceneData.points.size())) ?
                    sceneData.points[p2].userCreated : false;
                bool p3User = (p3 >= 0 && p3 < static_cast<int>(sceneData.points.size())) ?
                    sceneData.points[p3].userCreated : false;
                sceneData.planes.erase(sceneData.planes.begin() + i);
                if (p1 >= 0 && p1 < static_cast<int>(sceneData.points.size())) {
                    if (!p1User) {
                        sceneData.points[p1].hidden = true;
                        sceneData.points[p1].name = "deleted";
                    } else {
                        sceneData.points[p1].hidden = false;
                    }
                }
                if (p2 >= 0 && p2 < static_cast<int>(sceneData.points.size())) {
                    if (!p2User) {
                        sceneData.points[p2].hidden = true;
                        sceneData.points[p2].name = "deleted";
                    } else {
                        sceneData.points[p2].hidden = false;
                    }
                }
                if (p3 >= 0 && p3 < static_cast<int>(sceneData.points.size())) {
                    if (!p3User) {
                        sceneData.points[p3].hidden = true;
                        sceneData.points[p3].name = "deleted";
                    } else {
                        sceneData.points[p3].hidden = false;
                    }
                }
                initialMidMap.erase(i);
                initialPMap.erase(i);
                ImGui::PopID();
                --i;
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // Show coordinate editing for selected plane
    if (selectedPlaneIndex >= 0 && selectedPlaneIndex < static_cast<int>(sceneData.planes.size())) {
        auto& plane = sceneData.planes[selectedPlaneIndex];
        if (plane.point1index >= 0 && plane.point1index < static_cast<int>(sceneData.points.size()) &&
            plane.point2index >= 0 && plane.point2index < static_cast<int>(sceneData.points.size()) &&
            plane.point3index >= 0 && plane.point3index < static_cast<int>(sceneData.points.size())) {
            auto& p1 = sceneData.points[plane.point1index];
            auto& p2 = sceneData.points[plane.point2index];
            auto& p3 = sceneData.points[plane.point3index];

            ImGui::Separator();
            ImGui::InputFloat3("P1", p1.coords);
            ImGui::InputFloat3("P2", p2.coords);
            ImGui::InputFloat3("P3", p3.coords);
        }
    }
    ImGui::PopStyleVar();
}

void UI::DrawPresetWindow(App& app) {
    auto& sceneData = app.GetSceneData();

    int width = app.GetWindowWidth();
    int height = app.GetWindowHeight();

    auto& jsonHandler = app.GetJsonHandler();
    std::string presetsPath = "./assets/presets.json";
    nlohmann::json jsonContent = jsonHandler.LoadPresets(presetsPath);

    ImGui::SetNextWindowPos(ImVec2(60, height - 30 - 150), ImGuiCond_FirstUseEver); 
    ImGui::Begin(SetText("presets_title", currentLanguage).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text(SetText("presets_message", currentLanguage).c_str());

    if (jsonContent.is_null()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load presets!");
        ImGui::End();
        return;
    }

    if (ImGui::Button(SetText("multi_points", currentLanguage).c_str())) {
        ImGui::OpenPopup("Points Presets");
    }
    
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Points Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(SetText("presets_points_available", currentLanguage).c_str());
        ImGui::Separator();
        
        auto pointPresets = jsonHandler.GetPointPresets(jsonContent);
        if (pointPresets.empty()) {
            ImGui::Text(SetText("presets_no_points_found", currentLanguage).c_str());
        } else {
            for (const auto& preset : pointPresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    sceneData.points.push_back({
                        preset["name"],
                        {
                            preset["coords"]["d"].get<float>(),
                            preset["coords"]["a"].get<float>(),
                            preset["coords"]["c"].get<float>()
                        }
                    });
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button(SetText("multi_close", currentLanguage).c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(SetText("multi_lines", currentLanguage).c_str())) {
        ImGui::OpenPopup("Lines Presets");
    }
    
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Lines Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(SetText("presets_lines_available", currentLanguage).c_str());
        ImGui::Separator();
        
        auto linePresets = jsonHandler.GetLinePresets(jsonContent);
        if (linePresets.empty()) {
            ImGui::Text(SetText("presets_no_lines_found", currentLanguage).c_str());
        } else {
            for (const auto& preset : linePresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    sceneData.lines.push_back({
                        preset["name"],
                        static_cast<int>(sceneData.points.size()),
                        static_cast<int>(sceneData.points.size() + 1)
                    });
                    
                    sceneData.points.push_back({
                        ("A_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point1"]["d"].get<float>(),
                            preset["point1"]["a"].get<float>(),
                            preset["point1"]["c"].get<float>()
                        },
                        true
                    });
                    sceneData.points.push_back({
                        ("B_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point2"]["d"].get<float>(),
                            preset["point2"]["a"].get<float>(),
                            preset["point2"]["c"].get<float>()
                        },
                        true
                    });
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button(SetText("multi_close", currentLanguage).c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::Button(SetText("multi_planes", currentLanguage).c_str())) {
        ImGui::OpenPopup("Plane Presets");
    }
    
    if (ImGui::BeginPopupModal("Plane Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(SetText("presets_planes_available", currentLanguage).c_str());
        ImGui::Separator();
        
        auto planePresets = jsonHandler.GetPlanePresets(jsonContent);
        if (planePresets.empty()) {
            ImGui::Text(SetText("presets_no_planes_found", currentLanguage).c_str());
        } else {
            for (const auto& preset : planePresets) {
                std::string label = preset["name"].get<std::string>() + " - " + preset["description"].get<std::string>();
                if (ImGui::Button(label.c_str())) {
                    sceneData.planes.push_back({
                        preset["name"],
                        static_cast<int>(sceneData.points.size()),
                        static_cast<int>(sceneData.points.size() + 1),
                        static_cast<int>(sceneData.points.size() + 2)
                    });
                    
                    sceneData.points.push_back({
                        ("A_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point1"]["d"].get<float>(),
                            preset["point1"]["a"].get<float>(),
                            preset["point1"]["c"].get<float>()
                        },
                        true // hidden
                    });
                    sceneData.points.push_back({
                        ("B_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point2"]["d"].get<float>(),
                            preset["point2"]["a"].get<float>(),
                            preset["point2"]["c"].get<float>()
                        },
                        true 
                    });
                    sceneData.points.push_back({
                        ("C_" + preset["name"].get<std::string>()).c_str(),
                        {
                            preset["point3"]["d"].get<float>(),
                            preset["point3"]["a"].get<float>(),
                            preset["point3"]["c"].get<float>()
                        },
                        true 
                    });

                    sceneData.planes.back().expand = preset["expand"].get<bool>();

                    // check if there is a specific color
                    if (preset.contains("color")) {
                        sceneData.planes.back().color[0] = preset["color"]["r"].get<float>();
                        sceneData.planes.back().color[1] = preset["color"]["g"].get<float>();
                        sceneData.planes.back().color[2] = preset["color"]["b"].get<float>();
                    }

                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button(SetText("multi_close", currentLanguage).c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}