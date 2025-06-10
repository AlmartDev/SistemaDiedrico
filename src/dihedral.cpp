#include "dihedral.h"
#include "app.h"
#include <glm/glm.hpp>

void DihedralViewport::Draw(App& app) {
    auto& sceneData = app.GetSceneData();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(
        sceneData.settings.dihedralBackgroundColor[0], 
        sceneData.settings.dihedralBackgroundColor[1], 
        sceneData.settings.dihedralBackgroundColor[2], 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
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

    DrawGroundLine(drawList, cursorPos, viewportSize, lineColor);
    DrawPoints(app, drawList, cursorPos, viewportSize, lineColor);
    DrawLines(app, drawList, cursorPos, viewportSize, lineColor);
    DrawPlanes(app, drawList, cursorPos, viewportSize, lineColor);

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void DihedralViewport::DrawGroundLine(ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor) {
    // Ground line (L.T.)
    ImVec2 p0(cursorPos.x, cursorPos.y + viewportSize.y / 2);
    ImVec2 p1(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y / 2);
    drawList->AddLine(p0, p1, lineColor, 2.5f); 

    // Small indicator lines
    drawList->AddLine(ImVec2(p0.x + 4, p0.y + 5), ImVec2(p0.x + 30, p0.y + 5), lineColor, 2.0f);
    drawList->AddLine(ImVec2(p1.x - 4, p1.y + 5), ImVec2(p1.x - 30, p1.y + 5), lineColor, 2.0f);
}

void DihedralViewport::DrawPoints(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor) {
    auto& sceneData = app.GetSceneData();
    ImVec2 viewportCenter(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2);

    for (const auto& point : sceneData.points) {
        if (point.hidden) continue;

        float x = point.coords[0] / 2.0f;
        float y1 = point.coords[2] / 3.0f;
        float y2 = -point.coords[1] / 3.0f;

        ImVec2 pos1(viewportCenter.x + x * 10, viewportCenter.y - y1 * 10);
        ImVec2 pos2(viewportCenter.x + x * 10, viewportCenter.y - y2 * 10);

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

        ImVec2 ltPos(viewportCenter.x + x * 10, viewportCenter.y);
        drawList->AddLine(pos1, ltPos, lineColor, .75f);
        drawList->AddLine(pos2, ltPos, lineColor, .75f);
    }
}

void DihedralViewport::CalculateEdgePoints(const ImVec2& p1, const ImVec2& p2, 
                         float minX, float maxX, float minY, float maxY,
                         ImVec2& edge1, ImVec2& edge2) {
    // Handle vertical lines (x1 == x2)
    if (abs(p2.x - p1.x) < 0.0001f) {
        edge1 = ImVec2(p1.x, minY);
        edge2 = ImVec2(p1.x, maxY);
    }
    // Handle horizontal lines (y1 == y2)
    else if (abs(p2.y - p1.y) < 0.0001f) {
        edge1 = ImVec2(minX, p1.y);
        edge2 = ImVec2(maxX, p1.y);
    }
    else {
        float m = (p2.y - p1.y) / (p2.x - p1.x);
        float b = p1.y - m * p1.x;

        if (abs(p2.x - p1.x) > abs(p2.y - p1.y)) {
            edge1 = ImVec2(minX, m * minX + b);
            edge2 = ImVec2(maxX, m * maxX + b);
        }
        else {
            edge1 = ImVec2((minY - b) / m, minY);
            edge2 = ImVec2((maxY - b) / m, maxY);
        }
    }
}

void DihedralViewport::DrawLineWithLabels(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2,
                        float minX, float maxX, float minY, float maxY,
                        ImU32 color, char lineName, bool is2, bool dashed) {
    ImVec2 edge1, edge2;
    CalculateEdgePoints(p1, p2, minX, maxX, minY, maxY, edge1, edge2);
    
    if (dashed) {
        // Draw dashed line
        const float dashLength = 10.0f;
        const float gapLength = 5.0f;
        ImVec2 dir = ImVec2(edge2.x - edge1.x, edge2.y - edge1.y);
        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
        float dirLength = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dirLength > 0.0001f) {
            dir.x /= dirLength;
            dir.y /= dirLength;
        }
        
        for (float i = 0; i < length; i += dashLength + gapLength) {
            ImVec2 start = ImVec2(edge1.x + dir.x * i, edge1.y + dir.y * i);
            ImVec2 end = ImVec2(start.x + dir.x * dashLength, start.y + dir.y * dashLength);
            drawList->AddLine(start, end, color, 1.0f);
        }
    } else {
        // Draw solid line
        drawList->AddLine(edge1, edge2, color, 1.0f);
    }    
    // Draw labels
    float labelX = (edge1.x + edge2.x) / 2 + (is2 ? 15 : -15);
    float labelY = (edge1.y + edge2.y) / 2 - 20;
    ImGui::SetCursorScreenPos(ImVec2(labelX, labelY));

    ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);
    ImGui::TextColored(colorVec, "%c%d", lineName, is2 ? 2 : 1);
}


void DihedralViewport::DrawLines(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor) {
    auto& sceneData = app.GetSceneData();
    ImVec2 viewportCenter(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2);

    for (const auto& line : sceneData.lines) {
        const auto& p1 = sceneData.points[line.point1index];
        const auto& p2 = sceneData.points[line.point2index];

        // Calculate transformed coordinates
        float x1 = p1.coords[0] / 2.0f;
        float y1_r2 = p1.coords[2] / 3.0f;
        float y1_r1 = -p1.coords[1] / 3.0f;

        float x2 = p2.coords[0] / 2.0f;
        float y2_r2 = p2.coords[2] / 3.0f;
        float y2_r1 = -p2.coords[1] / 3.0f;

        float scale = 10.0f;

        // R2 line (vertical plane)
        ImVec2 p1_r2(viewportCenter.x + x1 * scale, viewportCenter.y - y1_r2 * scale);
        ImVec2 p2_r2(viewportCenter.x + x2 * scale, viewportCenter.y - y2_r2 * scale);
        DrawLineWithLabels(drawList, p1_r2, p2_r2, 
                        cursorPos.x, cursorPos.x + viewportSize.x,
                        cursorPos.y, cursorPos.y + viewportSize.y,
                        lineColor, line.name[0], true, false);

        // R1 line (horizontal plane)
        ImVec2 p1_r1(viewportCenter.x + x1 * scale, viewportCenter.y - y1_r1 * scale);
        ImVec2 p2_r1(viewportCenter.x + x2 * scale, viewportCenter.y - y2_r1 * scale);
        DrawLineWithLabels(drawList, p1_r1, p2_r1,
                        cursorPos.x, cursorPos.x + viewportSize.x,
                        cursorPos.y, cursorPos.y + viewportSize.y,
                        lineColor, line.name[0], false, false);

        // Draw ground points
        if (y1_r2 * y2_r2 <= 0) {  // Check if line crosses ground level
            float t = -y1_r2 / (y2_r2 - y1_r2);
            float x_ground = x1 + t * (x2 - x1);
            float y_r1_ground = y1_r1 + t * (y2_r1 - y1_r1);
            
            ImVec2 groundPoint(
                viewportCenter.x + x_ground * scale,
                viewportCenter.y - y_r1_ground * scale
            );

            // Optional: Draw a vertical line from r2 to ground
            ImVec2 r2_groundPoint(
                viewportCenter.x + x_ground * scale,
                viewportCenter.y  // Ground level in r2 view is at viewportCenter.y (y=0)
            );

            if (sceneData.settings.showCutLines) {
                drawList->AddCircleFilled(groundPoint, 3.0f, IM_COL32(0, 0, 255, 255));
                drawList->AddLine(r2_groundPoint, groundPoint, IM_COL32(100, 100, 100, 128), 1.0f);
            }
        }

        // V
        if (y1_r1 * y2_r1 <= 0) { 
            float t = -y1_r1 / (y2_r1 - y1_r1);
            float x_ground = x1 + t * (x2 - x1);
            float y_r2_ground = y1_r2 + t * (y2_r2 - y1_r2);
            
            ImVec2 groundPoint(
                viewportCenter.x + x_ground * scale,
                viewportCenter.y - y_r2_ground * scale
            );
            
            // Optional: Draw a horizontal line from r1 to ground
            ImVec2 r1_groundPoint(
                viewportCenter.x + x_ground * scale,
                viewportCenter.y  // Ground level in r1 view is at viewportCenter.y (y=0)
            );
            
            if (sceneData.settings.showCutLines) {
                drawList->AddCircleFilled(groundPoint, 3.0f, IM_COL32(255, 0, 0, 255));
                drawList->AddLine(r1_groundPoint, groundPoint, IM_COL32(100, 100, 100, 128), 1.0f);
            }
        }

        // H
        if (y1_r2 * y2_r1 <= 0 && y1_r1 * y2_r2 <= 0) { 
            float t1 = -y1_r2 / (y2_r2 - y1_r2);
            float x_ground1 = x1 + t1 * (x2 - x1);
            float y_r1_ground1 = y1_r1 + t1 * (y2_r1 - y1_r1);
            
            ImVec2 groundPointR2(
                viewportCenter.x + x_ground1 * scale,
                viewportCenter.y - y_r1_ground1 * scale
            );

            float t2 = -y1_r1 / (y2_r1 - y1_r1);
            float x_ground2 = x1 + t2 * (x2 - x1);
            float y_r2_ground2 = y1_r2 + t2 * (y2_r2 - y1_r2);
            
            ImVec2 groundPointR1(
                viewportCenter.x + x_ground2 * scale,
                viewportCenter.y - y_r2_ground2 * scale
            );

            if (sceneData.settings.showCutLines) {
                drawList->AddCircleFilled(groundPointR2, 3.0f, IM_COL32(255, 0, 0, 255));
                drawList->AddCircleFilled(groundPointR1, 3.0f, IM_COL32(255, 0, 0, 255));
                drawList->AddLine(groundPointR2, groundPointR1, IM_COL32(100, 100, 100, 128), 1.0f);
            }
        }
    }
}

void DihedralViewport::DrawPlanes(App& app, ImDrawList* drawList, const ImVec2& cursorPos, const ImVec2& viewportSize, ImU32 lineColor) {
    auto& sceneData = app.GetSceneData();
    ImVec2 viewportCenter(cursorPos.x + viewportSize.x / 2, cursorPos.y + viewportSize.y / 2);

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
        
        float z1_vert = (-D) / C;
        glm::vec3 vert_point1(0.0f, 0.0f, z1_vert / 3);

        float x1_vert = (-D) / A;
        glm::vec3 vert_point2(x1_vert / 2, 0.0f, 0.0f);

        float y1_horiz = (-D) / B;
        glm::vec3 horiz_point1(0.0f, -y1_horiz / 3, 0.0f);

        float x2_horiz = (-D) / A;
        glm::vec3 horiz_point2(x2_horiz / 2, 0.0f, 0.0f);

        ImVec2 p1_vert(cursorPos.x + viewportSize.x / 2 + vert_point1.x * 10,
                   (cursorPos.y + viewportSize.y / 2) - vert_point1.z * 10);
        ImVec2 p2_vert(cursorPos.x + viewportSize.x / 2 + vert_point2.x * 10,
                   (cursorPos.y + viewportSize.y / 2) - vert_point2.z * 10);

        ImVec2 p1_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point1.x * 10,
                (cursorPos.y + viewportSize.y / 2) - horiz_point1.y * 10);
        ImVec2 p2_horiz(cursorPos.x + viewportSize.x / 2 + horiz_point2.x * 10,
                (cursorPos.y + viewportSize.y / 2) + horiz_point2.y * 10);

        ImVec2 vert_dir = ImVec2(p2_vert.x - p1_vert.x, p2_vert.y - p1_vert.y);
        if (fabs(vert_dir.x) < 1e-5) { // vertical line
            p1_vert = ImVec2(p1_vert.x, cursorPos.y);
            p2_vert = ImVec2(p1_vert.x, cursorPos.y + viewportSize.y);
        } else {
            float m = vert_dir.y / vert_dir.x;
            float b = p1_vert.y - m * p1_vert.x;

            float x_top = (cursorPos.y - b) / m;
            float x_bottom = (cursorPos.y + viewportSize.y - b) / m;
            p1_vert = ImVec2(x_top, cursorPos.y);
        }

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
        ImVec4 lineColorVec = ImGui::ColorConvertU32ToFloat4(lineColor);
        ImGui::TextColored(lineColorVec, "%c1", plane.name[0]);
        ImGui::SetCursorScreenPos(ImVec2((p1_vert.x + p2_vert.x) / 2 + 15, (p1_vert.y + p2_vert.y) / 2 - 20));
        ImGui::TextColored(lineColorVec, "%c2", plane.name[0]);
    }
}