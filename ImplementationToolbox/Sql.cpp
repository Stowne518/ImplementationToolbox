#include <string>
#include <fstream>
#include "Sql.h"
#include "sqlConnectionHandler.h"
#include "AppLog.h"
#include <filesystem>
#include <iostream>

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

void Sql::DisplaySqlConfigWindow(bool* p_open, std::string dir, AppLog& log) {
    // SQL Connection string variables for connection test
    static char serverNameBuffer[256] = { }, databaseNameBuffer[256] = { }, usernameBuffer[256] = { }, passwordBuffer[256] = { };
    static std::string sqlServerName, databaseName, sqlUsername, sqlPassword;
    static bool integratedSecurity = false;
    static bool connection_attempted = false;   // Have we tried to connect yet? - Use this to determine what to display in the health check window
    static bool connection_success = false;     // Did we succeed in our attempt to connect to SQL?
    static bool integrated_security = false;    // Offer option to use integrated security within SQL instead of username/password
    static int server_type = 0;                 // Offer option of server type to include (0 == MSSQL, 1 == SQLEXPRESS)
    static float fieldLen = 101;                // Length to display connection string boxes
    bool checkrequiredInfo = false;             // Check that we have all info filled out
    static bool changed = false;

    // Log sql information
	log.logStateChange("integrated_security", integrated_security);
	log.logStateChange("connection_attempted", connection_attempted);
	log.logStateChange("connection_success", connection_success);
	log.logStateChange("server_type", server_type);
	log.logStateChange("checkrequiredInfo", checkrequiredInfo);

    ImGui::OpenPopup("SQL Connection Settings");

    if (ImGui::BeginPopupModal("SQL Connection Settings", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        readConnString(dir, serverNameBuffer, databaseNameBuffer, usernameBuffer, passwordBuffer, checkrequiredInfo);
        if (ImGui::CollapsingHeader("Server Type"))
        {
            ImGui::RadioButton("MSSQL", &server_type, 0);
            ImGui::SetItemTooltip("Select this option if you're connecting to a Microsoft SQL Server.");
            ImGui::SameLine();
            ImGui::RadioButton("SQLEXPRESS", &server_type, 1);
            ImGui::SetItemTooltip("Select this option if you're connecting to a SQLEXPRESS edition of SQL.");
        }
        if (ImGui::BeginTable("SQL Connection String", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Server name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##server", serverNameBuffer, IM_ARRAYSIZE(serverNameBuffer));
            if (serverNameBuffer != _GetSource()) { log.AddLog("[INFO] SQL Server Name changed to: %s from: %s\n", serverNameBuffer, _GetSource()); changed = true; }               // Did we change server name
            ImGui::TableNextColumn();
            ImGui::Text("Enter database name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##database", databaseNameBuffer, IM_ARRAYSIZE(databaseNameBuffer));
            if (databaseNameBuffer != _GetDatabase()) { log.AddLog("[INFO] SQL Server Database changed to: %s from: %s\n", databaseNameBuffer, _GetDatabase()); changed = true; }           // Did we change database name
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Username:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##user", usernameBuffer, IM_ARRAYSIZE(usernameBuffer));
            if (usernameBuffer != _GetUsername()) { log.AddLog("[INFO] SQL Server Username changed to: %s from: %s\n", usernameBuffer, _GetUsername()); changed = true; }               // Did we change username
            ImGui::TableNextColumn();
            ImGui::Text("Enter SQL Password:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##password", passwordBuffer, IM_ARRAYSIZE(passwordBuffer));
            if (passwordBuffer != _GetPassword()) { log.AddLog("[INFO] SQL Server Password changed to: %s from: %s\n", passwordBuffer, _GetPassword()); changed = true; }               // Did we change database name

            // End SQL entry table
            ImGui::EndTable();
        }

        /* 
        Found bug where using the new quick connect feature to connect and then later coming back and opening SQL connection screen would cause crashing
        This was due to us updating values with these buffers as soon as the screen is open, wasn't an issue when we didn't have that option, but it was blanking out
        the db name and then in generic import it would crash trying to use the current db name and finding it blank after opening connection screen.
        Moved these inside of test connection so it wouldn't break just from opening that.*/

        // Set all obj variables to buffer values
        /*_SetSource(std::string(serverNameBuffer));
        _SetDatabase(std::string(databaseNameBuffer));
        _SetUsername(std::string(usernameBuffer));
        _SetPassword(std::string(passwordBuffer));*/

        // Check that field have been filled out
        if(!checkrequiredInfo)
            checkrequiredInfo = requiredInfo(serverNameBuffer, databaseNameBuffer, usernameBuffer, passwordBuffer);

        if (!_GetConnected() && checkrequiredInfo || changed) {
            if (ImGui::Button("Test Connection", ImVec2(120, 60))) {
                // Set all obj variables to buffer values
                _SetSource(std::string(serverNameBuffer));
                _SetDatabase(std::string(databaseNameBuffer));
                _SetUsername(std::string(usernameBuffer));
                _SetPassword(std::string(passwordBuffer));
				log.AddLog("[INFO] Attempting to connect to SQL Server: %s\n", _GetSource().c_str());
                _SetConnected(SqlConnectionHandler::sqlConnectionHandler(_GetSource(), _GetDatabase(), _GetUsername(), _GetPassword()));
                if(_GetConnected())
                {
					log.AddLog("[INFO] SQL Connection Succeeded!\n");
                    _SetConnectionString();
				}
                else
                {
                    log.AddLog("[ERROR] SQL Connection Failed!\n");
                }
                _SetConnectionAttempt();
                changed = false;
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
        if(checkrequiredInfo && _GetConnected())
        {
            if (ImGui::Button("Save Connection String", ImVec2(248, 30))) {
                if(server_type == 0)
                {
                    saveConnString(dir, _GetSource(), log);
                }
                else if (server_type == 1)
                {
                    saveConnString(dir, _GetSource(), server_type);
                }
            }
        }
        if (!checkrequiredInfo || !_GetConnected())
        {
            ImGui::BeginDisabled();
            ImGui::Button("Save Connection String", ImVec2(248, 30));
            ImGui::SetItemTooltip("Fill out all fields and test connection to enable saving.");
            ImGui::EndDisabled();
        }

        // End SQL Connection popup modal window
        ImGui::EndPopup();
    }
}

void Sql::saveConnString(std::string dir, std::string name, AppLog& log) {
    std::ofstream connStr;
    try
    {
        connStr.open(dir + name + "_" + _GetDatabase() + ".str");
        log.AddLog("Saving connection string to: %s_%s.str\n", (dir + name).c_str(), _GetDatabase());
    }    
    catch (std::exception& e)
    {
        log.AddLog("Error saving file: %s\n", e.what());
        return;
    }

    if (!connStr) {
        _SetSavedString(false);
        return;
    }
    else {
        connStr << _GetSource() << std::endl;
        connStr << _GetDatabase() << std::endl;
        connStr << _GetUsername() << std::endl;
        connStr << _GetPassword();
        log.AddLog("Connection string successfully saved to: %s_%s.str\n", dir + name, _GetDatabase());
		_SetSavedString(true);
    }
}

void Sql::saveConnString(std::string dir, std::string name, int servertype) {
    int serverlen = _GetSource().length() - 11;     // Server name length, minus 11 characters to drop off SQLEXPRESS\ //
    std::ofstream connStr;
    try
    {
        connStr.open(dir + name.substr(11, serverlen) + "_" + _GetDatabase() + ".str");
        std::cout << "Saving connection string to: " << dir + name.substr(11, serverlen) + "_" << _GetDatabase() << ".str" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Error saving file: " << e.what() << std::endl;
        return;
    }

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
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select a connetion", fileNameCStr[currentItem], ImGuiComboFlags_None)) {
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
    {
        ImGui::SetNextItemWidth(203);
        ImGui::TextWrapped("No saved connetion strings.");
    }

    // Return the currently selected name in the combo box
    return chosenName;
}

void Sql::readConnString(const std::string dir, char* source, char* db, char* un, char* pw, bool req) {
    ImGui::SetNextItemWidth(203);
    std::string selected_file = Sql::displayConnectionName(dir);
    const int FIELD_LEN = 256;
    char source_buff[FIELD_LEN], db_buff[FIELD_LEN], un_buff[FIELD_LEN], pw_buff[FIELD_LEN];
    std::ifstream connStr;
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        connStr.open(dir + selected_file + ".str");

        // Make sure we opened the file
        if (!connStr)
        {
            return;
        }
        else
        {
            std::string line;
            if (std::getline(connStr, line))
            {
                strncpy_s(source, FIELD_LEN, line.c_str(), FIELD_LEN);                  // Read source value and copy to buffer
            }
            if (std::getline(connStr, line))
            {
                strncpy_s(db, FIELD_LEN, line.c_str(), FIELD_LEN);                      // Read database value and copy to buffer
            }
            if (std::getline(connStr, line))
            {
                strncpy_s(un, FIELD_LEN, line.c_str(), FIELD_LEN);                      // Read username value and copy to buffer
            }
            if (std::getline(connStr, line))
            {
                strncpy_s(pw, FIELD_LEN, line.c_str(), FIELD_LEN);                      // Read password value
            }
            connStr.close();
			_SetSavedString(true);                                                      // Update saved string status
        }
    }
}

// Overloaded to take only a path to a pre-existing file
// Used to quick load the previously saved/loaded file that we store to the settings file
bool Sql::readConnString(std::string filepath)
{
    std::ifstream loadConnection;

    loadConnection.open(filepath);
    std::string line;
    if(std::getline(loadConnection, line))
	{
		_SetSource(line);
	}
	if (std::getline(loadConnection, line))
	{
		_SetDatabase(line);
	}
	if (std::getline(loadConnection, line))
	{
		_SetUsername(line);
	}
	if (std::getline(loadConnection, line))
	{
		_SetPassword(line);
	}
	loadConnection.close();
	_SetConnectionString();     // Build connection string with new values
	_SetConnected(SqlConnectionHandler::sqlConnectionHandler(_GetSource(), _GetDatabase(), _GetUsername(), _GetPassword()));    // Attempt to connect to SQL
	if (_GetConnected())
	{
		return true;
	}
	else
	{
		return false;
	}
}

int Sql::returnRecordCount(std::string table, std::string column) 
{
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column);
}

// Overload to count records for one specific value intead of just all records in one column
int Sql::returnRecordCount(std::string table, std::string column, std::string value) 
{
    int count;
    return count = SqlConnectionHandler::getRecordCount(_GetConnectionString(), _GetDatabase(), table, column, value);
}

std::vector<std::string> Sql::getTableColumns(std::string connStr, std::string table)
{
    std::vector<std::string> table_columns = SqlConnectionHandler::getColumns(connStr, table);
    return table_columns;
}

int Sql::returnTableCount(std::string connStr)
{
	int count;
	return count = SqlConnectionHandler::getTableCount(connStr, _GetDatabase());
}

std::vector<std::string> Sql::getTableNames(std::string connStr, std::string database)
{
	std::vector<std::string> table_names = SqlConnectionHandler::getTables(connStr, database);
	return table_names;
}