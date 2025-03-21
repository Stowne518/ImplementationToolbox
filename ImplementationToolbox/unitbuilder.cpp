#include "Sql.h"
#include "unitbuilder.h"
#include "unitImport.h"
#include "Units.h"
#include "Agencies.h"
#include "AgencyUnit.h"
#include "agencyImport.h"

void unitBuilder(bool* p_open, Sql& sql, std::string dir)
{
    // Bool trackers to ensure each step is complete
    static bool step1 = false, step2 = false, step3 = false, step4 = false, step5 = false;

    // Initialize objects for each step
    Units units;
    AgencyUnit agency;
    Agencies agencies;

    units.setDir(dir + "Unit\\");

	if (ImGui::CollapsingHeader("Step 1. Import Agencies", ImGuiTreeNodeFlags_DefaultOpen))
	{
        agencyImport(sql, agencies, dir);

        // Check agency table to see if any agencies have been added yet to advance to step 2
        if(sql._GetConnected() && sql._GetDatabase() == "cad" && !step1)
        {
            if (sql.returnRecordCount("agency", "agencycode") > 0)
            {
                step1 = true;
            }
        }
	}
    if (step1)
    {
        if (ImGui::CollapsingHeader("Step 2. Import Groups"))
        {
            // Add code to import groups
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
        if (ImGui::CollapsingHeader("Step 3. Import Beats"))
        {
            // Add code to import beats
            if (ImGui::Button("Complete Step 3"))
                step3 = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 3. Import Beats");
        ImGui::EndDisabled();
    }
    if (step1 && step2 && step3)
    {
        if (ImGui::CollapsingHeader("Step 4. Import Stations"))
        {
            // Add function to import stations
            if (ImGui::Button("Complete Step 4"))
                step4 = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::CollapsingHeader("Step 4. Import Stations");
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

/// <summary>
/// Function to read in file names and display in an ImGui::ComboBox for selection
/// </summary>
/// <param name="dir">is the directory path to the unit_csv files</param>
/// <returns>selected file name</returns>
std::string displayFiles(std::string dir) {
    static std::string chosenName;
    std::vector<std::string> fileNames;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        std::string filename = entry.path().filename().string();
        size_t pos = filename.find(".csv");
        if (pos != std::string::npos) {
            std::string displayName = filename;
            fileNames.push_back(displayName);
        }
    }

    // Convert std::vector<std::string> to array of const char* for ImGui display
    std::vector<const char*> fileNameCStr;
    for (const auto& name : fileNames) {
        fileNameCStr.push_back(name.c_str());
    }

    // Display the Combo box if we find a profile has been created otherwise we show text instead
    static int currentItem = 0;
    ImGui::SetNextItemWidth(250);
    if (!fileNameCStr.empty() && ImGui::BeginCombo(("##Select unit csv" + dir).c_str(), fileNameCStr[currentItem])) {
        for (int n = 0; n < fileNameCStr.size(); n++) {
            bool isSelected = (currentItem == n);
            if (ImGui::Selectable(fileNameCStr[n], isSelected)) {
                currentItem = n;
                chosenName = fileNameCStr[n];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        // End Combo Box for profile drop down
        ImGui::EndCombo();
    }
    else if (fileNameCStr.empty())
        ImGui::TextWrapped("No CSV files found. Place a .csv file in %s", dir.c_str());

    // Return the currently selected name in the combo box
    return chosenName;
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