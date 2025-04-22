#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include "imgui_stdlib.h"
#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>
#include <future>
#include <chrono>

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
    static std::vector<std::vector<std::string>>data_parsed_final;      // Store data broken apart into individual strings
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
    static bool allow_nulls[1000] = {};
    static bool restrict_duplicates[1000] = {};
    static int display_column_rows = 10;
    static int display_data_rows = 10;

    static int button_style = 0;
    static bool column_window = display_settings.getColumnWindow();
    static bool data_window = display_settings.getDataWindow();
    static bool insert_window = display_settings.getInsertWindow();
    static bool mapoverview = display_settings.getMappingOverview();

    // Forward thread processing declarations
    static std::future<std::vector<std::string>> columnFuture;
    static std::future<std::vector<std::string>> rowFuture;
    static std::future<std::vector<std::string>> destinationFuture;
    static std::future<std::vector<std::string>> insertFuture;
    static std::future<void> processFuture;
    static std::future<void> insertBuilderFuture;
    static bool
        isGettingRows = false,
        isLoadingColumns = false,
        isLoadingCSV = false,
        isLoadingData = false,
        isProcessingData = false,
        isLoadingInserts = false,
        dataProcessed = false,
        isBuildingInserts = false;

    std::mutex sqlMutex;

    enum ButtonStyle
    {
        Button_Comp,
        Button_Expand
    };

	log.logStateChange("loaded_csv", loaded_csv);
	log.logStateChange("load_tables", load_tables);
	log.logStateChange("load_columns", load_columns);
    log.logStateChange("isLoadingCSV", isLoadingCSV);
    log.logStateChange("isLoadingColumns", isLoadingColumns);
    log.logStateChange("isLoadingData", isLoadingData);
    log.logStateChange("isProcessingData", isProcessingData);
    log.logStateChange("dataProcessed", dataProcessed);
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
                // Start asynchronous thread to populate columns so we don't freeze program on larger file imports --#13
                columnFuture = std::async(
                    std::launch::async,
                    getColumns,
                    std::filesystem::path(dir + "DataImport\\" + filepath) // Pass the correct argument
                );
                isLoadingCSV = true;    // Flag the process of loading CSV has started

                // Start asynchronous thread to populate rows so we don't freeze program on larger file imports --#13
                rowFuture = std::async(
                    std::launch::async, 
                    getRows, 
                    std::filesystem::path(dir + "DataImport\\" + filepath)
                );
                isLoadingData = true;

                log.AddLog("[INFO] File selected: %s\n", filepath.c_str());
                // rowFuture.get();
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
        // Poll the future to see if CSV columns have finished loading
        if (isLoadingCSV)
        {
            // Check if future is ready without blocking
            if (columnFuture.valid() && columnFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                try
                {
                   // Retrieve the result
                   source_columns = columnFuture.get();
                   
                    // Log the loaded columns
                    for (const auto& column : source_columns)
                    {
                        log.AddLog("[INFO] CSV Table Column loaded: %s\n", column.c_str());
                    }

                    // Get count and index position of loaded columns
                    for (int i = 0; i < source_columns.size(); i++)
                    {
                        source_columns_index.push_back(i);
                        log.AddLog("[INFO] Source column index added: %i\n", i);
                    }
                }
                catch (const std::exception& e)
                {
                    log.AddLog("[ERROR] Failed to load CSV columns: %s\n", e.what());
                    ImGui::SetItemTooltip("Failed to load CSV columns.");
                }
            }

            // Check if rowFuture is ready without blocking
            if (rowFuture.valid() && rowFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                try
                {
                    // Retrieve the result
                    data_rows = rowFuture.get();

                    // Log the loaded rows
                    log.AddLog("[INFO] CSV rows loaded: %zu rows\n", data_rows.size());
                }
                catch (const std::exception& e)
                {
                    log.AddLog("[ERROR] Failed to load CSV rows: %s\n", e.what());
                    ImGui::SetItemTooltip("Failed to load CSV rows.");
                }
            }
            // If both futures are done, reset the loading flag
            if (!columnFuture.valid() && !rowFuture.valid())
            {
                isLoadingCSV = false; // Reset the loading flag
                isLoadingData = false;
                loaded_csv = true;      // Set loaded flag
                log.AddLog("[INFO] Columns and rows loading finished.\n");
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
                log.AddLog("[DEBUG] Connection string before lambda %s\n", sql._GetConnectionString().c_str());
                try
                {
                    // Capture the current value of table_name by value
                    std::string curr_table_name = table_name;
                    std::string connectionString;
                    sql._SetConnectionString();

                    {
                        // Lock the mutex to safely access sql object
                        std::lock_guard<std::mutex> lock(sqlMutex);
                        connectionString = sql._GetConnectionString();  // After locking sqlMutex get connection string value to protect from changes
                        curr_table_name = table_name;                   // After locking sqlMutex get table name to protect from changes while processing
                    }
                    // Start the asynchronous operation
                    destinationFuture = std::async(
                        std::launch::async,
                        [&sql, connectionString, curr_table_name]() {
                            //sql._SetConnectionString();
                            
                            return sql.getTableColumns(connectionString, curr_table_name);
                        }
                    );
                    log.AddLog("[DEBUG] Connection string after lambda %s\n", sql._GetConnectionString().c_str());
                    isLoadingColumns = true; // Indicate that the columns are being loaded
                }
                catch (const std::exception& e)
                {
                    log.AddLog("[ERROR] Failed to start loading SQL columns: %s\n", e.what());
                    ImGui::SetItemTooltip("Failed to start loading SQL columns.");
                }
                catch (...)
                {
                    log.AddLog("[ERROR] Unknown error occurred while loading SQL tables.\n");
                    ImGui::SetItemTooltip("Unknown error occurred.");
                }
            }
            // Poll the future to see if columns have finished loading
            if (isLoadingColumns)
            {
                // Check if future is ready without blocking
                if (destinationFuture.valid() && destinationFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    try
                    {
                        // Retrieve the result
                        destination_columns = destinationFuture.get();

                        // Log the loaded columns
                        for (const auto& column : destination_columns)
                        {
                            log.AddLog("[INFO] SQL Table Column loaded: %s\n", column.c_str());
                            buffer_columns.push_back("");
                        }

                        load_columns = true;
                        isLoadingColumns = false; // Reset the loading flag
                    }
                    catch (const std::exception& e)
                    {
                        log.AddLog("[ERROR] Failed to load SQL columns: %s\n", e.what());
                        ImGui::SetItemTooltip("Failed to load SQL columns.");
                        isLoadingColumns = false; // Reset the loading flag
                    }
                }
            }
        }
	}
    ImGui::EndGroup();
    ImGui::SameLine();
    if (ImGui::Button("Cancel Import", ImVec2(150, 0)))
    {
        clearMappings(log, source_columns, source_columns_index, destination_columns, destination_columns_index, buffer_columns, buffer_columns_index, data_rows, data_rows_index, insert_rows, table_name, loaded_csv, load_tables, load_columns, confirm_mapping, filepath, allow_nulls, confirm_data);
  //      // Clear all vectors
  //      source_columns.clear();
  //      destination_columns.clear();
  //      buffer_columns.clear();
  //      table_name.clear();
  //      data_rows.clear();
  //      insert_rows.clear();
		//source_columns_index.clear();
		//destination_columns_index.clear();
		//data_rows_index.clear();
		//buffer_columns_index.clear();
		//// Reset all flags
  //      loaded_csv = false;
  //      load_tables = false;
  //      load_columns = false;
  //      confirm_mapping = false;
  //      confirm_data = false;
  //      filepath.clear();
        }
    if(ImGui::Button("Display"))
    {
        ImGui::OpenPopup("WindowOptions");
    }

    if (ImGui::BeginPopup("WindowOptions"))
    {
        ImGui::Text("Display Settings");
        ImGui::Separator();
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
            if (load_columns && loaded_csv)
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
            if(ImGui::BeginMenu("Column Mapping"))
            {
                if (ImGui::MenuItem("Confirm Mapping", NULL, &confirm_mapping, (loaded_csv && load_columns && data_mapped_check && !confirm_mapping)))
                {
                    data_window = true;
                }
                if (ImGui::MenuItem("Cancel Mapping", NULL, false, (loaded_csv && load_columns)))
                {
                    clearMappings(log, source_columns, source_columns_index, destination_columns, destination_columns_index, buffer_columns, buffer_columns_index, data_rows, data_rows_index, insert_rows, table_name, loaded_csv, load_tables, load_columns, confirm_mapping, filepath, allow_nulls, confirm_data);
                }
                // End Column Mapping menu
                ImGui::EndMenu();
            }
            ImGui::Text("| Status:");
            if (isLoadingColumns || isLoadingCSV)
            {
                ImGui::Text("Loading Columns...");
            }
            else if (destination_columns.size() > 0 && source_columns.size() > 0)
            {
                ImGui::Text("All Columns loaded!");
            }
            else
            {
                ImGui::Text("Idle...");
            }
            // End column mapping menu bar
            ImGui::EndMenuBar();
        }
        if (load_columns && !confirm_mapping && loaded_csv)
        {
            displayMappingTable(log, display_settings, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows, allow_nulls, restrict_duplicates);
            if (confirm_mapping)
            {
                data_window = true;

            }
        }
        else
        {
            ImGui::BeginDisabled();
            displayMappingTable(log, display_settings, source_columns, destination_columns, buffer_columns, data_rows, buffer_columns_index, source_columns_index, destination_columns_index, display_column_rows, allow_nulls, restrict_duplicates);
            ImGui::EndDisabled();
        }
        // End Column Mapping window
        ImGui::EndChild();

        ImGui::SameLine();
    }
    if (mapoverview)
    {
        // Create new window for mapping overview
        (ImGui::BeginChild("Mapping Overview", ImVec2(175, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders /*| ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY*/, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar));
        ImVec2 overview_window_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - (ImGui::GetTextLineHeightWithSpacing() * 2));
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("< Mapping Overview >");

            // End Mapping overview menu bar
            ImGui::EndMenuBar();
        }
        // Display an overview of the mapping
        //ImGui::SeparatorText("Mapping Overview");
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
    
    // Perform data clean up
    // This involves taking all data from the CSV, then trimming it down so only data from columns that were mapped is prepared for import
    // Later, displayDataTable lines up the data with the correct order that the sql columns are listed in based on the column mapped to it using data_rows_index.
    //if (!cleanup && confirm_mapping)
    //{
    //    // Map out buffer index values from source column index positions
    //    // Clean up buffer index and remove -1's since we don't need them.
    //    for (int i = 0; i < buffer_columns_index.size(); i++)
    //    {
    //        if (buffer_columns_index[i] != -1)
    //        {
    //            destination_columns_index.push_back(i);                 // Map SQL table columns in order they were mapped
    //            data_rows_index.push_back(buffer_columns_index[i]);     // Get index position of data that matches columns to the SQL tables they were mapped to
    //        }
    //    }
    //    // Log index values for debugging
    //    log.AddLog("[DEBUG] destination_column_index positions =  ");
    //    for (int i = 0; i < destination_columns_index.size(); i++)
    //    {
    //        if (i == destination_columns_index.size() - 1)
    //            log.AddLog("%i", destination_columns_index[i]);
    //        else
    //            log.AddLog("%i, ", destination_columns_index[i]);
    //    }
    //    log.AddLog("\n");
    //    log.AddLog("[DEBUG] data_rows_index =  ");
    //    for (int i = 0; i < data_rows_index.size(); i++)
    //    {
    //        if (i == data_rows_index.size() - 1)
    //            log.AddLog("%i", data_rows_index[i]);
    //        else
    //            log.AddLog("%i, ", data_rows_index[i]);
    //    }
    //    log.AddLog("\n");
    //    // Remove values from data_rows that aren't mapped to a column in SQL
    //    for (int i = 0; i < data_rows.size(); i++)
    //    {
    //        std::vector<std::string>data_tmp;
    //        std::string value;
    //        std::vector<std::string>values;
    //        std::stringstream ss(data_rows[i]);
    //        while (std::getline(ss, value, ','))                                        // Read each line of data in data_rows
    //        {
    //            values.push_back(value);                                                // Add data to the vector containing the separated data elements
    //        }
    //        if (data_rows[i].back() == ',')
    //        {
    //            values.push_back("");                                                   // If last value is a comma push back an extra blank string to show we ended on a blank
    //        }
    //        for (int j = 0; j < destination_columns_index.size(); j++)                  // Loop over destination columns index size and use the data_rows_index to determine to which columns we're keeping
    //        {
    //            // If we get an empty cell we push back an empty string
    //            if (values[data_rows_index[j]] == "")
    //            {
    //                data_tmp.push_back("");
    //            }
    //            else
    //            {
    //                data_tmp.push_back(values[data_rows_index[j]]);
    //            }
    //        }
    //        data_parsed_final.push_back(data_tmp);                                      // Push vector of parsed out strings into data_parsed_final to use each data piece of only selected data
    //        data_rows[i] = "";                                                          // Clear out the row in the original data_row vector
    //        for (int k = 0; k < data_tmp.size(); k++)                                   // Loop over newly constructed row of data and concantate it into a single string to push back to data_rows
    //        {
    //            data_rows[i] += data_tmp[k];
    //            if (k != data_tmp.size() - 1)
    //                data_rows[i] += ",";                                                // Separate each value by a comma if we're not next to the end position yet
    //        }
    //    }
    //    cleanup = true;                                                                 // Tell application cleanup has been completed and we won't revist this section
    //}
    if(data_window)
    {
        // Display newly mapped rows as a table
        ImGui::BeginChild("Data Staging", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("< Data Staging >");
            if(confirm_mapping)
            {
                if (ImGui::MenuItem("Confirm Data", NULL, &confirm_data))
                {
                    insert_window = true;
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Confirm Data", NULL, &confirm_data);
                ImGui::EndDisabled();
            }

            // End data staging menu bar
            ImGui::EndMenuBar();
        }
        if (confirm_mapping && !confirm_data)
        {
            // Only process(formerly clean) data if:
            // isLoadingCSV is complete (aka column headers are loaded and the thread is closed)
            // confirm_mapping - flag set from the menu of column mapping to show the user is finished
            // isLoadingData is complete (row data has been loaded into data_rows and thread is closed)
            // dataProcessed is false - flag that is set when this thread is done and we confirm it's state is ready to indicate processing finished
            // isProcessingData - if we're already running this thread don't start it a second time. Now, we wait until it's done
            if (!isLoadingCSV && confirm_mapping && !isLoadingData &&!dataProcessed && !isProcessingData)
            {
                // Start asynchronous thread to process data
                processFuture = std::async(
                    std::launch::async,
                    processData,
                    std::ref(data_rows),
                    std::ref(data_parsed_final),
                    std::ref(buffer_columns_index),
                    std::ref(destination_columns_index),
                    std::ref(data_rows_index),
                    std::ref(log),
                    std::ref(cleanup)
                );

                isProcessingData = true; // Indicate that data processing has started
            }
            if (isProcessingData)
            {
                ImGui::Text("Cleaning up data and processing for editing...");
                if (processFuture.valid() && processFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    isProcessingData = false; // Reset the loading flag
                    dataProcessed = true;       // Set a compeletion flag since we access this function via a menu button
                    log.AddLog("[INFO] Data processing completed successfully.\n");
                }
            }
            // After data has processed make sure data_parsed_final is populated then show the table
            if(dataProcessed && data_parsed_final.size() > 0)
            {
                displayDataTable(log, destination_columns, destination_columns_index, data_rows, data_rows_index, display_data_rows);
            }
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
    
    static bool dup_check = false;
    // Generate insert queries and store them in vector
    if (!dup_check && confirm_data && insert_rows.size() < data_rows.size())
    {
        if(!isBuildingInserts)
        {
            insertBuilderFuture = std::async(
                std::launch::async,
                buildInsertQuery,
                std::ref(table_name),
                std::ref(insert_rows),
                std::ref(destination_columns_index),
                std::ref(destination_columns),
                std::ref(data_rows),
                std::ref(data_rows_index),
                std::ref(allow_nulls),
                std::ref(log)
            );

            isBuildingInserts = true;
        }
        if (isBuildingInserts &&
            insertBuilderFuture.valid() &&
            insertBuilderFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            isBuildingInserts = false;
            log.AddLog("[INFO] Insert builder processed insert statements successfully.\n");
        }
    }
    
    // Display insert queries that are generated in a new window. Display queries removed as duplicates afterward.
    if(insert_window)
    {
        // Begin child window for inserts that the user can review before pressing the button to run all inserts, as well as choose settings for the inserts
        ImGui::BeginChild("Insert Rows", ImVec2(ImGui::GetContentRegionAvail().x, 250), /*ImGuiChildFlags_AlwaysAutoResize |*/ ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar);
        static std::vector<std::string> query_results;
        static std::vector<std::string> dup_rows;
        static std::vector<bool> inserted;
        // Populate vector with bools for tracking when they are inserted into SQL or not
        while (inserted.size() < insert_rows.size())
        {
            inserted.push_back(false);
        }
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("< Insert Review >");
            if(confirm_data)
            {
                if(!dup_check)
                {
                    // Loop through each row to insert data
                    for (int i = 0; i < inserted.size(); i++)
                    {
                        int duplicate_count = 0;
                        // Loop over each column to check if we need to be concerned with duplicates
                        for (int j = 0; j < destination_columns_index.size(); j++)
                        {
                            // Check for duplicate restricted columns
                            if (restrict_duplicates[destination_columns_index[j]])
                            {
                                if (duplicate_count = sql.returnRecordCount(table_name, destination_columns[destination_columns_index[j]], data_parsed_final[i][j]) > 0)
                                {
                                    log.AddLog("[WARN] Duplicate value detected when inserting on column: '%s', which was marked to restrict duplicates. No data inserted.\n", destination_columns[destination_columns_index[j]]);
                                    // Add row to new vector for rows that contain illegal duplicate data
                                    dup_rows.push_back(insert_rows[i]);
                                    // Set insert row to blank for deleting later
                                    insert_rows[i] = "";
                                    // Break loop since we've already found a duplicate in a column marked to restrict duplicates and go to next row
                                    break;
                                }
                            }
                        }
                    }
                    // Delete rows that are blank from vector
                    for (int i = insert_rows.size() - 1; i >= 0; i--)
                    {
                        if (insert_rows[i] == "")
                            insert_rows.erase(insert_rows.begin() + i);
                    }
                    // Mark that we've checked for duplicates
                    dup_check = true;
                }
                if (ImGui::MenuItem("Insert all data"))
                {
                    try
                    {
                        for (int i = 0; i < insert_rows.size(); i++)
                        {
                            if(!inserted[i])
                            {
                                // Start new threads for each insert statement to help with performance
                                std::thread(insertMappedData, sql, insert_rows[i]).detach();
                                log.AddLog("[INFO] Successfully inserted data into SQL!");
                                inserted[i] = true;
                                query_results.push_back("Insert successful for row: " + std::to_string(i));
                            }
                            else
                            {
                                query_results.push_back("Row #" + std::to_string(i) + " already inserted into SQL!");
                                log.AddLog("[WARN] Row %i was already inserted into SQL\n", i);

                            }
                        }
                    }
                    catch (std::exception& e)
                    {
                        log.AddLog("[ERROR] Could not insert SQL statement: %s", e.what());
                    }

                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Insert all data");
                ImGui::EndDisabled();
            }

            // End insert menu bar
            ImGui::EndMenuBar();
        }
        if(confirm_data)
        {
            if (insert_rows.size() >= 1)
            {
                ImGui::Text("Review insert statements generated by tool before running insert into SQL");
                for (int i = 0; i < insert_rows.size(); i++)
                {
                    ImGui::PushID(i);
                    ImGui::Text("#%i: %s", i, insert_rows[i].c_str()); ImGui::SameLine();
                    // Create button to allow individual lines to be submittedd to SQL after it's generated
                    if (!inserted[i])
                    {
                        if (ImGui::Button("SQL"))
                        {
                            try
                            {
                                if (insertMappedData(sql, insert_rows[i]))
                                {
                                    query_results.push_back("Insert Successful for insert #" + std::to_string(i));
                                    inserted[i] = true;
                                }
                            }
                            catch (std::exception& e)
                            {
                                query_results.push_back(e.what());
                            }
                        }
                    }
                    else
                    {
                        ImGui::BeginDisabled();
                        ImGui::Button("SQL");
                        ImGui::SetItemTooltip("Inserted successfully!");
                        ImGui::EndDisabled();
                    }
                    // Pop row ID value
                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::Text("No valid data for entry! Check duplicate restrictions and try again.");
            }
            if (dup_rows.size() > 0)
            {
                ImGui::Text("Data that was found to be duplicated and removed from insert list:");
                for (int i = 0; i < dup_rows.size(); i++)
                {
                    ImGui::Text(dup_rows[i].c_str());
                }
            }
            else
                ImGui::Text("No dupliate data found!");

            if(query_results.size() > 0)
            {
                for (int i = 0; i < query_results.size(); i++)
                {
                    ImGui::Text(query_results[i].c_str());
                }
            }
        }
        // End Insert statment rows display
        ImGui::EndChild();
    }
    

	// Default return
	// return false;
}

std::vector<std::string> getColumns(const std::filesystem::path& dir/*, std::vector<std::string>& columns*/) 
{
	std::ifstream dataImport;
    std::vector<std::string> columns;

	dataImport.open(dir);
    if (!dataImport.is_open())
        throw std::runtime_error("Failed to open file: " + dir.string());
    
    std::string line;
	if(std::getline(dataImport, line))
    {
        std::stringstream ss(line);
        std::string token;
        // Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
        while (std::getline(ss, token, ','))
        {
            columns.push_back(token);
        }
    }

    return columns;
}

std::vector<std::string> getRows(const std::filesystem::path& dir)
{
    std::ifstream rowData;
    std::vector<std::string> rowTmp;
    rowData.open(dir);

    if (!rowData)
        return rowTmp;
	// Ignore first row in CSV, this is assumed to be the column headers
    rowData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(rowData, line))
    {
        rowTmp.push_back(line);
    }

    return rowTmp;
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

	// Display the Combo box if we find sql tables otherwise we show text instead
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

void displayMappingTable(AppLog& log, DisplaySettings& ds, std::vector<std::string>&s_columns, std::vector<std::string>&d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_column_index, std::vector<int>& s_columns_index, std::vector<int>& d_column_index, int& table_len, bool* nulls, bool* duplicate)
{
    std::string value;
    std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    ImVec2 button_style = ds.getButtonStyle();

    // Fix for issue #14: Clear out buffer if clear button has been clicked and we clear out the source columns from the CSV
    if (s_columns.size() == 0)
    {
        s_columns_buf.clear();
        copied = false;
    }
    // Copy current source columns to source column buffer
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
    if (ImGui::BeginTable("DestinationMappingTable", 4, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
    {
        ImGui::TableSetupColumn("Null", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn("Dup", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Table Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Mapped Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableHeadersRow();
        for (int i = 0; i < d_columns.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(i);
            ImGui::Checkbox("##nulls", &nulls[i]);
            ImGui::SetItemTooltip("Check this box to include NULLs instead of blanks for data in column '%s'.", d_columns[i]);
            log.logStateChange(("%s", d_columns[i] + " allows nulls").c_str(), nulls[i]);
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##Dups", &duplicate[i]);
            ImGui::SetItemTooltip("Check this box to check for duplicates for data in column '%s' before inserting.", d_columns[i]);
            log.logStateChange(("%s", d_columns[i] + " restricts duplicates on insert.").c_str(), duplicate[i]);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("  %s", d_columns[i]);
            ImGui::TableSetColumnIndex(3);
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
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX))
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

// Function to process data after dependencies are completed
void processData(std::vector<std::string>& data_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& buffer_columns_index,
    std::vector<int>& destination_columns_index,
    std::vector<int>& data_rows_index,
    AppLog& log, bool& cleanup)
{
    // Map out buffer index values from source column index positions
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
        std::vector<std::string> data_tmp;
        std::string value;
        std::vector<std::string> values;
        std::stringstream ss(data_rows[i]);
        while (std::getline(ss, value, ',')) // Read each line of data in data_rows
        {
            values.push_back(value); // Add data to the vector containing the separated data elements
        }
        if (data_rows[i].back() == ',')
        {
            values.push_back(""); // If last value is a comma push back an extra blank string to show we ended on a blank
        }
        for (int j = 0; j < destination_columns_index.size(); j++) // Loop over destination columns index size and use the data_rows_index to determine to which columns we're keeping
        {
            // If we get an empty cell we push back an empty string
            if (values[data_rows_index[j]] == "")
            {
                data_tmp.push_back("");
            }
            else
            {
                data_tmp.push_back(values[data_rows_index[j]]);
            }
        }
        data_parsed_final.push_back(data_tmp); // Push vector of parsed out strings into data_parsed_final to use each data piece of only selected data
        data_rows[i] = "";                     // Clear out the row in the original data_row vector
        for (int k = 0; k < data_tmp.size(); k++) // Loop over newly constructed row of data and concatenate it into a single string to push back to data_rows
        {
            data_rows[i] += data_tmp[k];
            if (k != data_tmp.size() - 1)
                data_rows[i] += ","; // Separate each value by a comma if we're not next to the end position yet
        }
    }
    cleanup = true; // Tell application cleanup has been completed and we won't revisit this section
}

void buildInsertQuery(std::string table_name, std::vector<std::string>& insert_rows, std::vector<int>& d_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, std::vector<int>& rows_index, bool* nulls, AppLog& log)
{
	// TODO: Add multi-threading for large files to prevent UI from freezing and allow each insert to print to log as it's written
    // DONE: Changing logic here to only grab mapped data
    // Added integer vectors for tracking index positions of columns mapped, and columns from the table.
    // std::vector<std::string> queries;
    
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
                    // Convert value to uppercase for comparison
                    std::string upper_value = values[i];
                    std::transform(upper_value.begin(), upper_value.end(), upper_value.begin(), ::toupper);
                    if (upper_value == "NULL")                                  // Handle any NULL values passed with correct SQL syntax. Need to ignore case and catch all nulls
                        query += "NULL";
                    else if (nulls[d_columns_index[i]] && values[i] == "")      // If we marked a column as null requried while mapping and it has a blank fill in NULL instead
                        query += "NULL";
                    else
                        query += "'" + values[i] + "'";
                    if (i != d_columns_index.size() - 1)
                        query += ",";
                }
                query += ");";
                insert_rows.push_back(query);
                log.AddLog("[INFO] Insert query generated: %s\n", query.c_str());
            }
        } while (insert_rows.size() < rows.size());
    }
	catch (const std::exception& e)
	{
		log.AddLog("[ERROR] Failed to build insert query: %s\n", e.what());
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

void clearMappings(AppLog& log, std::vector<std::string>& source_columns, std::vector<int>& source_columns_index, std::vector<std::string>& destination_columns, std::vector<int>& destination_columns_index, std::vector<std::string>& buffer_columns, std::vector<int>& buffer_columns_index, std::vector<std::string>& data_rows, std::vector<int>& data_rows_index, std::vector<std::string>& insert_rows, std::string& table_name, bool& loaded_csv, bool& load_tables, bool& load_columns, bool& confirm_mapping, std::string& filepath, bool* nulls, bool& confirm_data)
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
    confirm_data = false;
    filepath.clear();

    // Log the clearing action
    log.AddLog("[INFO] All vectors and flags have been cleared.\n");
}