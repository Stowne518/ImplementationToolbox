#pragma once
#pragma comment(lib, "D3dx9")

#include "imgui.h"
#include "readFieldList.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <D3dx9tex.h>


/// <summary>
/// Function used to check the main directory path exists for us to save various files into it. Returns true if it is able to create the directory or already exists. Returns false if it can't create it and doesn't exist
/// </summary>
/// <param name="path">= the directory path that we check to save profiles to</param>
/// <returns></returns>
bool createDirectory(std::string& path) {
    // Check for the primary directory we will use for program if not there we try to create it
    if (!std::filesystem::exists(path)) {
        // Try and create it
        if (std::filesystem::create_directory(path)) {
            return true;
        }
        else {
            return false;
        }
    }
    // If it already exists we also return true
    else {
        return true;
    }
 }

// Function to check in the list of generic export fields were correctly read in.
bool genericFieldCheck() {
    std::vector<std::string> fieldTest;

    fieldTest = readFieldList();
    if (fieldTest.empty()) {
        return false;
    }
    else
        return true;

    // Return false by default
    return false;
}

// Display text as either green or red by passing true/false respectively
void DisplayColoredText(const char* text, bool isGreen) {
    ImVec4 color = isGreen ? ImVec4(0.0f, 0.75f, 0.0f, 1.0f) : ImVec4(0.75f, 0.0f, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

void displayUpdates() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, (ImVec2(0, 5)));
    if (ImGui::CollapsingHeader("General")) {
        ImGui::TextWrapped("- began work on direct SQL connectivity within the app. Much work to be done yet, but the plan is to have that available so we can place the tool on site and have it integrate directly with SQL and the fileshare.");
        ImGui::TextWrapped("- Updated the health check window to track SQL connectivity.");
    }
    if (ImGui::CollapsingHeader("Generic Export Generator")) {
        ImGui::Spacing();
        ImGui::TextWrapped("- Completed significant work on bug fixes for the field ordering when  loading/saving profiles. There was a known bug when profiles were loaded the order reset to match the order the fields appear in for the list, not the order they were selected in. This is FIXED.");
        ImGui::TextWrapped("- Changed the formatting of some options  in the file options and field options tabs, tried to make it more clear what options go with what items. Also stopped hiding options, and show them always but they will be disabled until the correct option is selected.");
        ImGui::TextWrapped("- Moved field options and field selection to the same window for easier configuring when including additional fields.");
        ImGui::TextWrapped("- Modified the SQL output window so it's smaller and the text itself is only visible for part of the window. This keeps the copy to clipboard and close buttons visible at all times.");
        ImGui::TextWrapped("- Added a check to see if field options have been added before allowing them to be selected in the field list for locations and pin fields.");
        ImGui::SeparatorText("Known Bugs");
    }
    if (ImGui::CollapsingHeader("One Button Database Refresh")) {
        ImGui::Spacing();
        ImGui::TextWrapped("- Modified the SQL output window so it's smaller and the text itself is only visible for part of the window. This keeps the copy to clipboard and close buttons visible at all times.");
    }
    if (ImGui::CollapsingHeader("SQL Query Builder")) {
        ImGui::Spacing();
        ImGui::TextWrapped("No updates this version.");
        ImGui::Spacing();
    }
    // End spacing style
    ImGui::PopStyleVar();
}

// Function to display a color picker and change the background color
void colorPickerWithBackgroundChange(ImVec4& bgColor) {
    ImGui::Begin("Select a Background color");

    // Array of pleasing colors
    static ImVec4 colors[] = {
        ImVec4(0.9f, 0.1f, 0.1f, 1.0f), // Red
        ImVec4(0.1f, 0.9f, 0.1f, 1.0f), // Green
        ImVec4(0.1f, 0.1f, 0.9f, 1.0f), // Blue
        ImVec4(0.9f, 0.9f, 0.1f, 1.0f), // Yellow
        ImVec4(0.9f, 0.1f, 0.9f, 1.0f), // Magenta
        ImVec4(0.1f, 0.9f, 0.9f, 1.0f), // Cyan
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f)  // Gray
    };

    // Display color buttons
    for (int i = 0; i < IM_ARRAYSIZE(colors); ++i) {
        ImGui::PushID(i); // Ensure each button has a unique ID
        if (ImGui::ColorButton("##color", colors[i], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(50, 50))) {
            bgColor = colors[i];
        }
        ImGui::PopID();
        if ((i + 1) % 3 != 0) ImGui::SameLine();
    }

    ImGui::End();
}

// See imgui wiki for function example: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool displayCentralSquareLogo(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height) {
    PDIRECT3DTEXTURE9 texture;
    HRESULT hr = D3DXCreateTextureFromFileA(g_pd3dDevice, filename, &texture);
    if (hr != S_OK) {
        std::cerr << "Failed to load image. HRESULT:" << hr << std::endl;
        return false;
    }

    // Retrieve description of texture surfgace so we can access the size
    D3DSURFACE_DESC my_image_desc;
    texture->GetLevelDesc(0, &my_image_desc);
    *out_texture = texture;
    *out_width = (int)my_image_desc.Width;
    *out_height = (int)my_image_desc.Height;
    return true;
}
