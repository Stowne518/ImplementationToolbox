#include "genericDataImport.h"
#include "Sql.h"
#include <iostream>

void genericDataImport(Sql sql, std::string dir)
{
	static std::vector<std::string> source_columns;
	static std::vector<std::string> destination_columns;
    static std::vector<std::string> buffer_columns;
    static std::vector<std::string> data_rows;
	static std::vector<std::string> insert_rows;
	static std::string table_name;
	ImGui::Text("Select a file to import: "); ImGui::SameLine();
	std::string filepath = displayDataFiles(dir + "DataImport\\");

    static bool loaded_csv = false;
    static bool load_tables = false;
    static bool load_columns = false;
    static bool confirm_mapping = false;
    ImGui::SameLine();
	if (filepath.empty() || loaded_csv)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Load CSV");
        ImGui::SetItemTooltip("CSV loaded!");
        ImGui::EndDisabled();
	}
    else
    {
        if (ImGui::Button("Load CSV"))
        {
            getColumns(std::filesystem::path(dir + "DataImport\\" + filepath), source_columns);
            getRows(std::filesystem::path(dir + "DataImport\\" + filepath), data_rows);
            for (int i = 0; i < source_columns.size(); i++)
            {
                std::cout << "Column loaded: " << source_columns[i] << std::endl;
                buffer_columns.push_back("");
            }
            loaded_csv = true;
        }        
    }

	// Get destination columns
	if (!sql._GetConnected())
    {
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button("Load SQL Tables");
        ImGui::SetItemTooltip("Connect to a SQL database to proceed.");
        ImGui::EndDisabled();
	}
	else
	{
        if (!load_tables)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load SQL Tables"))
            {
                load_tables = true;
            }
        }
        if (load_tables)
        {
            ImGui::Text("Select a table to import to: "); ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            table_name = displayTableNames(sql);
        }
        if (load_columns)
        {
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Load Columns");
            ImGui::SetItemTooltip("Columns loaded!");
            ImGui::EndDisabled();
        }
        else if (table_name.empty())
        {
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Load Columns");
            ImGui::SetItemTooltip("Select a table to proceed.");
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::SameLine();
            if (ImGui::Button("Load Columns"))
            {
                destination_columns = sql.getTableColumns(sql._GetConnectionString(), table_name);
                for (int i = 0; i < destination_columns.size(); i++)
                {
                    std::cout << "Column loaded: " << destination_columns[i] << std::endl;
                    buffer_columns.push_back("");
                }
                load_columns = true;
            }
        }
	}
    if (ImGui::CollapsingHeader("Column Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (load_columns && !confirm_mapping && loaded_csv)
        {
            displayMappingTable(source_columns, destination_columns, buffer_columns, data_rows);
            ImGui::SameLine();
            if (ImGui::Button("Confirm Mapping", ImVec2(150, 75)))
            {
                // Confirm mapping and import data
                confirm_mapping = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel Mapping", ImVec2(150, 75)))
            {
                // Clear all vectors
                source_columns.clear();
                destination_columns.clear();
                buffer_columns.clear();
                table_name.clear();
                loaded_csv = false;
                load_tables = false;
                load_columns = false;
                confirm_mapping = false;
            }
        }
        if (confirm_mapping)
        {
            // Remove blanks from buffer vector
            for (int i = 0; i < buffer_columns.size(); i++)
            {
                if (buffer_columns[i] == "")
                {
                    buffer_columns.erase(buffer_columns.begin() + i);
                    i--;
                }
            }
            // Generate insert queries and store them in vector
            for (int i = 0; i < data_rows.size(); i++)
            {
                insert_rows = buildInsertQuery(table_name, buffer_columns, data_rows);
            }
        }
    }

	// Default return
	// return false;
}

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns) 
{
	std::ifstream dataImport;

	dataImport.open(dir);
	std::string line;
	std::getline(dataImport, line);
	std::stringstream ss(line);
	std::string token;
	// Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
	while (std::getline(ss, token, ',')) 
	{
		columns.push_back(token);
	}
}

void getRows(std::filesystem::path& dir, std::vector<std::string>& rows)
{
    std::ifstream rowData;
    rowData.open(dir);

    if (!rowData)
        return;
	// Ignore first row in CSV, this is assumed to be the column headers
    rowData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(rowData, line))
    {
        rows.push_back(line);
    }
}

/// <summary>
/// Function to read in file names and display in an ImGui::ComboBox for selection
/// </summary>
/// <param name="dir">is the directory path to the unit_csv files</param>
/// <returns>selected file name</returns>
std::string displayDataFiles(std::string dir)
{
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
        ImGui::TextWrapped("No CSV files found. Place a .csv file in %s", dir.c_str());

    // Return the currently selected name in the combo box
    return chosenName;
}

std::string displayTableNames(Sql& sql)
{
	static std::vector<std::string> tableNames;
	if (tableNames.empty())
    {
        tableNames = sql.getTableNames(sql._GetConnectionString(), sql._GetDatabase());
    }
	static std::string chosenName;
	// Convert std::vector<std::string> to array of const char* for ImGui display
	std::vector<const char*> tableNameCStr;
	for (const auto& name : tableNames) {
		tableNameCStr.push_back(name.c_str());
	}
	// Display the Combo box if we find a profile has been created otherwise we show text instead
	static int currentItem = 0;
	if (!tableNameCStr.empty() && ImGui::BeginCombo("##Select table", tableNameCStr[currentItem])) {
		for (int n = 0; n < tableNameCStr.size(); n++) {
			bool isSelected = (currentItem == n);
			if (ImGui::Selectable(tableNameCStr[n], isSelected)) {
				currentItem = n;
				chosenName = tableNameCStr[n];
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		// End Combo Box for profile drop down
		ImGui::EndCombo();
	}
	else if (tableNameCStr.empty())
		ImGui::TextWrapped("No tables found in database.");
	// Return the currently selected name in the combo box
	return chosenName;
}

void displayMappingTable(std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows)
{
    enum Mode
    {
        Mode_Copy
    };
    std::string value;
    static std::vector<std::string> values;
	// Populate vector with first row of data for preview
    for (int i = 0; i < rows.size(); i++)
    {
        std::stringstream ss(rows[i]);
        while (std::getline(ss, value, ','))
        {
            values.push_back(value);
        }        
    }
    if (rows.size() < s_columns.size())
    {
        for (int i = rows.size(); i < s_columns.size(); i++)
        {
            values.push_back("");
        }
    }
    /*
    * Created flexible mapping tables
	* We check the number of columns imported from the source and destination tables
    * and build tables tailored specifically to their column counts
    * This way we don't blow up if the column numbers aren't exactly the same.
	* We can also expand/contract based on the number of columns imported.
    */
	ImGui::BeginChild("DestinationColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ ImGui::GetContentRegionAvail().y * 0.7), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
    if (ImGui::BeginTable("DestinationMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
    {
        ImGui::TableSetupColumn(" Table Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Mapped Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < d_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("  %s", d_columns[i]);
            ImGui::TableSetColumnIndex(1);
            ImGui::Button(b_columns[i].c_str(), ImVec2(120, 40));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_Column"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(int));
                    int payload_n = *(const int*)payload->Data;
                    b_columns[i] = s_columns[payload_n];
                }
                ImGui::EndDragDropTarget();
            }
        }
        // End column mapping
        ImGui::EndTable();
    }
	// End destination column mapping child window
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("SourceColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ ImGui::GetContentRegionAvail().y * 0.7), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadOuterX/* | ImGuiTableFlags_NoPadInnerX*/))
    {
        ImGui::TableSetupColumn(" Source Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Sample data", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < s_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Button(("%s", s_columns[i]).c_str(), ImVec2(120, 40));
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("DND_Column", &i, sizeof(int));
                // Display prevew
                ImGui::Text("Mapping column %s", s_columns[i]);
                ImGui::EndDragDropSource();
            }
            ImGui::TableNextColumn();
			ImGui::Text(("%s", values[i]).c_str());
        }
		// End source column mapping table
        ImGui::EndTable();
    }
    ImGui::SameLine();
	// Display an overview of the mapping
    ImGui::BeginGroup();
    ImGui::Text("Mapping Overview:\n(Destination field = Source field)");
    for (int i = 0; i < d_columns.size(); i++)
    {
        if (!b_columns[i].empty())
        {
            ImGui::Text("%s = %s", d_columns[i], b_columns[i]);
        }
    }

    // End Mapping overview display group
    ImGui::EndGroup();

    // End table child window
    ImGui::EndChild();
}

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<std::string>& b_columns, std::vector<std::string>& rows)
{
    std::vector<std::string> queries;
    for(int i = 0; i < rows.size(); i++)
    {
        std::string query = "INSERT INTO " + table_name + " (";
        for (int i = 0; i < b_columns.size(); i++)
        {
            query += b_columns[i];
            if (i != b_columns.size() - 1)
                query += ", ";
        }
        query += ") VALUES (";
        for (int i = 0; i < rows.size(); i++)
        {
            query += "'" + rows[i] + "'";
            if (i != rows.size() - 1)
                query += ", ";
        }
        query += ");";
        queries.push_back(query);
    }

	return queries;
}

bool insertMappedData(Sql& sql, std::string query)
{
	// Execute the query to insert the data
	bool success = sql.executeQuery(query);
	return success;
}