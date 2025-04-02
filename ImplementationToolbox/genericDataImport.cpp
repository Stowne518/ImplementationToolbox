#include "genericDataImport.h"
#include "Sql.h"
#include <iostream>

void genericDataImport(Sql sql, std::string dir)
{
	static std::vector<std::string> source_columns;
	static std::vector<std::string> destination_columns;
    static std::vector<std::string> buffer_columns;
	static std::string table_name;
	ImGui::Text("Select a file to import: "); ImGui::SameLine();
	std::string filepath = displayDataFiles(dir + "DataImport\\");

    static bool loaded = false;
    static bool load_tables = false;
    static bool load_columns = false;

	if (!filepath.empty() && !loaded)
    {
        ImGui::SameLine();
		if(ImGui::Button("Load CSV"))
        {
            getColumns(std::filesystem::path(dir + "DataImport\\" + filepath), source_columns);
			for (int i = 0; i < source_columns.size(); i++)
			{
				std::cout << "Column loaded: " << source_columns[i] << std::endl;
				buffer_columns.push_back("");
			}
            loaded = true;
        }
        
	}
    else
    {
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button("Load CSV");
        ImGui::EndDisabled();
    }

	// Get destination columns
	if (sql._GetConnected())
    {
        if (ImGui::Button("Load SQL Tables"))
        {
            load_tables = true;
        }
        if(load_tables)
        {
            ImGui::SameLine();
            ImGui::Text("Select a table to import to: "); ImGui::SameLine();
            ImGui::SetNextItemWidth(250);
            table_name = displayTableNames(sql);
        }
		if (!table_name.empty())
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
        else
        {
            ImGui::SameLine();
			ImGui::BeginDisabled();
			ImGui::Button("Load Columns");
			ImGui::EndDisabled();
        }
	}
	else
	{
		ImGui::BeginDisabled();
		ImGui::Button("Load SQL Tables");
        ImGui::SetItemTooltip("Connect to a SQL database to proceed.");
		ImGui::EndDisabled();
	}
	if (load_columns)
	{
        ImGui::BeginGroup();
		displayMappingTable(source_columns, destination_columns, buffer_columns);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Mapping Overview: ");
        for (int i = 0; i < destination_columns.size(); i++)
        {
            if(!buffer_columns[i].empty())
            {
                ImGui::Text("%s = %s", destination_columns[i], buffer_columns[i]);
            }
        }
        ImGui::EndGroup();
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

void displayMappingTable(std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns)
{
    enum Mode
    {
        Mode_Copy
    };
    /*
    * Created flexible mapping tables
	* We check the number of columns imported from the source and destination tables
    * and build tables tailored specifically to their column counts
    * This way we don't blow up if the column numbers aren't exactly the same.
	* We can also expand/contract based on the number of columns imported.
    */
    if(ImGui::TreeNode("Column Mapping"))
    {
        if (ImGui::BeginTable("DestinationMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Table Columns", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Mapped Columns", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (int i = 0; i < d_columns.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", d_columns[i]);
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
        ImGui::SameLine();
        if (ImGui::BeginTable("SourceMappingTable", 1, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Source Columns", ImGuiTableColumnFlags_WidthFixed);
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
            }
			// End source column mapping table
            ImGui::EndTable();
        }
        // End tree node
		ImGui::TreePop();
    }
}