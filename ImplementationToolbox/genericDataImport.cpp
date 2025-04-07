#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include <iostream>

void genericDataImport(Sql sql, AppLog& log, std::string dir)
{
	static std::vector<std::string> source_columns;
    static std::vector<int> source_columns_index;
	static std::vector<std::string> destination_columns;
    static std::vector<int> destination_columns_index;
    static std::vector<std::string> buffer_columns;
    static std::vector<int> buffer_columns_index;
    static std::vector<std::string> data_rows;
    static std::vector<int> data_rows_index;
	static std::vector<std::string> insert_rows;
	static std::string table_name;
	ImGui::Text("Select a file to import: "); ImGui::SameLine();
	std::string filepath = displayDataFiles(dir + "DataImport\\");

    static bool loaded_csv = false;
    static bool load_tables = false;
    static bool load_columns = false;
    static bool confirm_mapping = false;
	static bool ready_to_import = false;

	log.logStateChange("loaded_csv", loaded_csv);
	log.logStateChange("load_tables", load_tables);
	log.logStateChange("load_columns", load_columns);
	log.logStateChange("confirm_mapping", confirm_mapping);
	log.logStateChange("ready_to_import", ready_to_import);
    ImGui::SameLine();
    ImGui::BeginGroup();
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
            try 
            {
                getColumns(std::filesystem::path(dir + "DataImport\\" + filepath), source_columns);
                for (int i = 0; i < source_columns.size(); i++)
                {
                    source_columns_index.push_back(i);
                }
                getRows(std::filesystem::path(dir + "DataImport\\" + filepath), data_rows);
                log.AddLog("[INFO] File selected: %s\n", filepath.c_str());
                for (int i = 0; i < source_columns.size(); i++)
                {
                    log.AddLog("[INFO] CSV Column Successfully loaded: %s\n", source_columns[i]);
                    buffer_columns.push_back("");
                }
                loaded_csv = true;
			}
			catch (const std::ifstream::failure& e)
			{
				log.AddLog("[ERROR] Failed to load CSV file: %s", e.what());
				ImGui::SetItemTooltip("Failed to load CSV file.");
			}
            catch (std::filesystem::filesystem_error& e)
            {
				log.AddLog("[ERROR] Filesystem error: %s", e.what());
				ImGui::SetItemTooltip("Filesystem error occurred.");
			}
			catch (const std::exception& e)
			{
				log.AddLog("[ERROR] Failed to load CSV file: %s", e.what());
				ImGui::SetItemTooltip("Failed to load CSV file.");
			}
            catch (...)
            {
                log.AddLog("[ERROR] Unknown error occurred while loading CSV file.");
                ImGui::SetItemTooltip("Unknown error occurred.");
            }
        }
        
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();

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
            ImGui::SetNextItemWidth(175);
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
                    log.AddLog("[INFO] SQL Table Column loaded: %s\n", destination_columns[i].c_str());
                    buffer_columns.push_back("");
                }
                load_columns = true;
            }
        }
	}
    ImGui::EndGroup();
    ImGui::SameLine();
    if (ImGui::Button("Cancel Mapping", ImVec2(150, 0)))
    {
        // Clear all vectors
        source_columns.clear();
        destination_columns.clear();
        buffer_columns.clear();
        table_name.clear();
        data_rows.clear();
        insert_rows.clear();
		source_columns_index.clear();
		destination_columns_index.clear();
		data_rows_index.clear();
		buffer_columns_index.clear();
		// Reset all flags
        loaded_csv = false;
        load_tables = false;
        load_columns = false;
        confirm_mapping = false;
    }
    if (ImGui::CollapsingHeader("Column Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (load_columns && !confirm_mapping && loaded_csv)
        {
            displayMappingTable(source_columns, destination_columns, buffer_columns, data_rows, data_rows_index, source_columns_index, destination_columns_index);
            ImGui::SameLine();
            if (ImGui::Button("Confirm Mapping", ImVec2(150, 75)))
            {
                // Confirm mapping and import data
                confirm_mapping = true;
            }
            
        }
        if (confirm_mapping && insert_rows.size() == 0)
        {
            // Map out buffer index values from source column index positions
            // Loop over each buffer field to get it's matching source column index
            for(int i = 0; i < buffer_columns.size(); i++)
            {
                // Loop over each source column to compare the buffer value
                for (int j = 0; j < source_columns.size(); j++)
                {
					// When we match store source index position in buffer index columns vector
					if (buffer_columns[i] == source_columns[j])
					{
						// Store index position from source columns vector to use in insert query builder
                        // This index will match the destination columns to the respective data rows
						buffer_columns_index.push_back(j);
						log.AddLog("[INFO] Buffer column %s mapped to source column %s. SQL Column will be: %s\n", buffer_columns[i].c_str(), source_columns[j].c_str(), destination_columns[i]);
                        // break this loop since we can only map one column to one buffer, there's no need to go over the rest when we find our match
                        break;
					}
                }
            }
            // Generate insert queries and store them in vector
            for (int i = 0; i < data_rows.size(); i++)
            {
                insert_rows = buildInsertQuery(table_name, buffer_columns_index, destination_columns, data_rows, log);
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

    // Display the Combo box for csv files in the directory otherwise we show text instead
    static int currentItem = 0;
    ImGui::SetNextItemWidth(175);
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select unit csv", fileNameCStr[currentItem])) {
        file_filter.Draw("Filter##Files", 110.0f);
        for (int n = 0; n < fileNameCStr.size(); n++) {
            if(file_filter.PassFilter(fileNameCStr[n]))
            {
                bool isSelected = (currentItem == n);
                if (ImGui::Selectable(fileNameCStr[n], isSelected)) {
                    currentItem = n;
                    chosenName = fileNameCStr[n];
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
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
    // TODO: add keyboard capture to get a letter while combobox is active, then move the displayed position to that letter. use ImGui to see if ItemIsActive, if so start an IO monitor, when a key is pressed we do a find in the list and maybe a get position for the value in the box
    static std::vector<std::string> tableNames;
	static ImGuiTextFilter filter;
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
        ImGui::SetNextItemWidth(100);
        filter.Draw("Filter##Tables", 110.0f);
        for (int n = 0; n < tableNameCStr.size(); n++) {
            if(filter.PassFilter(tableNameCStr[n]))
            {
                bool isSelected = (currentItem == n);
                if (ImGui::Selectable(tableNameCStr[n], isSelected)) {
                    currentItem = n;
                    chosenName = tableNameCStr[n];
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
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

void displayMappingTable(std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_column_index, std::vector<int>& s_column_index, std::vector<int>& d_column_index)
{
    std::string value;
    std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    if(!copied)
    {
        s_columns_buf = s_columns;
        copied = true;
    }
	// Populate vector with first row of data for preview
    for (int i = 0; i < rows.size(); i++)
    {
        std::stringstream ss(rows[i]);
        while (std::getline(ss, value, ','))
        {
            values.push_back(value);
        }
    }
    // If there are more columns than sample data fill in blank to prevent crashing
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
	ImGui::BeginChild("DestinationColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ ImGui::GetContentRegionAvail().y), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
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
            ImGui::PushID(i);
            ImGui::Button(b_columns[i].c_str(), ImVec2(120, 40));
            ImGui::PopID();
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_Column"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(int));
                    int payload_n = *(const int*)payload->Data;
                    // Going to try a different approach with a swap method and see if it is any better.
                    const std::string tmp = b_columns[i];
                    b_columns[i] = s_columns_buf[payload_n];
					s_columns_buf[payload_n] = tmp;
                    // Swap works well, just need to track where the source columns index positions were when building queries.
                    // If source column[0] swaps with buffer [1] we need to know that column [0] from source maps to column [1] for SQL columns
                    // Perhaps use a buffer vector for source columns that way we don't mangle the original order and we can refer back to it when mapping is complete


                    // Insert source column index into buffer column index list
                    
                    // If we find the most recently pushed back payload_n value in the b_columns_index already, erase the earlier index value to keep list current
                    // Remove the old index if it exists
       //             auto it = std::find(b_column_index.begin(), b_column_index.end(), payload_n);
       //             if (it != b_column_index.end())
       //             {
       //                 // clear previous buffer column
       //                 if(i < *it)
       //                 {
       //                     //auto find_label = std::find(b_columns.rbegin(), b_columns.rend(), s_columns[payload_n]); // Need way to get distance from end of array if new buffer index is less than the old
       //                     // if i < find_label set b_columns[find_label] = "" and erase b_column_index[find_label]
       //                     auto find_label = std::find(b_columns.begin(), b_columns.end(), s_columns[payload_n]);
       //                     if (find_label != b_columns.end()) {
       //                         b_columns[std::distance(b_columns.begin(), find_label)] = "";
       //                     }
       //                 }
       //                 else
       //                 {
							//auto find_label = std::find(b_columns.begin(), b_columns.end(), s_columns[payload_n]); // Need way to get distance from end of array if new buffer index is less than the old
							//if (find_label != b_columns.end()) {
							//	b_columns[std::distance(b_columns.begin(), find_label)] = "";
							//}
       //                 }
       //                 b_column_index.erase(it);
       //             }
       //             // Use this to swap buffer columns for table columns on the sql insert generation
       //             b_column_index.push_back(payload_n);
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
    ImGui::BeginChild("SourceColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ ImGui::GetContentRegionAvail().y), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadOuterX/* | ImGuiTableFlags_NoPadInnerX*/))
    {
        ImGui::TableSetupColumn(" Source Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Sample data", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < s_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            ImGui::Button(("%s", s_columns_buf[i]).c_str(), ImVec2(120, 40));
			ImGui::PopID();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("DND_Column", &i, sizeof(int));
                // Display prevew
                ImGui::Text("Mapping column %s", s_columns[i]);
                ImGui::EndDragDropSource();
            }
            ImGui::TableNextColumn();
			ImGui::Text(("%s\n%s", s_columns[i], values[i]).c_str());
        }
		// End source column mapping table
        ImGui::EndTable();
    }
    // End Source column mapping
    ImGui::EndChild();
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
}

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<int>& b_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, AppLog& log)
{
	// TODO: Add multi-threading for large files to prevent UI from freezing and allow each insert to print to log as it's written
    // Changing logic here to only grab mapped data
    // Added integer vectors for tracking index positions of columns mapped, and columns from the table.
	// Using these index positions we can selectively choose from the table columns and map them to the source columns
    std::vector<std::string> queries;
    std::string value;
    std::vector<std::string> values;
    try
    {
        for (int i = 0; i < rows.size(); i++)
        {
            std::string query = "INSERT INTO " + table_name + " (";
            // This should be d_columns, not b_columns
            for (int i = 0; i < b_columns_index.size(); i++)
            {
                query += d_columns[b_columns_index[i]];
                if (i != b_columns_index.size() - 1)
                    query += ", ";
            }
            query += ") VALUES (";
            /*for (int i = 0; i < rows.size(); i++)
            {
                std::stringstream ss(rows[i]);
                while (std::getline(ss, value, ','))
                {
                    values.push_back(value);
                }
            }*/
            // Remove for loop. We're doing a while loop here that will get all values for that row. 
            // Now we just need to only get the data from columns that were selected in the correct order
            std::stringstream ss(rows[i]);
            while (std::getline(ss, value, ','))
            {
                values.push_back(value);
            }
            for (int i = 0; i < b_columns_index.size(); i++)
            {
                query += "'" + values[i] + "'";
                if (i != b_columns_index.size() - 1)
                    query += ", ";
            }
            query += ");";
            queries.push_back(query);
            log.AddLog("[INFO] Insert query generated: %s\n", query.c_str());
        }

        return queries;
    }
	catch (const std::exception& e)
	{
		log.AddLog("[ERROR] Failed to build insert query: %s\n", e.what());
        return {};
	}
    return {};
}

bool insertMappedData(Sql& sql, std::string query)
{
	// Execute the query to insert the data
	bool success = sql.executeQuery(query);
	return success;
}