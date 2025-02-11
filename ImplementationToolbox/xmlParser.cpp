/*
This will be used to read an XML file and create an editor that dynamically adjusts to the XML file and allows for easy reading and modifying of the XML files loaded into it.
*/
#include <vector>
#include <sstream>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "imgui.h"

/*
    Function used to get a list of XML files in the temp directory and display them in a Combo box that we then pass to import and bring the data in.
*/
std::string displayFiles(const std::string& directory) {
    static std::string chosenName;
    std::vector<std::string> fileNames;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        std::string filename = entry.path().filename().string();
        size_t pos = filename.find(".xml");
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
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select a file", fileNameCStr[currentItem])) {
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
        ImGui::TextWrapped("No profiles found. Use Export profile first to generate a profile text file.");

    // Return the currently selected name in the combo box
    return chosenName;
}

void importXMLData(std::string fileDir, std::string file) {
    std::vector<std::string> tags;
    std::ifstream readXml;

    readXml.open(fileDir + file);

    if (!readXml)
        ImGui::Text("Could not open file!");
    else {
        while (readXml) {
            std::string line;
            std::getline(readXml,line);
            int spos = line.find('<'), npos = line.find('>'), endTag = line.find('/');
            if (spos >= 0 && endTag == -1) {
                std::string tagName = line.substr(spos + 1, npos - 1);
                tags.push_back(tagName);
            }
        }
        ImGui::BeginChild("##TestGroups");
        // print tag names for testing
        for (int i = 0; i < tags.size(); i++)
        {
            ImGui::BeginGroup();
            ImGui::Text(tags[i].c_str());
            ImGui::Text("Example 1"); ImGui::SameLine(); ImGui::Text("Example 2"); ImGui::SameLine(); ImGui::Text("Example 3");
            ImGui::EndGroup();
        }

        ImGui::EndChild();
    }
}

void xmlParser(const std::string& fileDir) {
    /*
        Funciton plan
        1. Detect any file with a .xml tag using displayFiles()
        2. Present options in a combo list
        3. When one is selected have a button that when pressed will read in the file and build a UI based on groups/field optionsx
        4. Read in current values of xml tags and put them into input boxes with current values but allow for editing and then saving
    */
    ImGui::Text("Select an XML to view/edit."); ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    std::string fileName = displayFiles(fileDir);
    ImGui::SameLine();
    // if(ImGui::Button("Load XML"))
        importXMLData(fileDir, fileName);
}
