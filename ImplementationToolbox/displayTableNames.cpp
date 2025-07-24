#include "genericDataImport.h"
#include "systemFunctions.h"    // Button/Combobox functions
#include "Sql.h"

/// <summary>
/// Creates a combo box using a list of tables pulled from the SQL database to select where to import data to.
/// </summary>
/// <param name="sql"></param>
/// <returns>string containining the selected table name</returns>
std::string displayTableNames(Sql& sql)
{
    // DONE: add keyboard capture to get a letter while combobox is active, then move the displayed position to that letter. use ImGui to see if ItemIsActive, if so start an IO monitor, when a key is pressed we do a find in the list and maybe a get position for the value in the box -- solved with text filter
    static std::vector<std::string> tableNames;
    static int selectedTable = 0;
    static ImGuiTextFilter filter;
    static std::string tmp_db = "";
    static std::string tmp_source = "";
    if (tableNames.empty() || tmp_db != sql._GetDatabase() || tmp_source != sql._GetSource())           // update tables if the connection string changes
    {
        tableNames = sql.getTableNames(sql._GetConnectionString(), sql._GetDatabase());
        tmp_db = sql._GetDatabase();
        tmp_source = sql._GetSource();
    }
    static std::string chosenName;
    // Convert std::vector<std::string> to array of const char* for ImGui display
    std::vector<const char*> tableNameCStr;
    for (const auto& name : tableNames) {
        tableNameCStr.push_back(name.c_str());
    }

    // Display the Combo box if we find sql tables otherwise we show text instead
    ImGui::SetNextItemWidth(175);
    ImGui::PushID("tableBox");
    ComboBox("##Tables", tableNameCStr, selectedTable, filter);
    ImGui::PopID();
    return chosenName = tableNameCStr[selectedTable];
}