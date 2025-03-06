#include <string>
#include <fstream>
#include "Sql.h"
#include "sqlConnectionHandler.h"
#include <filesystem>

bool Sql::requiredInfo(std::string s, std::string db, std::string u, std::string p) {
	// Should check if any field is blank on the SQL Connection form and if so not allow the connection attempt
	if (s == "" || db == "" || u == "" || p == "")
		return false;

	return true;
}

bool Sql::executeQuery(std::string query) {
    if (!SqlConnectionHandler::executeUpdateQuery(_GetConnectionString(), query)) {
        return false;
    }
    else
        return true;
}

void Sql::DisplaySqlConfigWindow(bool* p_open, std::string dir) {
    // SQL Connection string variables for connection test
    static char serverNameBuffer[256] = { }, databaseNameBuffer[256] = { }, usernameBuffer[256] = { }, passwordBuffer[256] = { };
    static std::string sqlServerName, databaseName, sqlUsername, sqlPassword;
    static bool integratedSecurity = false;
    static bool connection_attempted = false;   // Have we tried to connect yet? - Use this to determine what to display in the health check window
    static bool connection_success = false;     // Did we succeed in our attempt to connect to SQL?
    static float fieldLen = 101;                // Length to display connection string boxes
    ImGui::OpenPopup("SQL Connection Settings");

    if (ImGui::BeginPopupModal("SQL Connection Settings", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("SQL Connection String", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Server name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##server", serverNameBuffer, IM_ARRAYSIZE(serverNameBuffer));
            ImGui::TableNextColumn();
            ImGui::Text("Enter database name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##database", databaseNameBuffer, IM_ARRAYSIZE(databaseNameBuffer));
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Username:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##user", usernameBuffer, IM_ARRAYSIZE(usernameBuffer));
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Password:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##password", passwordBuffer, IM_ARRAYSIZE(passwordBuffer));

            // End SQL entry table
            ImGui::EndTable();
        }

        // Copy char array buffers to strings
        _SetSource(std::string(serverNameBuffer));
        _SetDatabase(std::string(databaseNameBuffer));
        _SetUsername(std::string(usernameBuffer));
        _SetPassword(std::string(passwordBuffer));

        // Check that field have been filled out
        bool checkrequiredInfo = requiredInfo(_GetSource(), _GetDatabase(), _GetUsername(), _GetPassword());

        if (!_GetConnected() && checkrequiredInfo) {
            if (ImGui::Button("Test Connection", ImVec2(120, 60))) {
                _SetConnected(SqlConnectionHandler::sqlConnectionHandler(_GetSource(), _GetDatabase(), _GetUsername(), _GetPassword()));
                if(_GetConnected())
                    _SetConnectionString();
                _SetConnectionAttempt();
            }
        }
        else if (!_GetConnected() && !checkrequiredInfo) {
            ImGui::BeginDisabled();
            ImGui::Button("Test Connection", ImVec2(120, 60));
            ImGui::EndDisabled();
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("SQL Connected!", ImVec2(120, 60));
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120, 60))) {
            *p_open = false;
            ImGui::CloseCurrentPopup();
        }

        // End SQL Connection popup modal window
        ImGui::EndPopup();
    }
}

void Sql::saveConnString(std::string dir, std::string name) {
    std::ofstream connStr;
    connStr.open(dir + name + ".str");

    if (!connStr) {
        return;
    }
    else {
        connStr << _GetSource() << std::endl;
        connStr << _GetDatabase() << std::endl;
        connStr << _GetUsername() << std::endl;
        connStr << _GetPassword();
    }
}

std::string Sql::displayConnectionName(const std::string& directory) {
    static std::string chosenName;
    std::vector<std::string> fileNames;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        std::string filename = entry.path().filename().string();
        size_t pos = filename.find(".str");
        if (pos != std::string::npos) {
            std::string displayName = filename.substr(0, pos);
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
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select a connetion", fileNameCStr[currentItem])) {
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
        ImGui::TextWrapped("No connection string files found. Use Save Connection first to generate a connection string file.");

    // Return the currently selected name in the combo box
    return chosenName;
}

void Sql::readConnString(const std::string dir) {
    ImGui::Text("Select a saved connection string"); ImGui::SameLine(); 
    std::string seleted_file = Sql::displayConnectionName(dir);
    std::ifstream connStr;
    
}

int Sql::returnRecordCount(std::string table, std::string column) {
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column);
}
// Overload to count records for one specific value intead of just all records in one column
int Sql::returnRecordCount(std::string table, std::string column, std::string value) {
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column, value);
}