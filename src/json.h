#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "scene.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#else
    #include <windows.h>
#endif

class JsonHandler {
public:    
    void Load(std::string& filename) { // load any json file and return its content
        nlohmann::json content;

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }

        try {
            file >> content;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
        file.close();

        SceneData sceneData;
        
        if (content.contains("points")) {
            for (const auto& pointJson : content["points"]) {
                SceneData::Point point;
                point.name = pointJson.value("name", "");
                point.coords[0] = pointJson["coords"].value("d", 0.0f);
                point.coords[1] = pointJson["coords"].value("a", 0.0f);
                point.coords[2] = pointJson["coords"].value("c", 0.0f);
                point.hidden = pointJson.value("hidden", false);
                point.userCreated = pointJson.value("userCreated", false);
                if (pointJson.contains("color")) {
                    const auto& color = pointJson["color"];
                    if (color.is_array() && color.size() == 3) {
                        point.color[0] = color[0].get<float>();
                        point.color[1] = color[1].get<float>();
                        point.color[2] = color[2].get<float>();
                    }
                }
                sceneData.points.push_back(point);
            }
        }
        if (content.contains("lines")) {
            for (const auto& lineJson : content["lines"]) {
                SceneData::Line line;
                line.name = lineJson.value("name", "");
                line.point1index = lineJson.value("point1", -1);
                line.point2index = lineJson.value("point2", -1);
                if (lineJson.contains("color")) {
                    const auto& color = lineJson["color"];
                    if (color.is_array() && color.size() == 3) {
                        line.color[0] = color[0].get<float>();
                        line.color[1] = color[1].get<float>();
                        line.color[2] = color[2].get<float>();
                    }
                }
                line.showVisibility = lineJson.value("showVisibility", false);
                sceneData.lines.push_back(line);
            }
        }
        if (content.contains("planes")) {
            for (const auto& planeJson : content["planes"]) {
                SceneData::Plane plane;
                plane.name = planeJson.value("name", "");
                plane.point1index = planeJson.value("point1", -1);
                plane.point2index = planeJson.value("point2", -1);
                plane.point3index = planeJson.value("point3", -1);
                if (planeJson.contains("color")) {
                    const auto& color = planeJson["color"];
                    if (color.is_array() && color.size() == 3) {
                        plane.color[0] = color[0].get<float>();
                        plane.color[1] = color[1].get<float>();
                        plane.color[2] = color[2].get<float>();
                    }
                }
                plane.expand = planeJson.value("expand", false);
                sceneData.planes.push_back(plane);
            }
        }
    }

    nlohmann::json LoadPresets(const std::string& filename) {
        nlohmann::json content;

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }

        try {
            file >> content;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
        file.close();

        return content;
    }

    void Save(const std::string& filename, SceneData& sceneData) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
        }

        nlohmann::json content;
        content["points"] = nlohmann::json::array();
        for (const auto& point : sceneData.points) {
            nlohmann::json pointJson;
            pointJson["name"] = point.name;
            pointJson["coords"] = {{"d", point.coords[0]}, {"a", point.coords[1]}, {"c", point.coords[2]}};
            pointJson["hidden"] = point.hidden;
            pointJson["userCreated"] = point.userCreated;
            pointJson["color"] = {point.color[0], point.color[1], point.color[2]};
            content["points"].push_back(pointJson);
        }
        content["lines"] = nlohmann::json::array();
        for (const auto& line : sceneData.lines) {
            nlohmann::json lineJson;
            lineJson["name"] = line.name;
            lineJson["point1"] = line.point1index;
            lineJson["point2"] = line.point2index;
            lineJson["color"] = {line.color[0], line.color[1], line.color[2]};
            lineJson["showVisibility"] = line.showVisibility;
            content["lines"].push_back(lineJson);
        }
        content["planes"] = nlohmann::json::array();
        for (const auto& plane : sceneData.planes) {
            nlohmann::json planeJson;
            planeJson["name"] = plane.name;
            planeJson["point1"] = plane.point1index;
            planeJson["point2"] = plane.point2index;
            planeJson["point3"] = plane.point3index;
            planeJson["color"] = {plane.color[0], plane.color[1], plane.color[2]};
            planeJson["expand"] = plane.expand;
            content["planes"].push_back(planeJson);
        }

        // write to file
        try {
            file << content.dump(4); // pretty print with 4 spaces
        } catch (const nlohmann::json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << std::endl;
        }
        file.close();
    }

    // TODO: file dialog
    std::string OpenFileDialog() {
        
#ifdef __EMSCRIPTEN__ // browser dialog

#elif _WIN32 // widndows api
     OPENFILENAMEA ofn;
        CHAR szFile[MAX_PATH] = { 0 };
        CHAR currentDir[MAX_PATH];

        // Get current directory
        GetCurrentDirectoryA(MAX_PATH, currentDir);

        // Set up filter (must use double null-termination)
        const char* filter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1; // Default to JSON filter
        ofn.lpstrInitialDir = currentDir; // Start in current directory
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn)) {
            return ofn.lpstrFile;
        }
        else {
            DWORD err = CommDlgExtendedError();
            if (err != 0) {
                std::cerr << "File dialog error: " << err << std::endl;
            }
        }
#else // imgui file dialog

#endif
        return "";
    }

    std::string SaveFileDialog(const std::string& filter = "JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0") {
#ifdef __EMSCRIPTEN__ // browser dialog

#elif _WIN32 // widndows api
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        
        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter.c_str();
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrDefExt = "json"; // Default extension
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        
        if (GetSaveFileNameA(&ofn)) {
            return ofn.lpstrFile;
        }
#else // imgui file dialog
#endif
        return "";
    }

    std::vector<nlohmann::json> GetPointPresets(nlohmann::json& jsonData) {
        std::vector<nlohmann::json> presets;
        if (jsonData.contains("presets") && jsonData["presets"].contains("points")) {
            for (const auto& preset : jsonData["presets"]["points"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }

    std::vector<nlohmann::json> GetLinePresets(nlohmann::json& jsonData) {
        std::vector<nlohmann::json> presets;
        if (jsonData.contains("presets") && jsonData["presets"].contains("lines")) {
            for (const auto& preset : jsonData["presets"]["lines"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }

    std::vector<nlohmann::json> GetPlanePresets(nlohmann::json& jsonData) {
        std::vector<nlohmann::json> presets;
        if (jsonData.contains("presets") && jsonData["presets"].contains("planes")) {
            for (const auto& preset : jsonData["presets"]["planes"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }
};