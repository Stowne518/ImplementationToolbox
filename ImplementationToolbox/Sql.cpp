#include <string>
#include "Sql.h"
#include "sqlConnectionHandler.h"

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

void Sql::DisplaySqlConfigWindow(bool* p_open) {
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

int Sql::returnRecordCount(std::string table, std::string column) {
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column);
}
// Overload to count records for one specific value intead of just all records in one column
int Sql::returnRecordCount(std::string table, std::string column, std::string value) {
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column, value);
}