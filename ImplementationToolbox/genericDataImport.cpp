#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include "imgui_stdlib.h"
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

    static int display_column_rows = 15;
    static int display_data_rows = 15;

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
                    /*buffer_columns.push_back("");*/
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
        if (load_columns || table_name.empty())
        {
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Load Columns");
            ImGui::SetItemTooltip("Columns loaded!");
            ImGui::EndDisabled();
        }
        /*else if (table_name.empty())
        {
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Load Columns");
            ImGui::SetItemTooltip("Select a table to proceed.");
            ImGui::EndDisabled();
        }*/
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
        filepath.clear();
    }
    static bool mapoverview = false;
    if (ImGui::Checkbox("Display mapping overview", &mapoverview));
    if(mapoverview)
    {
        log.logStateChange("mapoverview", &mapoverview);
        // Create new window for mapping overview
        ImGui::Begin("Mapping Overview", &mapoverview, ImGuiWindowFlags_AlwaysAutoResize);
        // Display an overview of the mapping
        ImGui::Text("Mapping Overview:");
        if(ImGui::BeginTable("Overview", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Destination");
            ImGui::TableSetupColumn("=");
            ImGui::TableSetupColumn("Source");
            ImGui::TableHeadersRow();
            for (int i = 0; i < destination_columns.size(); i++)
            {                
                if (!buffer_columns[i].empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", destination_columns[i]);
                    ImGui::TableNextColumn();
                    ImGui::Text("=");
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", buffer_columns[i]);
                }
            }   // End Mapping overview

            // End Mapping overview table
            ImGui::EndTable();
        }

        // End Mapping overview window
        ImGui::End();
    }
    ImGui::SameLine();
    if (ImGui::Button("Table Settings"))
    {
        ImGui::OpenPopup("TableSettings");
    }
    if (ImGui::BeginPopup("TableSettings"))
    {
        ImGui::Text("Table Settings\nClick outside this box to close it.");
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("Mapping Table Length", &display_column_rows, 10, 100);
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("Data Staging Table Length", &display_data_rows, 10, 100);

        ImGui::EndPopup();
    }
    if (ImGui::CollapsingHeader("Column Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (load_columns && !confirm_mapping && loaded_csv)
        {   
            displayMappingTable(log, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows);
            ImGui::SameLine();
            if (!load_columns && !loaded_csv && confirm_mapping)
            {
                ImGui::BeginDisabled();
                ImGui::Button("Confirm Mapping", ImVec2(150, 75));
                ImGui::EndDisabled();
            }
            else if (load_columns && loaded_csv && confirm_mapping)
            {
                ImGui::BeginDisabled();
                ImGui::Button("Confirm Mapping", ImVec2(150, 75));
                ImGui::EndDisabled();
            }
            else
            {
                if (ImGui::Button("Confirm Mapping", ImVec2(150, 75)))
                {
                    // Confirm mapping and import data to staging
                    confirm_mapping = true;
                }
            }
        }
        else
        {
            ImGui::BeginDisabled();
            displayMappingTable(log, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows);
            ImGui::SameLine();
            if (ImGui::Button("Confirm Mapping", ImVec2(150, 75)))
            {
                // Confirm mapping and import data to staging
                confirm_mapping = true;
            }
            ImGui::EndDisabled();
        }

        if (confirm_mapping && insert_rows.size() == 0)
        {
            // Map out buffer index values from source column index positions
            // Clean up buffer index and remove -1's since we don't need them.
            for (int i = 0; i < buffer_columns_index.size(); i++)
            {
                if (buffer_columns_index[i] != -1)
                {
                    destination_columns_index.push_back(i);                 // Map SQL table columns in order they were mapped
                    data_rows_index.push_back(buffer_columns_index[i]);     // Get index position of data that matches columns to the SQL tables they were mapped to
                }
            }

            // Remove values from the columns that aren't mapped to anything from data_rows
            for (int i = 0; i < data_rows.size(); i++)
            {
                std::vector<std::string>data_tmp;
                std::string value;
                std::vector<std::string>values;
				std::stringstream ss(data_rows[i]);
				while (std::getline(ss, value, ','))                                        // Read each line of data in data_rows
				{
					values.push_back(value);                                                // Add data to the vector containing the separated data elements
				}
				for (int j = 0; j < destination_columns_index.size(); j++)                  // Loop over destination columns index size and use the data_rows_index to determine to which columns we're keeping
				{
					data_tmp.push_back(values[data_rows_index[j]]);
				}
				data_rows[i] = "";                                                          // Clear out the row in the original data_row vector
				for (int k = 0; k < data_tmp.size(); k++)                                   // Loop over newly constructed row of data and concantate it into a single string to push back to data_rows
				{
					data_rows[i] += data_tmp[k];
					if (k != data_tmp.size() - 1)
						data_rows[i] += ",";                                                // Separate each value by a comma if we're not next to the end position yet
				}
            }
            
            // Generate insert queries and store them in vector
            /*for (int i = 0; i < data_rows.size(); i++)
            {
                insert_rows = buildInsertQuery(table_name, buffer_columns_index, destination_columns, data_rows, log);
            }*/

            // This is for debugging so we don't keep running this section.
            // TODO: Remove this when we're ready to start building insert queries
            insert_rows.push_back("");
        }
    }

    
    // Display newly mapped rows as a table
    if(confirm_mapping)
    {
        displayDataTable(log, destination_columns, destination_columns_index, data_rows, data_rows_index, display_data_rows);
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

void displayMappingTable(AppLog& log, std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_column_index, std::vector<int>& s_columns_index, std::vector<int>& d_column_index, int& table_len)
{
    std::string value;
    std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    if(!copied && s_columns.size() > 0)
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
	// Prepopulate b_column_index with -1 to show no mapping has been done yet
	if (b_column_index.size() < b_columns.size())
    {
        for (int i = 0; i < b_columns.size(); i++)
        {
            b_column_index.push_back(-1);
        }
    }
    /*
    * Created flexible mapping tables
	* We check the number of columns imported from the source and destination tables
    * and build tables tailored specifically to their column counts
    * This way we don't blow up if the column numbers aren't exactly the same.
	* We can also expand/contract based on the number of columns imported.
    */
    const int TEXT_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
	ImGui::BeginChild("DestinationColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ TEXT_HEIGHT * table_len), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
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
                    // Using a swap method to manage column labels
                    const std::string tmp = b_columns[i];
                    b_columns[i] = s_columns_buf[payload_n];                    
                    // If source column[0] swaps with buffer [1] we need to know that column [0] from source maps to column [1] for SQL columns
                    // Use this to set the source column index position to the buffer button it swapped places with
					// Verify against the s_column vector at the end and correct any mismatched values
                    if(s_columns_buf[payload_n] != "")
                    {
                        b_column_index[i] = payload_n;
                        log.AddLog("[INFO] Source column index added to buffer column index %i = %i\n", i, payload_n);
                    }
                    else
                    {
                        b_column_index[i] = -1;                 // Set buffer back to -1 to indicate no mapping if we swap with a source column that is blank
                    }

                    s_columns_buf[payload_n] = tmp;             // Moving s_column swap after we store the index location to buffer column

                    // Verify newly assigned buffer index against untouched source index
                    log.AddLog("[DEBUG] Verifying buffer index position against source columns index...\n");
                    for (int j = 0; j < b_column_index.size(); j++)
                    {
                        if (b_columns[j] == "")
                        {
                            // if label is blank do nothing
                        }
                        else if (b_columns[j] != s_columns[b_column_index[j]])
                        {
                            // If the buffer label isn't equal to the source label we know the source buffer has been misaligned.
                            // Correct buffer label by comparing to source labels and use that position
                            log.AddLog("[WARN] Detected mismatched indexes! Attempting to match now...\n");
							auto it = std::find(s_columns.begin(), s_columns.end(), b_columns[j]);          // Find label value of buffer column at position j within the source column vector
							int s_col = std::distance(s_columns.begin(), it);                               // Find distance to label located before and return position number
                            b_column_index[j] = s_col;                                                      // Update buffer column to have correct index from source vector
                            log.AddLog("[DEBUG] Buffer column index matched %i set to source column index %i\n", j, s_col);
                        }
                    }
                    log.AddLog("[DEBUG] Buffer column index positions verified against source columns index successfully.\n");
                    // Display final b_column_index values, when we're done it should match the order and source index of each column added
                    log.AddLog("[DEBUG] b_column_index positions =  ");
                    for (int i = 0; i < b_column_index.size(); i++)
                    {
                        if (i == b_column_index.size() - 1)
                            log.AddLog("%i", b_column_index[i]);
                        else
                            log.AddLog("%i, ", b_column_index[i]);
                    }
					log.AddLog("\n");
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
    ImGui::BeginChild("SourceColumnMapping", ImVec2(0,/*ImGui::GetContentRegionAvail().x / 2,*/ TEXT_HEIGHT * table_len), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX/* | ImGuiChildFlags_Border*/);
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadOuterX))
    {
        ImGui::TableSetupColumn(" Source Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Sample data", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < s_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            ImGui::Button((s_columns_buf[i]).c_str(), ImVec2(120, 40));
			ImGui::PopID();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("DND_Column", &i, sizeof(int));
                // Display prevew
                ImGui::Text("Mapping column %s", s_columns_buf[i]);
                ImGui::EndDragDropSource();
            }
            ImGui::TableNextColumn();
			ImGui::Text("%s\n%s", s_columns[i].c_str(), values[i].c_str());
        }
        
		// End source column mapping table
        ImGui::EndTable();
    }
    
    // End Source column mapping
    ImGui::EndChild();
}

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<int>& d_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, std::vector<int>& rows_index, AppLog& log)
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
            for (int i = 0; i < d_columns_index.size(); i++)
            {
                query += d_columns[d_columns_index[i]];
                if (i != d_columns_index.size() - 1)
                    query += ", ";
            }
            query += ") VALUES (";  
            std::stringstream ss(rows[i]);
            while (std::getline(ss, value, ','))
            {
                values.push_back(value);
            }
            for (int i = 0; i < d_columns_index.size(); i++)
            {
                query += "'" + values[i] + "'";
                if (i != d_columns_index.size() - 1)
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

/// <summary>
/// Uses data collected from an imported CSV and mapped column headers to build the table where we display the data we mapped and the order it was mapped in.
/// </summary>
/// <param name="log"> - Pass your AppLog object to allow the ability to write log lines as the data is processed for debugging</param>
/// <param name="d_columns"> - vector containing all column headers pulled from the loaded SQL table.</param>
/// <param name="d_columns_index"> - vector containing index positions of each column that was mapped.</param>
/// <param name="rows"> - vector containing data that was read in from the CSV. By this point it should be reduced to only contain data from columns we have mapped to.</param>
/// <param name="rows_index"> - vector containing index positions from the CSV of data columns to track which SQL column it shoudl belong in.</param>
void displayDataTable(AppLog& log, std::vector<std::string>& d_columns, std::vector<int>& d_columns_index, std::vector<std::string>& rows, std::vector<int>& rows_index, int& table_len)
{
    const int TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * table_len);

    ImGui::SeparatorText("Staging Area");                                           // Create some space between column mapping and staging table
    // Begin table for displaying and editing actual data, as well as showing it was mapped correctly.
	// TODO fix table display for large imports, fix display for mapping overview to not be in the way(maybe make it it's own window?)
    if (ImGui::BeginTable("DataStaging", d_columns_index.size(), ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV, outer_size))
    {
        std::string value;
        std::vector<std::string> tmp_rows;
        static std::vector<std::vector<std::string>> values;
        for(int i = 0; i < rows.size(); i++)
        {
            std::stringstream ss(rows[i]);
            while (std::getline(ss, value, ','))
            {
                tmp_rows.push_back(value);
            }
            values.push_back(tmp_rows);
            tmp_rows.clear();
        }
        // Setup mapped columns as headers
        ImGui::TableSetupScrollFreeze(0, 1);                                        // Make headers row always visible
        for (int i = 0; i < d_columns_index.size(); i++)
        {
			ImGui::TableSetupColumn(d_columns[d_columns_index[i]].c_str());         // Use mapped column names from the SQL table to set up the headers
			// log.AddLog("[INFO] Table header %s identified\n", d_columns[d_columns_index[i]].c_str());
        }
		ImGui::TableHeadersRow();                                                   // Tells UI this will be our headers for the table


        // Set up data rows
        for (int i = 0; i < rows.size(); i++)                                       // Begin looping through each row of data
        {
			ImGui::TableNextRow();                                                  // Start new row each loop
   //         std::string value;                                                      // String to store each individual value into while we parse lines
			//std::vector<std::string> values;                                        // Vector to hold each value from the row
			//std::stringstream ss(rows[i]);                                          // Stream the row data into a string stream
   //         while (std::getline(ss, value, ','))
   //         {
   //             values.push_back(value.c_str());                                    // Push back value into values
   //         }
            // Push row ID to each row
            ImGui::PushID(i);
			for (int k = 0; k < d_columns_index.size(); k++)                        // Begin looping through the columns to populate data in each one
			{
				ImGui::TableSetColumnIndex(k);                                      // Set the next columns index up
                // Push value ID of k to each value
                ImGui::PushID(k);
                // TODO: Fix blank row editing probably need to append column name as well as index pos to field
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("", &values[i][k], ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase);          // Create box for input lines to apppear in. This displays imported data and allows edits to be made before sending it
                // Pop value ID of k
                ImGui::PopID();
			}
            // Pop row ID of i
            ImGui::PopID();
        }
        // End data table
        ImGui::EndTable();
    }
}

bool insertMappedData(Sql& sql, std::string query)
{
	// Execute the query to insert the data
	bool success = sql.executeQuery(query);
	return success;
}