#include "Sql.h"
#include "unitbuilder.h"
#include "unitImport.h"
#include "Units.h"
#include "Agencies.h"
#include "AgencyUnit.h"
#include "agencyImport.h"
#include "Group.h"
#include "Groups.h"
#include "groupImport.h"
#include "beatImport.h"

void unitBuilder(bool* p_open, Sql& sql, std::string dir)
{
    // Bool trackers to ensure each step is complete
    static bool step1 = false, step2 = false, step3 = false, step4 = false, step5 = false;

    // Initialize objects for each step
    Units units;
    AgencyUnit agency;
    Agencies agencies;
    Group group;
    Groups groups;

    units.setDir(dir + "Unit\\");

	if (ImGui::CollapsingHeader("Step 1. Import Agencies", ImGuiTreeNodeFlags_DefaultOpen))
	{
        static int unitSuccess = 0;
        unitSuccess = agencyImport(sql, agencies, dir);

        if(sql._GetConnected() && sql._GetDatabase() == "cad" && !step1)
        {
            if (unitSuccess > 0)
            {
                step1 = true;
            }
        }
        if (ImGui::Button("Complete Step 1"))
            step1 = true;
	}
    if (step1)
    {
        if (ImGui::CollapsingHeader("Step 2. Import Groups"))
        {
            static int groupSuccess = 0;
            groupSuccess = groupImport(sql, groups, dir);
            if (groupSuccess > 0)
                step2 = true;
            if (ImGui::Button("Complete Step 2"))
                step2 = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 2. Import Groups");
        ImGui::EndDisabled();
    }
    if (step1 && step2)
    {
        if (ImGui::CollapsingHeader("Step 3. Import Beats - Moving to map import")) // Beats move to Geotab1 import
        {
            // Add code to import beats
            // Need smarter import options here
            // Beat info comes from shape files that we get from GIS
            // Current idea is to read in column names first, then have a drag-n-drop mapping option to match client columns to our geotab1 columns before importing data
            // After columns are confirmed we enable data mapping/importing
            // Go to imgui_demo.cpp line 2434 for example of drag/drop swapping
            /*beatImport(beats, dir);*/
            if (ImGui::Button("Complete Step 3"))
                step3 = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 3. Import Beats - moving to map import");
        ImGui::EndDisabled();
    }
    if (step1 && step2 && step3)
    {
        if (ImGui::CollapsingHeader("Step 4. Import Stations - moving to map import"))
        {
            // Add function to import stations
            if (ImGui::Button("Complete Step 4"))
                step4 = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 4. Import Stations - moving to map import"); // Move stations to Geotab1 import
        ImGui::EndDisabled();
    }
    if(step1 && step2 && step3 && step4)
    {
        if (ImGui::CollapsingHeader("Step 5. Import Units"))
        {
            unitInsert(sql, units);
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 5. Import Units");
        ImGui::EndDisabled();
    }
}

void DrawGreenCheckMark(ImDrawList* draw_list, ImVec2 pos, float size) {
    float thickness = size / 6.0f;
    ImVec2 p1 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.55f);
    ImVec2 p2 = ImVec2(pos.x + size * 0.45f, pos.y + size * 0.85f);
    ImVec2 p3 = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.15f);
    draw_list->AddLine(p1, p2, IM_COL32(0, 200, 0, 255), thickness);
    draw_list->AddLine(p2, p3, IM_COL32(0, 200, 0, 255), thickness);
}

void DrawRedXMark(ImDrawList* draw_list, ImVec2 pos, float size) {
    float thickness = size / 6.0f;
    ImVec2 p1 = ImVec2(pos.x - size * 0.30f, pos.y + size * 0.45f);
    ImVec2 p2 = ImVec2(pos.x + size * 0.15f, pos.y - size * 0.45f);
    ImVec2 p3 = ImVec2(pos.x - size * 0.30f, pos.y - size * 0.45f);
    ImVec2 p4 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.45f);
    draw_list->AddLine(p1, p2, IM_COL32(200, 0, 0, 255), thickness);
    draw_list->AddLine(p3, p4, IM_COL32(200, 0, 0, 255), thickness);
}