#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include "imgui_stdlib.h"
#include <iostream>

void genericDataImport(bool* p_open, Sql sql, AppLog& log, std::string dir)
{

    static DisplaySettings display_settings;                // Init struct for display settings to maintain user settings
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
    static bool confirm_data = false;
    static bool cleanup = false;
    static bool data_mapped_check = false;              //  Set to true when a buffer column is mapped to something. Reset to false if they are all blank
    static bool* allow_nulls;
    static int display_column_rows = 10;
    static int display_data_rows = 10;

    static int button_style = 0;
    static bool column_window = display_settings.getColumnWindow();
    static bool data_window = display_settings.getDataWindow();
    static bool insert_window = display_settings.getInsertWindow();
    static bool mapoverview = display_settings.getMappingOverview();

    enum ButtonStyle
    {
        Button_Comp,
        Button_Expand
    };

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
            if (load_columns) ImGui::SetItemTooltip("Columns loaded!");
            else ImGui::SetItemTooltip("Select a table to load columns.");
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
        filepath.clear();
    }
    if(ImGui::Button("Display"))
    {
        ImGui::OpenPopup("WindowOptions");
    }

    if (ImGui::BeginPopup("WindowOptions"))
    {
        ImGui::SeparatorText("Mapping display style:");
        if (ImGui::RadioButton("Expanded", &button_style, Button_Comp)); ImGui::SameLine(); if (ImGui::RadioButton("Compact", &button_style, Button_Expand));
        if (button_style == Button_Comp) 
        { 
            display_settings.setButtonStyle(false); 
        }
        else 
        { 
            display_settings.setButtonStyle(true); 
        }
        ImGui::SeparatorText("Windows");
        if (ImGui::Checkbox("Display Column Mapping", &column_window));
        if (ImGui::Checkbox("Display mapping overview", &mapoverview));
        if (ImGui::Checkbox("Display Data Staging", &data_window));
        if (ImGui::Checkbox("Display Insert Window", &insert_window));

        display_settings.setColumnWindow(column_window);
        display_settings.setDataWindow(data_window);
        display_settings.setInsertWindow(insert_window);
        display_settings.setMappingOverview(mapoverview);
        log.logStateChange("mapoverview", mapoverview);
        log.logStateChange("column_window", &column_window);
        log.logStateChange("data_window", &data_window);
        log.logStateChange("insert_window", &insert_window);

        // End windowoptions popup
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Table Settings"))
    {
        ImGui::OpenPopup("TableSettings");
    }
    if (ImGui::BeginPopup("TableSettings"))
    {
        ImGui::Text("Table Settings");
        ImGui::Separator();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("Mapping Table Length", &display_column_rows, 10, 100);
        ImGui::SetItemTooltip("This slider adjusts the maximum length of the column mapping source and destination tables.\nThis allows you to show or hide more rows of the mapping table.");
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("Data Staging Table Length", &display_data_rows, 10, 100);
        ImGui::SetItemTooltip("This slider adjusts the maximum length of the data mapping table.\nThis allows you to show or hide more rows of the data table.");
        // End Table settings popup
        ImGui::EndPopup();
    }
    if (column_window)
    {
        /*ImGui::SetNextWindowSizeConstraints(ImVec2(410, 150), ImVec2(1920, 1080));*/
        ImGui::BeginChild("Column Mapping", ImVec2(500.0f, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar())
        {
            if(load_columns && loaded_csv)
            {
                // Quickly check if any buffer column has a value if we haven't already
                for (int i = 0; i < buffer_columns.size(); i++)
                {
                    if (buffer_columns[i] != "")
                    {
                        data_mapped_check = true;                   // set true and break loop if we find a label in buffer
                        break;
                    }
                    else
                    {
                        data_mapped_check = false;                  // keep setting to false until we find one and break so we don't end up back here
                    }
                }
            }
            if (ImGui::MenuItem("Confirm Mapping", NULL, &confirm_mapping, (loaded_csv && load_columns && data_mapped_check && !confirm_mapping)))
            {
                data_window = true;
            }
            if (ImGui::MenuItem("Cancel Mapping", NULL, false, (loaded_csv && load_columns)))
            {
                clearMappings(log, source_columns, source_columns_index, destination_columns, destination_columns_index, buffer_columns, buffer_columns_index, data_rows, data_rows_index, insert_rows, table_name, loaded_csv, load_tables, load_columns, confirm_mapping, filepath, allow_nulls);
            }

            // End column mapping menu bar
            ImGui::EndMenuBar();
        }
        if (load_columns && !confirm_mapping && loaded_csv)
        {
            ImGui::SeparatorText("Column Mapping");
            displayMappingTable(log, display_settings, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows, allow_nulls);
            if (confirm_mapping)
            {
                data_window = true;
            }
        }
        else
        {
            ImGui::BeginDisabled();
            displayMappingTable(log, display_settings, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows, allow_nulls);
            ImGui::EndDisabled();
        }
        // End Column Mapping window
        ImGui::EndChild();

        ImGui::SameLine();
    }
    if (mapoverview)
    {
        // Create new window for mapping overview
        (ImGui::BeginChild("Mapping Overview", ImVec2(175, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders /*| ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY*/, ImGuiWindowFlags_HorizontalScrollbar));
        ImVec2 overview_window_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - (ImGui::GetTextLineHeightWithSpacing() * 2));
        // Display an overview of the mapping
        ImGui::SeparatorText("Mapping Overview");
        if (ImGui::BeginTable("Overview", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX, overview_window_size))
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
        ImGui::EndChild();
    
        ImGui::SameLine();
    }
    
    if (!cleanup && confirm_mapping)
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
        // Log index values for debugging
        log.AddLog("[DEBUG] destination_column_index positions =  ");
        for (int i = 0; i < destination_columns_index.size(); i++)
        {
            if (i == destination_columns_index.size() - 1)
                log.AddLog("%i", destination_columns_index[i]);
            else
                log.AddLog("%i, ", destination_columns_index[i]);
        }
        log.AddLog("\n");
        log.AddLog("[DEBUG] data_rows_index =  ");
        for (int i = 0; i < data_rows_index.size(); i++)
        {
            if (i == data_rows_index.size() - 1)
                log.AddLog("%i", data_rows_index[i]);
            else
                log.AddLog("%i, ", data_rows_index[i]);
        }
        log.AddLog("\n");
        // Remove values from data_rows that aren't mapped to a column in SQL
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
            if (data_rows[i].back() == ',')
            {
                values.push_back("");                                                   // If last value is a comma push back an extra blank string to show we ended on a blank
            }
            for (int j = 0; j < destination_columns_index.size(); j++)                  // Loop over destination columns index size and use the data_rows_index to determine to which columns we're keeping
            {
                // If the last position is a comma it means we ended with an empty string
                /*if (j == destination_columns_index.size() - 1 && data_rows[i].end()[0] == ',')
                {
                    data_tmp.push_back("");
                }*/
                // If we get an empty cell we just push back an empty string
                if (values[data_rows_index[j]] == "")
                {
                    data_tmp.push_back("");
                }
                else
                {
                    data_tmp.push_back(values[data_rows_index[j]]);
                }
            }
            data_rows[i] = "";                                                          // Clear out the row in the original data_row vector
            for (int k = 0; k < data_tmp.size(); k++)                                   // Loop over newly constructed row of data and concantate it into a single string to push back to data_rows
            {
                data_rows[i] += data_tmp[k];
                if (k != data_tmp.size() - 1)
                    data_rows[i] += ",";                                                // Separate each value by a comma if we're not next to the end position yet
            }
        }
        cleanup = true;                                                                 // Tell application cleanup has been completed and we won't revist this section
    }
    if(data_window)
    {
        // Display newly mapped rows as a table
        ImGui::BeginChild("Data Staging", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::MenuItem("Confirm Data", NULL, &confirm_data))
            {
                insert_window = true;
            }

            // End data staging menu bar
            ImGui::EndMenuBar();
        }
        if (confirm_mapping && !confirm_data)
        {
            displayDataTable(log, destination_columns, destination_columns_index, data_rows, data_rows_index, display_data_rows);
            /*if (ImGui::Button("Confirm Data"))
            {
                confirm_data = true;
                insert_window = true;
            }
            ImGui::SetItemTooltip("When data is mapped to your liking, click this to generate SQL queries");*/
        }
        else if (confirm_mapping && confirm_data)
        {
            ImGui::BeginDisabled();
            displayDataTable(log, destination_columns, destination_columns_index, data_rows, data_rows_index, display_data_rows);
            /*ImGui::Button("Confirm Data");*/
            ImGui::EndDisabled();
            insert_window = true;
        }
        // End Staging area window
        ImGui::EndChild();
    }

    // Generate insert queries and store them in vector
    if (confirm_data && insert_rows.size() < data_rows.size())
    {
        insert_rows = buildInsertQuery(table_name, destination_columns_index, destination_columns, data_rows, data_rows_index, log);
    }
    
    if(insert_window)
    {
        ImGui::BeginChild("Insert Rows", ImVec2(ImGui::GetContentRegionAvail().x, 250), /*ImGuiChildFlags_AlwaysAutoResize |*/ ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::MenuItem("Confirm Insert statements"));
            if (ImGui::MenuItem("Reject Insert statements"));

            // End insert menu bar
            ImGui::EndMenuBar();
        }
        if (insert_rows.size() > 1)
        {
            // Begin child window for inserts that the user can review before pressing the button to run all inserts, as well as choose settings for the inserts
            ImGui::Text("Review insert statements generated by tool before running insert into SQL");
            for (int i = 0; i < insert_rows.size(); i++)
            {
                ImGui::Text(insert_rows[i].c_str());
            }
        }
        // End Insert statment rows display
        ImGui::EndChild();
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
    // DONE: add keyboard capture to get a letter while combobox is active, then move the displayed position to that letter. use ImGui to see if ItemIsActive, if so start an IO monitor, when a key is pressed we do a find in the list and maybe a get position for the value in the box -- solved with text filter
    static std::vector<std::string> tableNames;
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

void displayMappingTable(AppLog& log, DisplaySettings& ds, std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_column_index, std::vector<int>& s_columns_index, std::vector<int>& d_column_index, int& table_len, bool nulls)
{
    std::string value;
    std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    ImVec2 button_style = ds.getButtonStyle();

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
    static bool allow_nulls[256] = {};
    const int TEXT_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

    
	ImGui::BeginChild("DestinationColumnMapping", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
    if (ImGui::BeginTable("DestinationMappingTable", 3, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
    {
        ImGui::TableSetupColumn("Null", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Table Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Mapped Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < d_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(i);
            ImGui::Checkbox("##nulls", &allow_nulls[i]); 
            // ImGui::AlignTextToFramePadding();
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("  %s", d_columns[i]);
            ImGui::TableSetColumnIndex(2);
            ImGui::Button(b_columns[i].c_str(), button_style);
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
    ImGui::BeginChild("SourceColumnMapping", ImVec2(0,0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
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
            ImGui::Button((s_columns_buf[i]).c_str(), button_style);
			ImGui::PopID();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("DND_Column", &i, sizeof(int));
                // Display prevew
                ImGui::Text("Mapping column %s", s_columns_buf[i]);
                ImGui::EndDragDropSource();
            }
            ImGui::TableNextColumn();
			ImGui::Text("%s", /*s_columns[i].c_str(),*/ values[i].c_str());
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
    // DONE: Changing logic here to only grab mapped data
    // Added integer vectors for tracking index positions of columns mapped, and columns from the table.
    std::vector<std::string> queries;
    
    try
    {
        do
        {
            for (int i = 0; i < rows.size(); i++)
            {
                std::string value;
                std::vector<std::string> values;
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
                if (rows[i].back() == ',')
                {
                    values.push_back("");           // Push a blank string back on the end if last value is a comma
                }
                for (int i = 0; i < d_columns_index.size(); i++)
                {
                    query += "'" + values[i] + "'";
                    if (i != d_columns_index.size() - 1)
                        query += ",";
                }
                query += ");";
                queries.push_back(query);
                log.AddLog("[INFO] Insert query generated: %s\n", query.c_str());
            }
        } while (queries.size() < rows.size());

        return queries;
    }
	catch (const std::exception& e)
	{
		log.AddLog("[ERROR] Failed to build insert query: %s\n", e.what());
        return {};
	}
    // return {};
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
	// DONE fix table display for large imports, fix display for mapping overview to not be in the way(maybe make it it's own window?)
    if (ImGui::BeginTable("DataStaging", d_columns_index.size(), ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV, outer_size))
    {
        std::string value;                                                          // String to store each individual value while we parse row lines
        std::vector<std::string> tmp_rows;                                          // Vector to hold each value from a row
        static std::vector<std::vector<std::string>> values;                        // Vector to store the rows together - this is what we'll edit and update the rows vector with
        while(values.size() < rows.size())                                          // Added while loop so we only populate values once
        {
            for (int i = 0; i < rows.size(); i++)                                   // Loop through rows to parse out data from each one - need to do this only once after reading data into these buffer variables
            {
                std::stringstream ss(rows[i]);                                      // Get each value from original data_rows vector
                while (std::getline(ss, value, ','))
                {
                    tmp_rows.push_back(value);                                      // Push back value into tmp_rows
                }
                if (value == "")                                                    // Empty string handle
                {
                    tmp_rows.push_back("");
                }
                values.push_back(tmp_rows);                                         // Push new tmp_rows into values
                tmp_rows.clear();                                                   // Clear the tmp rows to start again with the next row
            }
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
            // Push row ID to each row
            ImGui::PushID(i);
			for (int k = 0; k < d_columns_index.size(); k++)                        // Begin looping through the columns to populate data in each one
			{
				ImGui::TableSetColumnIndex(k);                                      // Set the next columns index up
                // Push value ID of k to each value
                ImGui::PushID(k);
                // DONE: Fix blank row editing probably need to append column name as well as index pos to field
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                std::string tmp = values[i][k];                                     // Store current value of field before edits are made for logging
                if(ImGui::InputText("", &values[i][k], ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue))          // Create box for input lines to apppear in. This displays imported data and allows edits to be made before sending it
                {
                    log.AddLog("[DEBUG] Modified row %i, column %s. Old value: %s; New value: %s;\n", i, d_columns[d_columns_index[k]], tmp, values[i][k]);
                }
                // Pop value ID of k
                ImGui::PopID();
			}
            // Pop row ID of i
            ImGui::PopID();
        }
        // Update referenced rows vector with modified data and then set rows vector equal to tmp_rows to update the reference vector
        for (int i = 0; i < rows.size(); i++)
        {
            // Blank out current row value to replace with new data from values
            rows[i] = "";
            for (int j = 0; j < d_columns_index.size(); j++)
            {
                rows[i] += values[i][j];
                if (values[i][j] == "")
                {
                    rows[i] += "";
                }
                if (j < d_columns_index.size() - 1 || d_columns_index.size() == 1 && rows.size() > 1 && i < rows.size() - 1)
                    rows[i] += ',';
            }
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

void clearMappings(AppLog& log, std::vector<std::string>& source_columns, std::vector<int>& source_columns_index, std::vector<std::string>& destination_columns, std::vector<int>& destination_columns_index, std::vector<std::string>& buffer_columns, std::vector<int>& buffer_columns_index, std::vector<std::string>& data_rows, std::vector<int>& data_rows_index, std::vector<std::string>& insert_rows, std::string& table_name, bool& loaded_csv, bool& load_tables, bool& load_columns, bool& confirm_mapping, std::string& filepath, bool* nulls)
{
    // Clear null flags
    for (int i = 0; i < destination_columns.size(); i++)
    {
        nulls[i] = false;
    }
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

    // Log the clearing action
    log.AddLog("[INFO] All vectors and flags have been cleared.\n");
}