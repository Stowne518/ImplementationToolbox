// This program will read in a pre-formatted CSV and popluate a table that can be edited, then it will send an insert query to SQL for each unit row that is present on the table
#include "Sql.h"
#include "unitImport.h"
#include "Units.h"
#include "Unit.h"
#include <iostream>

// Forward declaration
void DrawGreenCheckMark(ImDrawList* draw_list, ImVec2 pos, float size);

// Send current SQL class in use, and file directory pointed directly to the unit_csv files
void unitInsert(bool* p_open, Sql& sql, Units& units) {
    std::string dir = units.getDir();
    static std::vector<std::string> cols;
    static std::vector<std::string> rows;
    static std::vector<Unit> unit;
    // buffers for reading unit data into
    static char unitcode[300][7];
    static char type[300][5];
    static char agency[300][5];
    static bool imported = false;

    ImGui::Text("Select a file to import: "); ImGui::SameLine(); 
    std::string fileName = displayFiles(dir);
    units.setFileName(fileName);
    ImGui::SameLine();

    if(!imported)
        if (ImGui::Button("ImportCSV"))
            imported = true;
    if (imported) {
        ImGui::BeginDisabled();
        ImGui::Button("ImportCSV");
        ImGui::EndDisabled();
    }

    if(imported){
        static bool read = false;
        // Only read in imported data once
        while (!read){
            // Read in columns from CSV
            readColumns(units);
            // Read in row data from CSV
            readRows(units);
            // Read in column data to vector from Units class
            cols = units.getCols();
            // Read in row data to local vector from Units class
            rows = units.getRows();
            // Parse row data into data elements in Unit class
            unit = buildUnits(units);
            // Populate buffer fields for editing later
            for (int i = 0; i < unit.size(); i++)
            {
                strncpy_s(unitcode[i], unit[i].getUnitCode(), sizeof(unitcode[i]));
                strncpy_s(type[i], unit[i].getType(), sizeof(type[i]));
                strncpy_s(agency[i], unit[i].getAgency(), sizeof(agency[i]));
            }
            // Only read fields once
            read = true;
        }
        
        // Build table for each element found in Unit class and make it editable
        if(ImGui::BeginTable("Imported Units", cols.size(), ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            // Start with header row
            ImGui::TableSetupColumn("Unitcode", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Agency", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int i = 0; i < unit.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::InputText(("##unitcode%i" + std::to_string(i)).c_str(), unitcode[i], IM_ARRAYSIZE(unitcode[i]));
                ImGui::TableSetColumnIndex(1);
                ImGui::InputText(("##type%i" + std::to_string(i)).c_str(), type[i], IM_ARRAYSIZE(type[i]));
                ImGui::TableSetColumnIndex(2);
                ImGui::InputText(("##agency%i" + std::to_string(i)).c_str(), agency[i], sizeof(agency[i]));

                // If any update is made to values update the respective object
                unit[i].setUnitCode(unitcode[i]);
                unit[i].setType(type[i]);
                unit[i].setAgency(agency[i]);
            }
            // End imported units table
            ImGui::EndTable();
        }
        static ImVec2 text_box_pos;
        static bool validOSUsername = false;
        bool sqlConnected = sql._GetConnected();
        static char username[20], buffer[1000];
        static int user_count;
        static std::vector<std::string> results;
        // DONE: Write sql query to return int count of record results
        ImGui::Text("Enter the username to add these units with: "); ImGui::SameLine(); 
        ImGui::SetNextItemWidth(150);
        if (!sqlConnected) {
            ImGui::BeginDisabled();
            ImGui::InputText("##OSUser", username, sizeof(username));
            ImGui::SameLine(); ImGui::Button("Check Username in CAD");
            text_box_pos = ImGui::GetItemRectMax(); // Get the position of the button

            ImGui::EndDisabled();
        }
        if (sqlConnected) {
            ImGui::InputText("##OSUser", username, IM_ARRAYSIZE(username));
            ImGui::SameLine();
            if (ImGui::Button("Check Username in CAD")) {
                user_count = sql.returnRecordCount("system1", "userid", username);
            }
            text_box_pos = ImGui::GetItemRectMax(); // Get the position of the text box

        }
        text_box_pos.x += 5.0f; // Adjust the position to place the check mark next to the text box
        text_box_pos.y -= 20.0f;
        if (validOSUsername) {  
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            float size = 20.0f; // Adjust size as needed
            DrawGreenCheckMark(draw_list, text_box_pos, size);
        }
        if (!validOSUsername && sqlConnected) { 
            // Get count of users when checking for user existence
            // If it's more than one we found 2 or more users with the same name (shouldn't be possible, but who knows).
            if (user_count > 1) {
                validOSUsername = false;
            }
            // If no results come back we know user isn't valid in DB
            else if (user_count < 1) {
                validOSUsername = false;
            }
            // If we only get 1 record returned we know the user exits and is unique in the DB
            else {
                validOSUsername = true;
            }
        }
        if (!sql._GetConnected() || !validOSUsername) {
            ImGui::BeginDisabled();
            ImGui::Button("SQL Insert");
            ImGui::EndDisabled();
        }
        if (sql._GetConnected() && validOSUsername) {
            if (ImGui::Button("SQL Insert")) {
                ImGui::OpenPopup("Verify Duplicates");
            }

            // Center window when it opens
            ImVec2 validate_center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(validate_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionMax().x * .3, ImGui::GetContentRegionMax().y * .17));
            if (ImGui::BeginPopupModal("Verify Duplicates", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextWrapped("CAD does not allow duplicated units. Any attempt to import duplicate units here will return a fail and no insertion will be made for that unit.");
                if (ImGui::Button("Acknowledge")) {
                    insertSql(sql, unit, &results, username);
                    ImGui::CloseCurrentPopup();
                } ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                // End confirmation popup modal
                ImGui::EndPopup();
            }

            if (ImGui::BeginTable("SqlResults", 1, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Results");
                ImGui::TableHeadersRow();
                for (int i = 0; i < results.size(); i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", results[i].c_str());
                }
                // End query results table
                ImGui::EndTable();
            }
        }
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
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select unit csv", fileNameCStr[currentItem])) {
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
        ImGui::TextWrapped("No unit files found. Place a .csv file in %s", dir.c_str());

    // Return the currently selected name in the combo box
    return chosenName;
}

/// <summary>
/// utilize SQL connetion to send insert statments for each unit added to the agency csv unit file
/// </summary>
/// <param name="sql">is a SQL Class with conneciton string info and query function</param>
/// <param name="units">is a vector container of all units built in our unitImport functions with all their information</param>
void insertSql(Sql& sql, std::vector<Unit> units, std::vector<std::string>* result, std::string username) {
    // Write query into buffer var since we store it with C style strings then convert to string and pass along to SQL        
    for (int i = 0; i < units.size(); i++) {
        char buffer[1000];
        std::string query;
        sprintf_s(buffer, "INSERT INTO %s..unit (unitcode, type, agency, adduser, addtime) VALUES ('%s','%s','%s','%s',GETDATE())", sql._GetDatabase().c_str(), units[i].getUnitCode(), units[i].getType(), units[i].getAgency(), username.c_str());
        query = buffer;
        // Check if there is already a unit with the same unit code in the system
        int unitCount = sql.returnRecordCount("unit", "unitcode", units[i].getUnitCode());
        if (unitCount > 0) {
            ImGui::Text("Query Failed: %s", query.c_str());
            result->push_back("Unit " + (std::string)units[i].getUnitCode() + " not added - detected duplicate!");
        }
        else {
            sql.executeQuery(query);
            result->push_back("Unit " + (std::string)units[i].getUnitCode() + " added successfully!");
        }
    }
}

void readColumns(Units& unit) {
    std::string dir = unit.getFileName();
    std::ifstream unitData;

    unitData.open(dir);
    std::string line;
    std::getline(unitData, line);
    std::stringstream ss(line);
    std::string token;
    // Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
    while (std::getline(ss, token, ',')) {
        unit.setColumns(token);
    }
}

void readRows(Units& unit)
{
    std::string dir = unit.getFileName();
    std::ifstream unitData;
    unitData.open(dir);
    unitData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(unitData, line)) {
        unit.setRows(line);        
    }
}

std::vector<Unit> buildUnits(Units& units) {
    std::vector<Unit> unit;
    std::vector<std::string> rows = units.getRows();
    std::string value;

    // Buffer char arrays
    char unitcode[7],
        type[5],
        agency[5];

    for (int i = 0; i < rows.size(); i++) {
        std::vector<std::string> values;
        std::stringstream ss(rows[i]);
        while (std::getline(ss, value, ',')) {
            values.push_back(value);
        }
        strncpy_s(unitcode, values[0].c_str(), IM_ARRAYSIZE(unitcode));
        strncpy_s(type, values[1].c_str(), IM_ARRAYSIZE(type));
        strncpy_s(agency, values[2].c_str(), IM_ARRAYSIZE(agency));
        unit.push_back(Unit(unitcode, type, agency));
    }

    return unit;
}

void DrawGreenCheckMark(ImDrawList* draw_list, ImVec2 pos, float size) {
    float thickness = size / 5.0f;
    ImVec2 p1 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.55f);
    ImVec2 p2 = ImVec2(pos.x + size * 0.45f, pos.y + size * 0.85f);
    ImVec2 p3 = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.15f);
    draw_list->AddLine(p1, p2, IM_COL32(0, 200, 0, 255), thickness);
    draw_list->AddLine(p2, p3, IM_COL32(0, 200, 0, 255), thickness);
}