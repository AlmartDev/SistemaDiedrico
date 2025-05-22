#pragma once

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

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