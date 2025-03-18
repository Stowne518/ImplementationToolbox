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
    std::string sqlOutputSubDir = path + "SQL Output\\";
    std::string profileOutputDir = path + "Profiles\\";
    std::string unitCSVDir = path + "Units\\";
    std::string connStrDir = path + "ConnectionStrings\\";
    // Check for the primary directory we will use for program if not there we try to create it
    if (!std::filesystem::exists(path)) {
        // Try and create it
        if (std::filesystem::create_directory(path)) {
            // Create sql output sub directory so we can save out output files there.
            std::filesystem::create_directory(sqlOutputSubDir);
            // Create profile sub directories
            std::filesystem::create_directory(profileOutputDir);
            // Create unit directory to store unit csv files
            std::filesystem::create_directory(unitCSVDir);
            // Create connection string directory
            std::filesystem::create_directory(connStrDir);
            return true;
        }
        else {
            return false;
        }
    }
    // Create subdirectories if they don't exists in the working dir
    else if (!std::filesystem::exists(sqlOutputSubDir)) {
        std::filesystem::create_directory(sqlOutputSubDir);
    }
    else if (!std::filesystem::exists(profileOutputDir)) {
        std::filesystem::create_directory(profileOutputDir);
    }
    else if (!std::filesystem::exists(unitCSVDir)) {
        std::filesystem::create_directory(unitCSVDir);
    }
    else if (!std::filesystem::exists(connStrDir)) {
        std::filesystem::create_directories(connStrDir);
    }
    // If they all already exists we also return true
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
        ImGui::TextWrapped("- Added new directories for SQL connection strings and unit csv files in the working directory.");
    }
    ImGui::SeparatorText("RMS/JMS");
    if (ImGui::CollapsingHeader("Generic Export Generator")) {
        ImGui::Spacing();
        ImGui::TextWrapped("- Began work on sorting table and editing field order values. Not functional yet.");
        ImGui::SeparatorText("Known Bugs");
        ImGui::TextWrapped("- Sorting on field list and editing field order is still not functional for generic exports.");
    }
    if (ImGui::CollapsingHeader("One Button Database Refresh")) {
        ImGui::Spacing();
        ImGui::TextWrapped("- Added QOL improvements by adding check box for using the same file path in training as live.");
        ImGui::TextWrapped("- Now check that all data is filled out before allowing a script to be generated to avoid errors when running SQL.");
        ImGui::TextWrapped("- Recently tested script on a client site and it did work without error, reducing refresh time to about 10 minutes in total.");
    }
    ImGui::SeparatorText("CAD");
    if (ImGui::CollapsingHeader("Unit Import"))
    {
        ImGui::TextWrapped("- New functionality: Added module for CAD units where a CSV can be read and data can be inserted directly into a SQL database in bulk to avoid hand entering. NOTE: SQL connectivity must be connected before this will work.");
        ImGui::TextWrapped("- Hotfix: Added verification that you are connected to the cad database before allowing the insert to proceed.");
    }
    ImGui::SeparatorText("SQL");
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

void showDisabledButton(static char label[], ImVec2 size) {
    ImGui::BeginDisabled();
    ImGui::Button(label, size);
    ImGui::EndDisabled();
}