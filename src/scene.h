#pragma once

#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>

struct SceneData {
    struct Point {
        std::string name;
        float coords[3];
        bool hidden = false;
        float color[3] = {1.0f, 0.5f, 0.0f}; // orange
    };

    struct Line {
        std::string name;
        int point1index;
        int point2index;
        float color[3] = {1.0f, 1.0f, 1.0f}; // white
        bool showVisibility = false;
    };

    struct Plane {
        std::string name;
        int point1index;
        int point2index;
        int point3index;
        float color[3] = {0.5f, 0.5f, 0.5f}; // gray
        bool expand = false;
    };

    struct Settings {
        float backgroundColor[3] = {0.13f, 0.13f, 0.13f};
        float dihedralBackgroundColor[3] = {1.0f, 1.0f, 1.0f};
        float dihedralLineColor[3] = {0.1f, 0.1f, 0.1f};

        int axesType = 2; 

        bool showDihedralSystem = true;

        bool showCutPoints = true;
        bool showCutLines = true;
        bool showCutPlanes = true;
        bool expandPlanes = false;

        float mouseSensitivity = 0.2f;
        float cameraDistance = 5.5f;
        float fontSize = 14.0f;

        float pointSize = 8.0f;
        float lineThickness = 3.0f;
        float planeOpacity = 0.6f;

        float offset[2] = {0.0f, 0.0f};
        bool VSync = true;

        bool showWelcomeWindow = true;
    };

    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Plane> planes;

    Settings settings;
};

#endif // SCENE_H