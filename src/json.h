#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "scene.h"

class JsonHandler {
public:
    JsonHandler(const std::string& filename) : m_filename(filename) {}
    
    bool LoadJson() {
        std::ifstream file(m_filename);
        if (!file.is_open()) {
            std::cerr << "Error opening JSON file: " << m_filename << std::endl;
            return false;
        }

        try {
            file >> m_jsonData;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return false;
        }

        file.close();
        return true;
    }
#ifndef __EMSCRIPTEN__
    bool JsonHandler::SaveProject(const SceneData& sceneData) {
        nlohmann::json projectData;
        // Save points
        for (const auto& point : sceneData.points) {
            if (point.hidden) continue;
            
            nlohmann::json p;
            p["name"] = point.name;
            p["coords"] = {point.coords[0], point.coords[1], point.coords[2]};
            p["color"] = {point.color[0], point.color[1], point.color[2]};
            projectData["points"].push_back(p);
        }

        // Save lines with full point data
        for (const auto& line : sceneData.lines) {
            const auto& p1 = sceneData.points[line.point1index];
            const auto& p2 = sceneData.points[line.point2index];
            
            nlohmann::json l;
            l["name"] = line.name;
            l["points"] = {
                {{"name", p1.name}, {"coords", {p1.coords[0], p1.coords[1], p1.coords[2]}}},
                {{"name", p2.name}, {"coords", {p2.coords[0], p2.coords[1], p2.coords[2]}}
            }};
            l["color"] = {line.color[0], line.color[1], line.color[2]};
            projectData["lines"].push_back(l);
        }

        // Save planes with full point data
        for (const auto& plane : sceneData.planes) {
            const auto& p1 = sceneData.points[plane.point1index];
            const auto& p2 = sceneData.points[plane.point2index];
            const auto& p3 = sceneData.points[plane.point3index];
            
            nlohmann::json pl;
            pl["name"] = plane.name;
            pl["points"] = {
                {{"name", p1.name}, {"coords", {p1.coords[0], p1.coords[1], p1.coords[2]}}},
                {{"name", p2.name}, {"coords", {p2.coords[0], p2.coords[1], p2.coords[2]}}},
                {{"name", p3.name}, {"coords", {p3.coords[0], p3.coords[1], p3.coords[2]}}}
            };
            pl["expand"] = plane.expand;
            projectData["planes"].push_back(pl);
        }

        // Save to file
        std::ofstream file(m_filename);
        file << projectData.dump(4);
        file.close();
        return true;
    }

    bool LoadProject(SceneData& sceneData) {
        if (!LoadJson()) return false;

        try {
            sceneData.points.clear();
            sceneData.lines.clear();
            sceneData.planes.clear();

            // Load points
            for (const auto& jsonPoint : m_jsonData["points"]) {
                SceneData::Point point;
                point.name = jsonPoint["name"];
                std::copy(jsonPoint["coords"].begin(), jsonPoint["coords"].end(), point.coords);
                std::copy(jsonPoint["color"].begin(), jsonPoint["color"].end(), point.color);
                point.hidden = jsonPoint.value("hidden", false);
                sceneData.points.push_back(point);
            }

            // Load lines
            for (const auto& jsonLine : m_jsonData["lines"]) {
                SceneData::Line line;
                line.name = jsonLine["name"];
                
                // Find or create points
                for (const auto& jsonPoint : jsonLine["points"]) {
                    auto it = std::find_if(sceneData.points.begin(), sceneData.points.end(),
                        [&](const SceneData::Point& p) { return p.name == jsonPoint["name"]; });
                    
                    if (it == sceneData.points.end()) {
                        // Create new point if not found
                        SceneData::Point newPoint;
                        newPoint.name = jsonPoint["name"];
                        auto coords = jsonPoint["coords"];
                        if (!coords.is_array() || coords.size() != 3) {
                            throw std::runtime_error("Invalid coordinates for point " + jsonPoint["name"].get<std::string>());
                        }
                        std::copy(coords.begin(), coords.end(), newPoint.coords);
                        sceneData.points.push_back(newPoint);
                        it = sceneData.points.end() - 1;
                    }
                    
                    if (line.point1index == -1) line.point1index = it - sceneData.points.begin();
                    else line.point2index = it - sceneData.points.begin();
                }

                //std::copy(jsonLine["color"].begin(), jsonLine["color"].end(), line.color);
                sceneData.lines.push_back(line);
            }

            // Load planes
            for (const auto& jsonPlane : m_jsonData["planes"]) {
                SceneData::Plane plane;
                plane.name = jsonPlane["name"];
                
                // Find or create points
                for (const auto& jsonPoint : jsonPlane["points"]) {
                    auto it = std::find_if(sceneData.points.begin(), sceneData.points.end(),
                        [&](const SceneData::Point& p) { return p.name == jsonPoint["name"]; });
                    
                    if (it == sceneData.points.end()) {
                        // Create new point if not found
                        SceneData::Point newPoint;
                        newPoint.name = jsonPoint["name"];

                        auto coords = jsonPoint["coords"];
                        if (!coords.is_array() || coords.size() != 3) {
                            throw std::runtime_error("Invalid coordinates for point " + jsonPoint["name"].get<std::string>());
                        }
                        std::copy(coords.begin(), coords.end(), newPoint.coords);

                        sceneData.points.push_back(newPoint);
                        it = sceneData.points.end() - 1;
                    }
                    
                    if (plane.point1index == -1) plane.point1index = it - sceneData.points.begin();
                    else if (plane.point2index == -1) plane.point2index = it - sceneData.points.begin();
                    else plane.point3index = it - sceneData.points.begin();
                }
                
                plane.expand = jsonPlane.value("expand", false);
                //std::copy(jsonPlane["color"].begin(), jsonPlane["color"].end(), plane.color);
                sceneData.planes.push_back(plane);
            }

            return true;
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON error loading project: " << e.what() << std::endl;
            return false;
        }
    }
#endif

    std::vector<nlohmann::json> GetPointPresets() {
        std::vector<nlohmann::json> presets;
        if (m_jsonData.contains("presets") && m_jsonData["presets"].contains("points")) {
            for (const auto& preset : m_jsonData["presets"]["points"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }

    std::vector<nlohmann::json> GetLinePresets() {
        std::vector<nlohmann::json> presets;
        if (m_jsonData.contains("presets") && m_jsonData["presets"].contains("lines")) {
            for (const auto& preset : m_jsonData["presets"]["lines"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }

    std::vector<nlohmann::json> GetPlanePresets() {
        std::vector<nlohmann::json> presets;
        if (m_jsonData.contains("presets") && m_jsonData["presets"].contains("planes")) {
            for (const auto& preset : m_jsonData["presets"]["planes"]) {
                presets.push_back(preset);
            }
        }
        return presets;
    }

private:
    std::string m_filename;
    nlohmann::json m_jsonData;
};