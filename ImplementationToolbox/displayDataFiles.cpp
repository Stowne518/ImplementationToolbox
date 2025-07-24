#include "genericDataImport.h"
#include "systemFunctions.h"        // Button/ComboBox functions

/// <summary>
/// Function to read in file names and display in an ImGui::ComboBox for selection
/// </summary>
/// <param name="dir">is the directory path to the unit_csv files</param>
/// <returns>selected file name</returns>
std::string displayDataFiles(std::string dir)
{
    static std::string chosenName;
    static int selectedFile = 0;
    std::vector<std::string> fileNames;
    static ImGuiTextFilter file_filter;

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

    ImGui::SetNextItemWidth(175);
    ImGui::PushID("filesBox");
    ComboBox("##FilesBox", fileNameCStr, selectedFile, file_filter);     // Pass vector of c string arrays, and state of selected item
    ImGui::PopID();

    return chosenName = fileNameCStr[selectedFile];     // Set chosenName equal to the vector with the state of selected from ComboBox function
}