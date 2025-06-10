#pragma once

#include "app.h"
#include <imgui.h>
#include <imgui_internal.h>

class DihedralViewport {
public:
    void Draw(App& app);

private:
    void DrawGroundLine(ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor);
    void DrawPoints(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor);
    void DrawLines(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor);
    void DrawPlanes(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor);
    
    void CalculateEdgePoints(const ImVec2& p1, const ImVec2& p2, 
                           float minX, float maxX, float minY, float maxY,
                           ImVec2& edge1, ImVec2& edge2);
    void DrawLineWithLabels(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2,
                           float minX, float maxX, float minY, float maxY,
                           ImU32 color, char lineName, bool is2, bool dashed);
};