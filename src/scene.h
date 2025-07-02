#pragma once

#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>

struct Point {
    std::string name;
    float coords[3];
    bool hidden = false;
    bool userCreated = false;
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

struct SceneData {
    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Plane> planes;

    struct Settings {
        float backgroundColor[3] = {0.13f, 0.13f, 0.13f};
        float dihedralBackgroundColor[3] = {1.0f, 1.0f, 1.0f};
        float dihedralLineColor[3] = {0.1f, 0.1f, 0.1f};

        int axesType = 2; 

        bool showDihedralSystem = true;

        bool showCutPoints = false;
        bool showCutLines = false;
        bool showCutPlanes = false; // doesnt do anything yet
        bool expandPlanes = false;

        float mouseSensitivity = 0.2f;
        bool invertMouse[2] = {false, false}; 
        float cameraDistance;
        float fontSize = 14.0f;

        float worldScale = 50.0f; // scale for coordinates

        float pointSize = 6.0f;
        float lineThickness = 1.5f;
        float planeOpacity = 0.6f;

        bool showLabels[3] = {true, true, true}; // Points, Lines, Planes
        bool showQuadrantLabels = false;

        float offset[2] = {-75.0f, 0.0f};
        bool VSync = true;

        std::string loadedFileName = "";

        bool showWelcomeWindow = true;

        // TRANSLATION
        std::string defaultLanguage = "EN";
    } settings;
};

#endif // SCENE_H