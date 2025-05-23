#include "ui.h"
#include "app.h"

#include <algorithm>
#include <string>

#include "style.h"

#define PROGRAM_VERSION "0.7.2"

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
    const char* fontPath = "/assets/Roboto-Regular.ttf"; 
    const char* initPath = "/assets/imgui.ini";
#else
    const char* fontPath = "./assets/Roboto-Regular.ttf"; 
    const char* initPath = "./assets/imgui.ini";
#endif
    io.Fonts->AddFontFromFileTTF(fontPath, sceneData.settings.fontSize);
    ImGui::GetIO().IniFilename = initPath;

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 100");
#endif
}

void UI::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::DrawUI(App& app) {
    DrawMenuBar(app);
    DrawSettingsWindow(app);
    DrawPresetWindow(app);
    DrawTabsWindow(app);
    DrawDihedralViewport(app);
}

void UI::DrawMenuBar(App& app) {
    auto& sceneData = app.GetSceneData();
    int width = app.GetWindowWidth();
    int height = app.GetWindowHeight();
    
#ifndef __EMSCRIPTEN__
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Save*", nullptr, false);
            ImGui::MenuItem("Save As*", nullptr, false);
            ImGui::MenuItem("Load*", nullptr, false);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("App")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(app.GetWindow(), true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("About")) {
            ImGui::Text("Version %s", PROGRAM_VERSION);
            ImGui::Text("Made by Alonso Martínez");
            ImGui::Text("@almartdev on GitHub");
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#else
    if (sceneData.settings.showWelcomeWindow) {
        // it has to be exacly in the middle of the screen
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
        
        if (ImGui::Begin("Welcome", &sceneData.settings.showWelcomeWindow, ImGuiWindowFlags_NoMove 
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("Welcome to the Dihedral System App!");
            ImGui::Text("IMPORTANT: This app is still in development.");
            ImGui::Separator();
            ImGui::Text("Version %s - WEB", PROGRAM_VERSION);
            ImGui::Text("Made by Alonso Martínez");
            ImGui::Text("@almartdev on GitHub");
        }
        ImGui::End();
    }
#endif
}

void UI::DrawSettingsWindow(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& camera = app.GetCamera();
    auto& renderer = app.GetRenderer();

    ImGui::Begin("3D Representation Settings");

    ImGui::ColorEdit3("Background Color", sceneData.settings.backgroundColor);
    ImGui::ColorEdit3("Dihedral Bg Color", sceneData.settings.dihedralBackgroundColor);
    ImGui::ColorEdit3("Dihedral Line Color", sceneData.settings.dihedralLineColor);

    const char* axesTypes[] = {"3D Axes", "Cartesian Axes", "Dihedral Axes ONLY"};
    ImGui::Combo("Axes Type", &sceneData.settings.axesType, axesTypes, IM_ARRAYSIZE(axesTypes));
    renderer.SetAxesType(sceneData.settings.axesType);

    if (sceneData.settings.axesType != 2) {
        ImGui::Checkbox("Show Dihedral System", &sceneData.settings.showDihedralSystem);
        renderer.SetDihedralsVisible(sceneData.settings.showDihedralSystem);
    }
    
    ImGui::SliderFloat("Mouse Sensitivity", &sceneData.settings.mouseSensitivity, 0.0f, 2.0f);
    camera.SetSensitivity(sceneData.settings.mouseSensitivity); 
    
    ImGui::SliderFloat("Camera Distance", &sceneData.settings.cameraDistance, 0.1f, 25.0f);
    camera.SetDistance(sceneData.settings.cameraDistance);

    ImGui::DragFloat2("Offset (X, Y)", sceneData.settings.offset, 0.3f);

    ImGui::Checkbox("Enable VSync", &sceneData.settings.VSync);

    /*ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", 
                camera.GetPosition().x, 
                camera.GetPosition().y, 
                camera.GetPosition().z);*/

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}

void UI::DrawPointsTab(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& renderer = app.GetRenderer();
    auto& camera = app.GetCamera();

    static char pointName[128] = "";
    static float pointCoords[3] = {0.0f, 0.0f, 0.0f};

    ImGui::InputText("Name", pointName, sizeof(pointName));
    ImGui::InputFloat3("Coordinates", pointCoords);

    if (ImGui::Button("Add Point")) {
        std::string name = pointName;
        name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
        name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

        if (name.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
        } 
        else if (std::any_of(sceneData.points.begin(), sceneData.points.end(), 
                            [&](const SceneData::Point& p) { return p.name == name; })) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name already exists");
        } 
        else {
            sceneData.points.push_back({name, {pointCoords[0], pointCoords[1], pointCoords[2]}});
            pointName[0] = '\0';
            memset(pointCoords, 0, sizeof(pointCoords));
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show Cut Points", &sceneData.settings.showCutPoints);
    renderer.SetCutPointVisible(sceneData.settings.showCutPoints);

    ImGui::DragFloat("Point Size", &sceneData.settings.pointSize, 0.1f, 0.1f, 100.0f);

    ImGui::Separator();
    
    // Draw point list with better styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("PointTable", 4, ImGuiTableFlags_NoHostExtendX                
                                         | ImGuiTableFlags_RowBg  
                                         | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn(" Name");
        ImGui::TableSetupColumn("Coords");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sceneData.points.size(); ++i) {
            if (sceneData.points[i].hidden) continue;

            auto& point = sceneData.points[i];
            ImVec4 color(point.color[0], point.color[1], point.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", point.name.c_str(), point.name.empty() ? '?' : point.name[0]);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(static_cast<int>(i));
            // Make the DragFloat3 as wide as possible in the cell
            float cellWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(cellWidth);
            ImGui::DragFloat3("", point.coords, 0.1f);
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

void UI::DrawTabsWindow(App& app) {
    ImGui::Begin("Properties");

    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Points")) {
            DrawPointsTab(app);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lines")) {
            DrawLinesTab(app);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Planes")) {
            DrawPlanesTab(app);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void UI::DrawLinesTab(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& renderer = app.GetRenderer();
    auto& camera = app.GetCamera();

    static char lineName[128] = "";
    ImGui::InputText("Name", lineName, sizeof(lineName));

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

    ImGui::Combo("Point 1", &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
    ImGui::Combo("Point 2", &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

    selectedPoint1 = getOriginalIndex(selectedIdx1);
    selectedPoint2 = getOriginalIndex(selectedIdx2);

    ImGui::DragFloat("Line Thickness", &sceneData.settings.lineThickness, 0.1f, 0.1f, 100.0f);

    if (ImGui::Button("Add Line")) {
        if (selectedPoint1 != selectedPoint2) {
            std::string name = lineName;
            name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
            name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

            if (name.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
            } else if (std::any_of(sceneData.points.begin(), sceneData.points.end(),
                [&](const SceneData::Point& p) { return p.name == name; })) {
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
    ImGui::Checkbox("Show Cut Lines", &sceneData.settings.showCutLines);
    renderer.SetCutLineVisible(sceneData.settings.showCutLines);

    ImGui::Separator();

    // Draw line list with better styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("LineTable", 5, ImGuiTableFlags_NoHostExtendX                
                                        | ImGuiTableFlags_RowBg  
                                        | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Hide Points");
        ImGui::TableSetupColumn("TEST:Visibility*");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sceneData.lines.size(); ++i) {
            auto& line = sceneData.lines[i];
            ImVec4 color(line.color[0], line.color[1], line.color[2], 1.0f);

            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", line.name.c_str(), line.name.empty() ? '?' : line.name[0]);

            /*ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s --> %s",
                sceneData.points[line.point1index].name.c_str(),
                sceneData.points[line.point2index].name.c_str());
            */

            ImGui::TableSetColumnIndex(1);
            bool pointsHidden = sceneData.points[line.point1index].hidden && sceneData.settings.showCutPoints;
            if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                sceneData.points[line.point1index].hidden = pointsHidden;
                sceneData.points[line.point2index].hidden = pointsHidden;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Checkbox("##Visibility", &line.showVisibility);

            ImGui::TableSetColumnIndex(3);
            if (ImGui::ColorEdit4("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                line.color[0] = color.x;
                line.color[1] = color.y;
                line.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("X")) {
                sceneData.lines.erase(sceneData.lines.begin() + i);
                app.DeletePoint(sceneData.points[line.point1index]);
                app.DeletePoint(sceneData.points[line.point2index]);
                ImGui::PopID();
                --i; // Re-adjust index
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar(); 
}

void UI::DrawPlanesTab(App& app) {
    auto& sceneData = app.GetSceneData();
    auto& renderer = app.GetRenderer();
    auto& camera = app.GetCamera();

    static char planeName[128] = "";
    ImGui::InputText("Name", planeName, sizeof(planeName));

    ImGui::DragFloat("Plane Opacity", &sceneData.settings.planeOpacity, 0.01f, 0.1f, 1.0f);

    // tabs to creat point with coords or select existing points
    if (ImGui::BeginTabBar("PlaneTabs")) {
        if (ImGui::BeginTabItem("Select Points")) {
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

            ImGui::Combo("Point 1", &selectedIdx1, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo("Point 2", &selectedIdx2, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));
            ImGui::Combo("Point 3", &selectedIdx3, visiblePointNames.data(), static_cast<int>(visiblePointNames.size()));

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
                    } else if (std::any_of(sceneData.points.begin(), sceneData.points.end(),
                        [&](const SceneData::Point& p) { return p.name == name; })) {
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
        if (ImGui::BeginTabItem("Add Coords")) {
            static float planeCoords[3] = {0.0f, 0.0f, 0.0f};
            ImGui::InputFloat3("Coords", planeCoords);

            if (ImGui::Button("Add Plane")) {
                std::string name = planeName;
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));

                if (name.empty()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Name required");
                } else if (std::any_of(sceneData.points.begin(), sceneData.points.end(),
                    [&](const SceneData::Point& p) { return p.name == name; })) {
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
    
    //ImGui::Checkbox("Show Cuts*", &sceneData.settings.showCutPlanes);
    //renderer.SetCutPlaneVisible(sceneData.settings.showCutPlanes);

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
    if (ImGui::BeginTable("PlaneTable", 5, ImGuiTableFlags_NoHostExtendX                
                                        | ImGuiTableFlags_RowBg  
                                        | ImGuiTableFlags_Resizable )) {
        ImGui::TableSetupColumn("Name");
        //ImGui::TableSetupColumn("Points");
        ImGui::TableSetupColumn("Hide Points");
        ImGui::TableSetupColumn("Expand");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sceneData.planes.size(); ++i) {
            auto& plane = sceneData.planes[i];
            ImVec4 color(plane.color[0], plane.color[1], plane.color[2], 1.0f);
            ImGui::PushID(static_cast<int>(i)); // Unique ID scope for widgets

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s (%c)", plane.name.c_str(), plane.name.empty() ? '?' : plane.name[0]);

            /*ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s, %s, %s",
                sceneData.points[plane.point1index].name.c_str(),
                sceneData.points[plane.point2index].name.c_str(),
                sceneData.points[plane.point3index].name.c_str());*/

            ImGui::TableSetColumnIndex(1);
            bool pointsHidden = sceneData.points[plane.point1index].hidden && 
                                sceneData.points[plane.point2index].hidden && 
                                sceneData.points[plane.point3index].hidden && 
                                sceneData.settings.showCutPoints;

            if (ImGui::Checkbox("##Hide", &pointsHidden)) {
                sceneData.points[plane.point1index].hidden = pointsHidden;
                sceneData.points[plane.point2index].hidden = pointsHidden;
                sceneData.points[plane.point3index].hidden = pointsHidden;
            }

            ImGui::TableSetColumnIndex(2);
            if (ImGui::Checkbox("##Expand", &plane.expand)) {
                sceneData.planes[i].expand = plane.expand; 
            }

            ImGui::TableSetColumnIndex(3);
            if (ImGui::ColorEdit4("##Color", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                plane.color[0] = color.x;
                plane.color[1] = color.y;
                plane.color[2] = color.z;
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("X")) {
                sceneData.planes.erase(sceneData.planes.begin() + i);
                app.DeletePoint(sceneData.points[plane.point1index]);
                app.DeletePoint(sceneData.points[plane.point2index]);
                app.DeletePoint(sceneData.points[plane.point3index]);
                ImGui::PopID();
                --i; // Re-adjust index
                continue;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar(); 
}

void UI::DrawPresetWindow(App& app) {
    auto& sceneData = app.GetSceneData();

    int width = app.GetWindowWidth();
    int height = app.GetWindowHeight();

    auto& jsonHandler = app.GetJsonHandler();
    bool m_jsonLoaded = jsonHandler.LoadJson();

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 250, 450), ImGuiCond_Always);
    ImGui::Begin("Presets", nullptr);
    ImGui::Text("Select a preset to load");

    if (!m_jsonLoaded) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load presets!");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Points")) {
        ImGui::OpenPopup("Points Presets");
    }
    
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Points Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Point Presets:");
        ImGui::Separator();
        
        auto pointPresets = jsonHandler.GetPointPresets();
        if (pointPresets.empty()) {
            ImGui::Text("No point presets found");
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
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Lines")) {
        ImGui::OpenPopup("Lines Presets");
    }
    
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Lines Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Line Presets:");
        ImGui::Separator();
        
        auto linePresets = jsonHandler.GetLinePresets();
        if (linePresets.empty()) {
            ImGui::Text("No line presets found");
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
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, height / 2 - 75), ImGuiCond_Always);
    if (ImGui::Button("Planes")) {
        ImGui::OpenPopup("Plane Presets");
    }
    
    if (ImGui::BeginPopupModal("Plane Presets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Available Plane Presets:");
        ImGui::Separator();
        
        auto planePresets = jsonHandler.GetPlanePresets();
        if (planePresets.empty()) {
            ImGui::Text("No plane presets found");
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
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

// This is the "Dihedral engine" // we should refactor this, and a lot from this file too
void UI::DrawDihedralViewport(App& app) {
    auto& sceneData = app.GetSceneData();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(
        sceneData.settings.dihedralBackgroundColor[0], 
        sceneData.settings.dihedralBackgroundColor[1], 
        sceneData.settings.dihedralBackgroundColor[2], 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
    // set this window to start at the top right corner
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 550, 50), ImGuiCond_Always);
    ImGui::Begin("Dihedral Projection", nullptr, 
                ImGuiWindowFlags_NoCollapse | 
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoTitleBar);

    ImColor lineColor = IM_COL32(
        sceneData.settings.dihedralLineColor[0] * 255, 
        sceneData.settings.dihedralLineColor[1] * 255, 
        sceneData.settings.dihedralLineColor[2] * 255, 255);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Ground line (L.T.)
    ImVec2 p0 = ImVec2(cursorPos.x, cursorPos.y + viewportSize.y / 2);
    ImVec2 p1 = ImVec2(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y / 2);
    drawList->AddLine(p0, p1, lineColor, 2.5f); 

    // Small indicator lines
    drawList->AddLine(ImVec2(p0.x + 4, p0.y + 5), ImVec2(p0.x + 25, p0.y + 5), lineColor, 2.0f);
    drawList->AddLine(ImVec2(p1.x - 4, p1.y + 5), ImVec2(p1.x - 25, p1.y + 5), lineColor, 2.0f);

    // Origin point
    ImVec2 p2 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 - 8);
    ImVec2 p3 = ImVec2(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2 + 8);
    drawList->AddLine(p2, p3, lineColor, 2.5f);
    
    // Draw points in 2D projection
    for (const auto& point : sceneData.points) {
        if (point.hidden) continue;

        float x = point.coords[0] / 2.0f;
        float y1 = point.coords[2] / 3.0f;
        float y2 = -point.coords[1] / 3.0f;

        ImVec2 pos1(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);
        ImVec2 pos2(cursorPos.x + viewportSize.x / 2 + x * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);

        drawList->AddCircleFilled(pos1, sceneData.settings.pointSize / 2,
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        drawList->AddCircleFilled(pos2, sceneData.settings.pointSize / 2,
                                IM_COL32(point.color[0] * 255, point.color[1] * 255, point.color[2] * 255, 255));
        
        // Draw labels
        if (pos2.x - 20 == pos1.x - 20 && pos2.y - 20 == pos1.y - 20) {
            ImGui::SetCursorScreenPos(ImVec2(pos2.x - 20, pos2.y - 20));
            ImGui::TextColored(ImVec4(point.color[0], point.color[1], point.color[2], 1.0f), "%c1 = %c2", point.name[0], point.name[0]);
        }
        else {
            ImGui::SetCursorScreenPos(ImVec2(pos2.x - 20, pos2.y - 20));
            ImGui::TextColored(ImVec4(point.color[0], point.color[1], point.color[2], 1.0f), "%c1", point.name[0]);
            ImGui::SetCursorScreenPos(ImVec2(pos1.x - 20, pos1.y - 20));
            ImGui::TextColored(ImVec4(point.color[0], point.color[1], point.color[2], 1.0f), "%c2", point.name[0]);
        }

        ImVec2 ltPos(cursorPos.x + viewportSize.x / 2 + x * 10, cursorPos.y + viewportSize.y / 2);

        drawList->AddLine(pos1, ltPos, lineColor, .75f);
        drawList->AddLine(pos2, ltPos, lineColor, .75f);
    }
    
    // lines
    // TODO: fix visibility
    for (const auto& line : sceneData.lines) {
        const auto& p1 = sceneData.points[line.point1index];
        const auto& p2 = sceneData.points[line.point2index];

        float x1 = p1.coords[0] / 2.0f;
        float y1 = p1.coords[2] / 3.0f;
        float y2 = -p1.coords[1] / 3.0f;

        float x2 = p2.coords[0] / 2.0f;
        float y3 = p2.coords[2] / 3.0f;
        float y4 = -p2.coords[1] / 3.0f;

        // Function to draw a line with mixed solid/dashed segments based on visibility
        auto drawMixedLine = [&](ImVec2 p1, ImVec2 p2, bool isR2) {
            if (!line.showVisibility) {
                drawList->AddLine(p1, p2, lineColor, 1.0f);
                return;
            }

            const float dash_length = 5.0f;
            ImVec2 dir = ImVec2(p2.x - p1.x, p2.y - p1.y);
            float length = sqrtf(dir.x*dir.x + dir.y*dir.y);
            dir.x /= length;
            dir.y /= length;

            // Find all points where the line crosses between visible and non-visible regions
            std::vector<float> transition_points;
            transition_points.push_back(0.0f); // Start point

            // Sample along the line to find visibility transitions
            const int samples = 100;
            bool last_visible = false;
            for (int i = 0; i <= samples; i++) {
                float t = (float)i / samples;
                ImVec2 pt(p1.x + dir.x * t * length, p1.y + dir.y * t * length);
                
                // Convert back to original coordinates to check visibility
                float orig_x = (pt.x - (cursorPos.x + viewportSize.x / 2)) / 10.0f * 2.0f;
                float orig_y;
                if (isR2) {
                    orig_y = -((pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                } else {
                    orig_y = ((pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                }

                bool current_visible = (orig_x >= 0 && orig_y >= 0);
                
                if (i > 0 && current_visible != last_visible) {
                    // Find the exact transition point using binary search
                    float low = (float)(i-1)/samples;
                    float high = t;
                    for (int j = 0; j < 5; j++) { // 5 iterations should be enough
                        float mid = (low + high) / 2;
                        ImVec2 mid_pt(p1.x + dir.x * mid * length, p1.y + dir.y * mid * length);
                        float mid_orig_x = (mid_pt.x - (cursorPos.x + viewportSize.x / 2)) / 10.0f * 2.0f;
                        float mid_orig_y;
                        if (isR2) {
                            mid_orig_y = -((mid_pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                        } else {
                            mid_orig_y = ((mid_pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                        }
                        bool mid_visible = (mid_orig_x >= 0 && mid_orig_y >= 0);
                        
                        if (mid_visible == last_visible) {
                            low = mid;
                        } else {
                            high = mid;
                        }
                    }
                    transition_points.push_back((low + high)/2);
                }
                
                last_visible = current_visible;
            }

            transition_points.push_back(1.0f); // End point

            // Draw segments between transition points
            for (size_t i = 0; i < transition_points.size() - 1; i++) {
                float t1 = transition_points[i];
                float t2 = transition_points[i+1];
                
                ImVec2 seg_p1(p1.x + dir.x * t1 * length, p1.y + dir.y * t1 * length);
                ImVec2 seg_p2(p1.x + dir.x * t2 * length, p1.y + dir.y * t2 * length);
                
                // Check visibility at midpoint
                float mid_t = (t1 + t2) / 2;
                ImVec2 mid_pt(p1.x + dir.x * mid_t * length, p1.y + dir.y * mid_t * length);
                float mid_orig_x = (mid_pt.x - (cursorPos.x + viewportSize.x / 2)) / 10.0f * 2.0f;
                float mid_orig_y;
                if (isR2) {
                    mid_orig_y = -((mid_pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                } else {
                    mid_orig_y = ((mid_pt.y - (cursorPos.y + viewportSize.y / 2)) / 10.0f * 3.0f);
                }
                bool is_visible = (mid_orig_x >= 0 && mid_orig_y >= 0);
                
                if (is_visible) {
                    drawList->AddLine(seg_p1, seg_p2, lineColor, 1.0f);
                } else {
                    // Draw dashed line for non-visible segments
                    float seg_length = sqrtf(pow(seg_p2.x - seg_p1.x, 2) + pow(seg_p2.y - seg_p1.y, 2));
                    ImVec2 seg_dir((seg_p2.x - seg_p1.x)/seg_length, (seg_p2.y - seg_p1.y)/seg_length);
                    
                    for (float t = 0; t < seg_length; t += dash_length * 2) {
                        ImVec2 start(seg_p1.x + seg_dir.x * t, seg_p1.y + seg_dir.y * t);
                        ImVec2 end(seg_p1.x + seg_dir.x * (t + dash_length), seg_p1.y + seg_dir.y * (t + dash_length));
                        if (t + dash_length > seg_length) end = seg_p2;
                        drawList->AddLine(start, end, lineColor, 1.0f);
                    }
                }
            }
        };

        // r2 line (vertical plane)
        ImVec2 p1_r2(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y1 * 10);
        ImVec2 p2_r2(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y3 * 10);
        
        float r2_minX = cursorPos.x;
        float r2_maxX = cursorPos.x + viewportSize.x;
        float r2_minY = cursorPos.y;
        float r2_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r2, edge2_r2;
        
        // Handle vertical lines (x1 == x2)
        if (abs(p2_r2.x - p1_r2.x) < 0.0001f) {
            edge1_r2 = ImVec2(p1_r2.x, r2_minY);
            edge2_r2 = ImVec2(p1_r2.x, r2_maxY);
        }
        // Handle horizontal lines (y1 == y2)
        else if (abs(p2_r2.y - p1_r2.y) < 0.0001f) {
            edge1_r2 = ImVec2(r2_minX, p1_r2.y);
            edge2_r2 = ImVec2(r2_maxX, p1_r2.y);
        }
        else {
            float m_r2 = (p2_r2.y - p1_r2.y) / (p2_r2.x - p1_r2.x);
            float b_r2 = p1_r2.y - m_r2 * p1_r2.x;
            
            if (abs(p2_r2.x - p1_r2.x) > abs(p2_r2.y - p1_r2.y)) {
                edge1_r2 = ImVec2(r2_minX, m_r2 * r2_minX + b_r2);
                edge2_r2 = ImVec2(r2_maxX, m_r2 * r2_maxX + b_r2);
            } else {
                edge1_r2 = ImVec2((r2_minY - b_r2) / m_r2, r2_minY);
                edge2_r2 = ImVec2((r2_maxY - b_r2) / m_r2, r2_maxY);
            }
        }
        
        drawMixedLine(edge1_r2, edge2_r2, true);

        // r1 line (horizontal plane)
        ImVec2 p1_r1(cursorPos.x + viewportSize.x / 2 + x1 * 10, (cursorPos.y + viewportSize.y / 2) - y2 * 10);
        ImVec2 p2_r1(cursorPos.x + viewportSize.x / 2 + x2 * 10, (cursorPos.y + viewportSize.y / 2) - y4 * 10);
        
        float r1_minX = cursorPos.x;
        float r1_maxX = cursorPos.x + viewportSize.x;
        float r1_minY = cursorPos.y;
        float r1_maxY = cursorPos.y + viewportSize.y;
        
        ImVec2 edge1_r1, edge2_r1;
        
        // Handle vertical lines (x1 == x2)
        if (abs(p2_r1.x - p1_r1.x) < 0.0001f) {
            edge1_r1 = ImVec2(p1_r1.x, r1_minY);
            edge2_r1 = ImVec2(p1_r1.x, r1_maxY);
        }
        // Handle horizontal lines (y1 == y2)
        else if (abs(p2_r1.y - p1_r1.y) < 0.0001f) {
            edge1_r1 = ImVec2(r1_minX, p1_r1.y);
            edge2_r1 = ImVec2(r1_maxX, p1_r1.y);
        }
        else {
            float m_r1 = (p2_r1.y - p1_r1.y) / (p2_r1.x - p1_r1.x);
            float b_r1 = p1_r1.y - m_r1 * p1_r1.x;
            
            if (abs(p2_r1.x - p1_r1.x) > abs(p2_r1.y - p1_r1.y)) {
                edge1_r1 = ImVec2(r1_minX, m_r1 * r1_minX + b_r1);
                edge2_r1 = ImVec2(r1_maxX, m_r1 * r1_maxX + b_r1);
            } else {
                edge1_r1 = ImVec2((r1_minY - b_r1) / m_r1, r1_minY);
                edge2_r1 = ImVec2((r1_maxY - b_r1) / m_r1, r1_maxY);
            }
        }
        
        drawMixedLine(edge1_r1, edge2_r1, false);

        // draw labels
        ImGui::SetCursorScreenPos(ImVec2((edge1_r1.x + edge2_r1.x) / 2 - 15, (edge1_r1.y + edge2_r1.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c1", line.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((edge1_r2.x + edge2_r2.x) / 2 + 15, (edge1_r2.y + edge2_r2.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c2", line.name[0]);
    }
    
    for (const auto& plane : sceneData.planes) {
        const auto& p1 = sceneData.points[plane.point1index];
        const auto& p2 = sceneData.points[plane.point2index];
        const auto& p3 = sceneData.points[plane.point3index];

        // Calculate plane equation: Ax + By + Cz + D = 0
        glm::vec3 v1(p2.coords[0] - p1.coords[0], p2.coords[1] - p1.coords[1], p2.coords[2] - p1.coords[2]);
        glm::vec3 v2(p3.coords[0] - p1.coords[0], p3.coords[1] - p1.coords[1], p3.coords[2] - p1.coords[2]);
        glm::vec3 normal = glm::cross(v1, v2);
        
        float A = normal.x;
        float B = normal.y;
        float C = normal.z;
        float D = -(A * p1.coords[0] + B * p1.coords[1] + C * p1.coords[2]);
        
        // First vertical point (set x = 0)
        float z1_vert = (-D) / C;
        glm::vec3 vert_point1(0.0f, 0.0f, z1_vert / 3);

        // Second vertical point (set z = 0)
        float x1_vert = (-D) / A;
        glm::vec3 vert_point2(x1_vert / 2, 0.0f, 0.0f);

        // First horizontal point (set x = 0)
        float y1_horiz = (-D) / B;
        glm::vec3 horiz_point1(0.0f, -y1_horiz / 3, 0.0f);

        // Second horizontal point (set y = 0)
        float x2_horiz = (-D) / A;
        glm::vec3 horiz_point2(x2_horiz / 2, 0.0f, 0.0f);

        // Calculate the two points for the vertical line in 2D projection
        ImVec2 p1_vert(cursorPos.x + viewportSize.x / 2 + vert_point1.x * 10,
                   (cursorPos.y + viewportSize.y / 2) - vert_point1.z * 10);
        ImVec2 p2_vert(cursorPos.x + viewportSize.x / 2 + vert_point2.x * 10,
                   (cursorPos.y + viewportSize.y / 2) - vert_point2.z * 10);

        // Calculate the two points for the horizontal line in 2D projection
        ImVec2 p1_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point1.x * 10,
                (cursorPos.y + viewportSize.y / 2) - horiz_point1.y * 10);
        ImVec2 p2_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point2.x * 10,
                (cursorPos.y + viewportSize.y / 2) + horiz_point2.y * 10);

        // Extend the vertical line to the window borders (top and bottom)
        ImVec2 vert_dir = ImVec2(p2_vert.x - p1_vert.x, p2_vert.y - p1_vert.y);
        if (fabs(vert_dir.x) < 1e-5) { // vertical line
            p1_vert = ImVec2(p1_vert.x, cursorPos.y);
            p2_vert = ImVec2(p1_vert.x, cursorPos.y + viewportSize.y);
        } else {
            float m = vert_dir.y / vert_dir.x;
            float b = p1_vert.y - m * p1_vert.x;
            // Intersect with top and bottom borders
            float x_top = (cursorPos.y - b) / m;
            float x_bottom = (cursorPos.y + viewportSize.y - b) / m;
            p1_vert = ImVec2(x_top, cursorPos.y);
        }

        // Extend the horizontal line to the window borders (left and right)
        ImVec2 horiz_dir = ImVec2(p2_horiz.x - p1_horiz.x, p2_horiz.y - p1_horiz.y);
        if (fabs(horiz_dir.y) < 1e-5) { // horizontal line
            p1_horiz = ImVec2(cursorPos.x, p1_horiz.y);
            p2_horiz = ImVec2(cursorPos.x + viewportSize.x, p1_horiz.y);
        } else {
            float m = horiz_dir.y / horiz_dir.x;
            float b = p1_horiz.y - m * p1_horiz.x;
            // Intersect with left and right borders
            float y_left = m * cursorPos.x + b;
            float y_right = m * (cursorPos.x + viewportSize.x) + b;
            p1_horiz = ImVec2(cursorPos.x, y_left);
        }

        // handle vertical lines (x1 == x2)
        if (abs(p2_vert.x - p1_vert.x) < 0.0001f) {
            p1_vert = ImVec2(p1_vert.x, cursorPos.y);
            p2_vert = ImVec2(p1_vert.x, cursorPos.y + viewportSize.y);
        }
        // handle horizontal lines (y1 == y2)
        else if (abs(p2_horiz.y - p1_horiz.y) < 0.0001f) {
            p1_horiz = ImVec2(cursorPos.x, p1_horiz.y);
            p2_horiz = ImVec2(cursorPos.x + viewportSize.x, p1_horiz.y);
        }

        // Draw the plane lines
        drawList->AddLine(p1_horiz, p2_horiz, lineColor, 3.0f);
        drawList->AddLine(p1_vert, p2_vert, lineColor, 3.0f);
        
        // add labels
        ImGui::SetCursorScreenPos(ImVec2((p1_horiz.x + p2_horiz.x) / 2 - 15, (p1_horiz.y + p2_horiz.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c1", plane.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((p1_vert.x + p2_vert.x) / 2 + 15, (p1_vert.y + p2_vert.y) / 2 - 20));
        ImGui::TextColored(lineColor, "%c2", plane.name[0]);
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}