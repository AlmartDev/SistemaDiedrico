#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "scene.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
    #include <emscripten/emscripten.h>
    #include <emscripten/fetch.h>
    #include <emscripten/html5.h>
    #include <emscripten/val.h>
    #include <stdio.h>
    #include <sys/stat.h>
#else
    #include <windows.h>
#endif

class JsonHandler {
public:
#ifdef __EMSCRIPTEN__
    std::string loadedContent;
    emscripten::val FS = emscripten::val::global("FS");
    std::string loadedPath = "/loaded.json";
    bool fileLoaded = false;
    std::function<void()> onFileLoadedCallback;
#endif

    std::vector<nlohmann::json> Load(std::string &filename) {
        nlohmann::json content;

#ifdef __EMSCRIPTEN__ // this crashes on web builds, so we use a different method
        if (!fileLoaded) {
            std::cout << "No file has been loaded yet or loading is in progress" << std::endl;
            return {};
        }

        FILE *file = fopen(filename.c_str(), "r");

        if (!file) {
            std::cout << "Failed to open file: " << filename << std::endl;
            return {};
        }

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        std::string buffer(size, '\0');
        fread(&buffer[0], 1, size, file);
        fclose(file);

        try {
            content = nlohmann::json::parse(buffer);
        } catch (const nlohmann::json::parse_error &e) {
            std::cout << "JSON parse error: " << e.what() << std::endl;
            return {};
        }

        std::cout << "Loaded JSON content: " << filename << std::endl;
#else
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return {};
        }

        try {
            file >> content;
        } catch (const nlohmann::json::parse_error &e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }

        file.close();
#endif
        std::vector<nlohmann::json> points, lines, planes;

        if (content.contains("points")) {
            for (const auto &item : content["points"]) points.push_back(item);
        } else
            points.push_back(content);

        if (content.contains("lines")) {
            for (const auto &item : content["lines"]) lines.push_back(item);
        } else
            lines.push_back(nlohmann::json::array());

        if (content.contains("planes")) {
            for (const auto &item : content["planes"]) planes.push_back(item);
        } else
            planes.push_back(nlohmann::json::array());

        return {points, lines, planes};
    }

    // LOAD ---------------------------------------------------------------

    nlohmann::json LoadPresets(const std::string &filename) {
        nlohmann::json content;

        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }

        try {
            file >> content;
        } catch (const nlohmann::json::parse_error &e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }

        file.close();

        return content;
    }

    // SAVE --------------------------------------------------------------

    void Save(const std::string& filename, SceneData& sceneData) {
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
            // get the 2 points by their indexes
            lineJson["point1"] = sceneData.points[line.point1index].coords;
            lineJson["point2"] = sceneData.points[line.point2index].coords;

            lineJson["color"] = {line.color[0], line.color[1], line.color[2]};
            lineJson["showVisibility"] = line.showVisibility;
            content["lines"].push_back(lineJson);
        }

        content["planes"] = nlohmann::json::array();
        for (const auto& plane : sceneData.planes) {
            nlohmann::json planeJson;
            planeJson["name"] = plane.name;
            // get the 3 points by their indexes
            planeJson["point1"] = sceneData.points[plane.point1index].coords;
            planeJson["point2"] = sceneData.points[plane.point2index].coords;
            planeJson["point3"] = sceneData.points[plane.point3index].coords;

            planeJson["color"] = {plane.color[0], plane.color[1], plane.color[2]};
            planeJson["expand"] = plane.expand;
            content["planes"].push_back(planeJson);
        }

        std::string jsonStr = content.dump(4);

#ifdef __EMSCRIPTEN__
        // For web builds, we'll use the filename from the dialog
        EM_ASM_({
            const content = UTF8ToString($0);
            const filename = UTF8ToString($1) || 'scene.json';
            
            // Create a Blob with the JSON data
            const blob = new Blob([content], {type: 'application/json'});
            const url = URL.createObjectURL(blob);
            
            // Create a download link and trigger click
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            
            // Cleanup
            setTimeout(() => {
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
            }, 100);
        }, jsonStr.c_str(), filename.c_str());
#else
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }

        try {
            file << jsonStr;
        } catch (const nlohmann::json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << std::endl;
        }
        file.close();
#endif
    }

    std::string OpenFileDialog() {
#ifdef __EMSCRIPTEN__
        fileLoaded = false;

        EM_ASM({
            if (!document.getElementById('fileLoader')) {
                const input = document.createElement('input');
                input.type = 'file';
                input.id = 'fileLoader';
                input.accept = '.json';
                input.style.display = 'none';
                
                input.onchange = function(e) {
                    const file = e.target.files[0];
                    const reader = new FileReader();
                    reader.onload = function(event) {
                        const content = event.target.result;
                        // Directly pass to C++ (no UTF8ToString needed)
                        Module.handleFileLoad(content);
                    };
                    reader.readAsText(file);
                };
                document.body.appendChild(input);
            }
            document.getElementById('fileLoader').click();
        });

        return "/loaded.json"; // Return the virtual filesystem path
#elif _WIN32
        OPENFILENAMEA ofn;
        CHAR szFile[MAX_PATH] = {0};
        CHAR currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        const char *filter =
            "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = currentDir;
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
        return "";
#else
        return "";
#endif
    }

    std::string SaveFileDialog( const std::string &filter = "JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0") {
#ifdef __EMSCRIPTEN__  // emscriptem
        // For web builds, we'll use the filename directly in the Save method
        return "scene.json";
#elif _WIN32
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
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
        ofn.lpstrDefExt = "json";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn)) {
            return ofn.lpstrFile;
        }

        return "";
#else
        return "";
#endif
    }

    std::vector<nlohmann::json> GetPointPresets(nlohmann::json &jsonData) {
        std::vector<nlohmann::json> presets;

        if (jsonData.contains("presets") &&
            jsonData["presets"].contains("points")) {
            for (const auto &preset : jsonData["presets"]["points"]) {
                presets.push_back(preset);
            }
        }

        return presets;
    }

    std::vector<nlohmann::json> GetLinePresets(nlohmann::json &jsonData) {
        std::vector<nlohmann::json> presets;

        if (jsonData.contains("presets") &&
            jsonData["presets"].contains("lines")) {
            for (const auto &preset : jsonData["presets"]["lines"]) {
                presets.push_back(preset);
            }
        }

        return presets;
    }

    std::vector<nlohmann::json> GetPlanePresets(nlohmann::json &jsonData) {
        std::vector<nlohmann::json> presets;

        if (jsonData.contains("presets") &&
            jsonData["presets"].contains("planes")) {
            for (const auto &preset : jsonData["presets"]["planes"]) {
                presets.push_back(preset);
            }
        }

        return presets;
    }
};

#ifdef __EMSCRIPTEN__
extern JsonHandler* JsonHandlerInstance();
extern "C" void handleFileLoad(const std::string& content); 
#endif