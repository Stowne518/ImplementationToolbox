#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include "systemFunctions.h"
//#include <iostream>
//#include <algorithm>
#include <thread>
//#include <future>
//#include <chrono>
//#include <unordered_set>
#pragma unmanaged
std::mutex sqlMutex;


void genericDataImport(bool* p_open, Sql& sql, AppLog& log, std::string dir)
{

    static DisplaySettings display_settings;                // Init struct for display settings to maintain user settings
    static std::vector<std::string> source_columns;
    static std::vector<int> source_columns_index;
    static std::vector<std::vector<std::string>> destination_columns;
    static std::vector<int> destination_columns_index;
    static std::vector<std::string> destination_column_name;
    static std::vector<std::string> destination_column_type;
    static std::vector<std::string> destination_column_max;
    static std::vector<std::string> destination_column_null;
    static std::vector<std::string> buffer_columns;
    static std::vector<int> buffer_columns_index;
    static std::vector<std::string> data_rows;
    static std::vector<int> data_rows_index;
    static std::vector<std::string> insert_rows;
    static std::vector<std::vector<std::string>>data_parsed_final;      // Store data broken apart into individual strings
    static std::string table_name;
    static std::vector<std::string> query_results;                  // Used by insert builder
    static std::vector<std::string> dup_rows;                       // Used by insert builder
    static std::vector<bool> inserted;                              // Used by insert builder
    static std::vector<std::string> validRows;
    static std::unordered_set<std::string> existingValues;
    static std::vector<std::string> sql_query_message;
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
    static bool dup_check = false;
    static bool new_window = false;
    static bool allow_nulls[1000] = {};
    static bool restrict_duplicates[1000] = {};
    static int display_column_rows = 10;
    static int display_data_rows = 10;
    static bool auto_map_cols = false;
    static bool adduser = false;
    static bool addtime = false;
    static bool show_sql_messages = false;
    static int button_style = 0;
    static bool column_window = display_settings.getColumnWindow();
    static bool data_window = display_settings.getDataWindow();
    static bool insert_window = display_settings.getInsertWindow();
    static bool mapoverview = display_settings.getMappingOverview();

    // Forward thread processing declarations
    static std::future<std::vector<std::string>> columnFuture;
    static std::future<std::vector<std::string>> rowFuture;
    static std::future<std::vector<std::vector<std::string>>> destinationFuture;
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


    enum ButtonStyle
    {
        Button_Comp,
        Button_Expand
    };

    // track boolean states in debug log
    log.logStateChange("loaded_csv", loaded_csv);
    log.logStateChange("load_tables", load_tables);
    log.logStateChange("load_columns", load_columns);
    log.logStateChange("confirm_mapping", confirm_mapping);
    log.logStateChange("ready_to_import", ready_to_import);
    log.logStateChange("confirm_data", confirm_data);
    log.logStateChange("cleanup", cleanup);
    log.logStateChange("data_mapped_check", data_mapped_check);
    log.logStateChange("dup_check", dup_check);
    log.logStateChange("auto_map_cols", auto_map_cols);
    log.logStateChange("column_window", column_window);
    log.logStateChange("data_window", data_window);
    log.logStateChange("insert_window", insert_window);
    log.logStateChange("mapoverview", mapoverview);
    log.logStateChange("isGettingRows", isGettingRows);
    log.logStateChange("isLoadingColumns", isLoadingColumns);
    log.logStateChange("isLoadingCSV", isLoadingCSV);
    log.logStateChange("isLoadingData", isLoadingData);
    log.logStateChange("isProcessingData", isProcessingData);
    log.logStateChange("isLoadingInserts", isLoadingInserts);
    log.logStateChange("dataProcessed", dataProcessed);
    log.logStateChange("isBuildingInserts", isBuildingInserts);
    log.logStateChange("filepath.empty()", filepath.empty());

    ImGui::SameLine();
    ImGui::BeginGroup();
    if (Button("Load CSV", (filepath.empty() || loaded_csv)))  // Only enable button if filepath is !empty OR we have !loaded_csv already
    {
        try
        {
            // Start asynchronous thread to populate columns so we don't freeze program on larger file imports --#13
            columnFuture = std::async(
                std::launch::async,
                getColumns,
                std::filesystem::path(dir + "DataImport\\" + filepath) // Pass the correct argument
            );

            // Start asynchronous thread to populate rows so we don't freeze program on larger file imports --#13
            rowFuture = std::async(
                std::launch::async,
                getRows,
                std::filesystem::path(dir + "DataImport\\" + filepath)
            );
            isLoadingCSV = true;    // Flag the process of loading CSV has started
            isLoadingData = true;
            loaded_csv = true;      // Set loaded flag

            log.AddLog("[INFO] File selected: %s\n", filepath.c_str());
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
        static auto lastCheck = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        // Check futures every 250ms
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck).count() > 250)
        {
            lastCheck = now;
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
                log.AddLog("[INFO] Columns and rows loading finished.\n");
            }
        }
    }

    if (filepath.empty())
        ImGui::SetItemTooltip("File path empty, add a file to DataImport or select a file to load.");
    else if (loaded_csv)
        ImGui::SetItemTooltip("CSV already loaded.");
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();

    // Get destination columns
    ButtonTrigger("Load SQL Tables", &load_tables, (load_tables || !sql._GetConnected())); // Add a button that flips the load_tables trigger and appears disabled if load_tables is true or if we're not connected to a SQL database
    
    ImGui::SameLine();
    if (load_tables)
    {
        ImGui::Text("Select a table to import to: "); ImGui::SameLine();
        ImGui::SetNextItemWidth(175);
        table_name = displayTableNames(sql);
    }
    ImGui::SameLine();
    if (Button("Load Columns", (load_columns || table_name.empty())))
    {
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

    // Set tooltips for button
    if (load_columns) ImGui::SetItemTooltip("Columns loaded!");
    else ImGui::SetItemTooltip("Select a table to load columns.");

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

                // DONE remove addtime and adduser columns before they make it to the mapping table -- #22
                // Perform some initial data parsing/clean up before we start mappinng it (removing adduser and addtime)
                for (int i = 0; i < destination_columns[0].size(); /*i++*/)                                                     // First layer of vectors are the containers for each complete column set
                {
                    if (destination_columns[0][i] == "adduser" || destination_columns[0][i] == "addtime")                       // If the column name is adduser or addtime remove it and each matching index for the schema information
                    {
                        destination_columns[0].erase(destination_columns[0].begin() + i);
                        destination_columns[1].erase(destination_columns[1].begin() + i);
                        destination_columns[2].erase(destination_columns[2].begin() + i);
                        destination_columns[3].erase(destination_columns[3].begin() + i);
                        adduser = true;     // If we find a column adduser or addtime set their flags to true by default
                        addtime = true;
                    }
                    else
                    {
                        i++;                                                                                                    // Only increment when we aren't removing a vector
                    }
                }
                for (int j = 0; j < destination_columns[0].size(); j++)
                {
                    destination_column_name.push_back(destination_columns[0][j]);
                    destination_column_type.push_back(destination_columns[1][j]);
                    destination_column_max.push_back(destination_columns[2][j]);
                    destination_column_null.push_back(destination_columns[3][j]);
                    log.AddLog("[INFO] SQL Table Column loaded: %s\n", destination_column_name[j].c_str());  // Index 0 should be a list of column names
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
    ImGui::EndGroup();
    ImGui::SameLine();
    // Cancel the current import and set all variables back to default
    if (Button("Cancel Import", ImVec2(150, 0)))
    {
        clearMappings(log, 
            source_columns, 
            source_columns_index,
            destination_columns, 
            destination_columns_index,
            buffer_columns,
            buffer_columns_index, 
            data_rows, 
            data_rows_index, 
            insert_rows,
            table_name,
            loaded_csv,
            load_tables, 
            load_columns,
            confirm_mapping, 
            dataProcessed, 
            filepath,
            allow_nulls, 
            restrict_duplicates, 
            confirm_data,
            destination_column_name,
            destination_column_type, 
            destination_column_max,
            destination_column_null,
            query_results,
            dup_rows,
            inserted,
            dup_check,
            adduser,
            addtime,
            sql_query_message,
            data_parsed_final);
    }
    if (Button("Display")) { ImGui::OpenPopup("WindowOptions"); }  // Open a  popup window to display various options related to display of child tables and buttons on the mapping window
    ImGui::SameLine();
    // Display options to include adduser and addtime
    ImGui::Text("Include:"); ImGui::SameLine();
    if (ImGui::Checkbox("adduser", &adduser)) ImGui::SetItemTooltip("This will automatically include adduser to the insert statement with a default value of 'CSTDATAIMPORT'"); ImGui::SameLine();
    if (ImGui::Checkbox("addtime", &addtime)) ImGui::SetItemTooltip("This will automatically include addtime to the insert statement with a default value of today's date and time");

    // Windo containing display options
    if (ImGui::BeginPopup("WindowOptions", ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Display Settings");
        ImGui::Separator();
        ImGui::SeparatorText("Mapping display style:");
        if (ImGui::RadioButton("Expanded", &button_style, Button_Comp)) ImGui::SameLine(); if (ImGui::RadioButton("Compact", &button_style, Button_Expand))
        if (button_style == Button_Comp) { display_settings.setButtonStyle(false); }
        else { display_settings.setButtonStyle(true); }
        ImGui::SeparatorText("Windows");
        if (ImGui::Checkbox("Display Column Mapping", &column_window))
        if (ImGui::Checkbox("Display mapping overview", &mapoverview))
        if (ImGui::Checkbox("Display Data Staging", &data_window))
        if (ImGui::Checkbox("Display Insert Window", &insert_window))

        display_settings.setColumnWindow(column_window);        // Display/hide column mapping child window
        display_settings.setDataWindow(data_window);            // Display/hide data staging child window
		display_settings.setInsertWindow(insert_window);        // Display/hide insert window
		display_settings.setMappingOverview(mapoverview);       // Display/hide mapping overview child window
        log.logStateChange("mapoverview", mapoverview);
        log.logStateChange("column_window", &column_window);
        log.logStateChange("data_window", &data_window);
        log.logStateChange("insert_window", &insert_window);

        // End WindowOptions popup
        ImGui::EndPopup();
    }
    if (column_window)
    {
        ImGui::BeginChild("Column Mapping", ImVec2(575.0f, ImGui::GetContentRegionAvail().y - 275), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar())
        {
            if (load_columns && loaded_csv && !confirm_mapping)
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
			ImGui::TextDisabled("Column Mapping");
            ImGui::Text("Status:");
            if (isLoadingColumns || isLoadingCSV)
            {
                ImGui::Text("Loading Columns... |");
            }
            else if (destination_columns.size() > 0 && source_columns.size() > 0)
            {
                ImGui::Text("All Columns loaded! |");
                // Disable all buttons once data is confirmed
                if (Button("Auto-Match", confirm_mapping))
                {
                    auto_map_cols = true;
                }

                // Disable confirm mapping button if there are not columns mapped
                if (Button("Confirm Mapping", (!data_mapped_check || confirm_mapping)))
                    confirm_mapping = true;
            }
            else
            {
                ImGui::Text("Idle... |");
            }

            // End column mapping menu bar
            ImGui::EndMenuBar();
        }
        if (load_columns && !confirm_mapping && loaded_csv)
        {
            displayMappingTable(log, 
                display_settings, 
                source_columns, 
                destination_column_name,
                destination_column_type, 
                destination_column_max, 
                destination_column_null,
                buffer_columns, 
                data_rows, 
                buffer_columns_index, 
                /*source_columns_index,
                destination_columns_index,
                display_column_rows,*/
                allow_nulls,
                restrict_duplicates,
                &auto_map_cols, 
                !confirm_mapping);
        }
        else
        {
            ImGui::BeginDisabled();
            displayMappingTable(log,
                display_settings,
                source_columns,
                destination_column_name,
                destination_column_type,
                destination_column_max,
                destination_column_null,
                buffer_columns,
                data_rows,
                buffer_columns_index,
                /*source_columns_index,
                destination_columns_index,
                display_column_rows,*/
                allow_nulls,
                restrict_duplicates,
                &auto_map_cols,
                !confirm_mapping);
            ImGui::EndDisabled();
        }
        // End Column Mapping window
        ImGui::EndChild();

        ImGui::SameLine();
    }
    if (mapoverview)
    {
        static bool overview_hovered = false;

        if(overview_hovered)
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.3f));

        // Create new window for mapping overview
        if ((ImGui::BeginChild("Mapping Overview", ImVec2(200, ImGui::GetContentRegionAvail().y - 275),
            ImGuiChildFlags_AlwaysAutoResize |
            ImGuiChildFlags_AutoResizeX |
            ImGuiChildFlags_AutoResizeY |
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_MenuBar)))
        {
            ImVec2 overview_window_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - (ImGui::GetTextLineHeightWithSpacing() * 2));
            if (ImGui::BeginMenuBar())
            {
                ImGui::TextDisabled("Mapping Overview");

                // End Mapping overview menu bar
                ImGui::EndMenuBar();
            }
            // Display an overview of the mapping
            if (ImGui::BeginTable("Overview", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX, overview_window_size))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Destination");
                ImGui::TableSetupColumn("=");
                ImGui::TableSetupColumn("Source");
                ImGui::TableHeadersRow();
                for (int i = 0; i < destination_column_name.size(); i++)
                {
                    if (!buffer_columns[i].empty())
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", destination_column_name[i].c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text("=");
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", buffer_columns[i].c_str());
                    }
                }   // End Mapping overview

                // End Mapping overview table
                ImGui::EndTable();
            }
        }
        
        // End Mapping overview window
        ImGui::EndChild();
        if (overview_hovered)
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered())
        {
            overview_hovered = true;
        }
        else
        {
            overview_hovered = false;
        }
        ImGui::SameLine();
    }

    if (data_window)
    {
        // Display newly mapped rows as a table
        ImGui::BeginChild("Data Staging",
            ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 275), 
            /*ImGuiChildFlags_AlwaysAutoResize |*/ 
            ImGuiChildFlags_Borders | 
            ImGuiChildFlags_AutoResizeY |
            ImGuiChildFlags_AutoResizeX,
            ImGuiWindowFlags_MenuBar
        );
        if (ImGui::BeginMenuBar())
        {
            ImGui::TextDisabled("Data Staging");

            if (ImGui::MenuItem("Confirm Data", NULL, &confirm_data, !confirm_data && confirm_mapping));
            if (ImGui::MenuItem("Popout Window", NULL, &new_window, !confirm_data && confirm_mapping));
            
			ImGui::Text("(Rows: %i)", data_parsed_final.size());
            // End data staging menu bar
            ImGui::EndMenuBar();
        }
        // Only process(formerly clean) data if:
        // isLoadingCSV is complete (aka column headers are loaded and the thread is closed)
        // confirm_mapping - flag set from the menu of column mapping to show the user is finished
        // isLoadingData is complete (row data has been loaded into data_rows and thread is closed)
        // dataProcessed is false - flag that is set when this thread is done and we confirm it's state is ready to indicate processing finished
        // isProcessingData - if we're already running this thread don't start it a second time. Now, we wait until it's done
        if (!isLoadingCSV && confirm_mapping && !isLoadingData && !dataProcessed && !isProcessingData)
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
        else
        {
            if (new_window)
            {
                // Create a "pop out" window to hold the data and allow resizing for easier data review while in staging
                ImGui::Text("Popout window open. Close to return display here.");
				ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
                ImGui::Begin("Data Staging Window", &new_window, ImGuiWindowFlags_NoDocking);
				displayDataTable(log,
					destination_columns,
					destination_columns_index,
					data_rows,
					data_rows_index,
					data_parsed_final,
					destination_column_max,
					display_data_rows,
					allow_nulls,
					!confirm_data);
                ImGui::End();   // End Data staging pop out window
            }
            else
            {
                displayDataTable(log,
                    destination_columns,
                    destination_columns_index,
                    data_rows,
                    data_rows_index,
                    data_parsed_final,
                    destination_column_max,
                    display_data_rows,
                    allow_nulls,
                    !confirm_data);
            }
        }

        // End Staging area window
        ImGui::EndChild();
    }

    // Generate insert queries and store them in vector
    if (!dup_check && confirm_data && insert_rows.size() + dup_rows.size() < data_parsed_final.size())
    {
        if (!isBuildingInserts)
        {
            insertBuilderFuture = std::async(
                std::launch::async,
                buildInsertQuery,
                std::ref(table_name),
                std::ref(insert_rows),
                std::ref(destination_columns_index),
                std::ref(destination_column_name),
                std::ref(data_rows),
                std::ref(data_rows_index),
                std::ref(data_parsed_final),
                adduser,
                addtime,
                std::ref(allow_nulls),
                std::ref(log)
            );

            isBuildingInserts = true;
        }
    }
    if (isBuildingInserts &&
        insertBuilderFuture.valid() &&
        insertBuilderFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
    {
        isBuildingInserts = false;
        log.AddLog("[INFO] Insert builder processed insert statements successfully.\n");
    }

    // Display insert queries that are generated in a new window. Display queries removed as duplicates afterward.
    if (insert_window)
    {
        // Begin child window for inserts that the user can review before pressing the button to run all inserts, as well as choose settings for the inserts
        ImGui::BeginChild("Insert Rows",
            ImVec2(ImGui::GetContentRegionAvail().x, 250),
            ImGuiChildFlags_ResizeY |
            ImGuiChildFlags_Border,
            ImGuiWindowFlags_MenuBar
            );
        static std::atomic<bool> running = false;       // Is duplicate checking running?

        if (confirm_data && !isBuildingInserts)
        {
            for(int i = inserted.size(); i < insert_rows.size(); i++)                   // Populate vector with bools for tracking when they are inserted into SQL or not
            {
                sql_query_message.push_back("");                                        // Push back empty string to match the size of insert_rows
                inserted.push_back(false);
                if (!dup_check && restrict_duplicates[i] && dup_rows.size() < 1)        // Check if we have any duplicates for later
                    dup_check = true;
            }
            if (dup_check)                                                              // If dup_check is true this indicates we need to look for duplicates before showing insert rows
            {
                if(!running)
                {
                    checkForDuplicates_Thread(
                    sql,
                    log,
                    table_name,
                    insert_rows,
                    data_parsed_final,
                    destination_columns_index,
                    destination_columns,
                    restrict_duplicates,
                    dup_rows,
                    inserted,
                    running
                    );
                }

                // Mark that we've checked for duplicates and don't need to look again
                dup_check = false;
            }
        }
        if (ImGui::BeginMenuBar())
        {
            ImGui::TextDisabled("Insert Review");

            if (ImGui::MenuItem("Insert all data", NULL, false, insert_rows.size() > 0))
            {
                try
                {
                    for (int i = 0; i < insert_rows.size(); i++)
                    {
                        if (!inserted[i])
                        {
                            // Start new threads for each insert statement to help with performance
                            std::thread(insertMappedData, sql, insert_rows[i], std::ref(sql_query_message[i])).detach();
                            inserted[i] = true;
                            if(inserted[i])
                                log.AddLog("[INFO] Successfully inserted data into SQL!\n");
                        }
                        else
                        {
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

		ButtonTrigger("Show Insert Results", &show_sql_messages, show_sql_messages); // Add a button that toggles the visibility of the SQL messages window

        // End insert menu bar
        ImGui::EndMenuBar();

        // Show SQL messages window when button is pressed
        if(show_sql_messages)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(150, 150), ImVec2(1920, 1080));
            ImGui::Begin("Insert Results", &show_sql_messages, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar);
            {
				static bool word_wrap = false;
                ImGuiListClipper clipper;
                if(ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("View"))
                    {
                        if (ImGui::MenuItem("Word Wrap", NULL, &word_wrap));

                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                ImGui::Text("Messages:");
                clipper.Begin(sql_query_message.size());
                while(clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    {
                        if (word_wrap)
                        {
                            ImGui::TextWrapped(sql_query_message[i].c_str());
                        }
                        else
                        {
                            ImGui::Text(sql_query_message[i].c_str());
                        }
                    }
                }
				clipper.End();
            }
            // End sql message window
            ImGui::End();
        }

        if(ImGui::BeginTabBar("Insert"))
        {
            if (ImGui::BeginTabItem("Insert Rows"))
            {
                ImGuiListClipper clipper;   // Used to only render displayed items for increased performance
                clipper.Begin(insert_rows.size());
                if (insert_rows.size() >= 1 && !isBuildingInserts && !running)
                {
                    ImGui::Text("Review insert statements generated by tool before running insert into SQL");
                    ImGui::SameLine(); ImGui::Text("(Rows: %i)", insert_rows.size());
                    if (ImGui::BeginTable("InsertRows", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
                    {
                        ImGui::TableSetupColumn("Insert #", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize);
                        ImGui::TableSetupColumn("SQL", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize);
                        ImGui::TableSetupColumn("Insert Statement");
                        ImGui::TableSetupScrollFreeze(1, 1);    // Freeze row counter and SQL insert button
                        ImGui::TableHeadersRow();

                        while (clipper.Step())
                        {
                            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::PushID(i);
                                ImGui::TextDisabled("%i", i);
                                ImGui::TableSetColumnIndex(1);
                                if (Button("SQL", inserted[i]))
                                {
                                    try
                                    {
                                        if (insertMappedData(sql, insert_rows[i], sql_query_message[i]))
                                        {
                                            // query_results.push_back("Insert Successful for insert #" + std::to_string(i));
                                            inserted[i] = true;
                                        }
                                    }
                                    catch (std::exception& e)
                                    {
                                        //query_results.push_back(e.what());
                                        inserted[i] = false;
                                        sql_query_message[i] = e.what();        // Capture exception message to display after insert attempt
                                    }
                                    ImGui::SetItemTooltip("Insert this row into SQL");
                                } ImGui::SameLine();
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", insert_rows[i].c_str());

                                // Pop row ID value
                                ImGui::PopID();
                            }
                        }
                        // End insert statment table
                        ImGui::EndTable();
                    }clipper.End();    // End the list clipper rendering
                }
                else if (running)
                {
					ImGui::Text("Checking for duplicate data...");
                }
                else
                {
                    ImGui::Text("No valid data for entry! Check duplicate rows tab.");
                }
                // End Insert Rows tab item
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Duplicate Rows"))
            {
                ImGui::Text("Data that was found to be duplicated and removed from insert list:");
				ImGui::SameLine(); ImGui::Text("(Rows: %i)", dup_rows.size());
                if(ImGui::BeginTable("Duplicates",2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
                {
					ImGui::TableSetupColumn("Duplicate #", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("Duplicate Rows");
                    ImGui::TableSetupScrollFreeze(1, 0);
                    ImGui::TableHeadersRow();
                    if (dup_rows.size() > 0)
                    {
                        for (int i = 0; i < dup_rows.size(); i++)
                        {
                            ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::Text("%i", i);
							ImGui::TableSetColumnIndex(1);
                            ImGui::Text(dup_rows[i].c_str());
                        }
                    }
                    ImGui::EndTable();
                }
                else
                    ImGui::Text("No dupliate data found!");
                ImGui::EndTabItem();
            }
            // End Duplicate Rows tab item
            ImGui::EndTabBar();
        }
        // End Insert Rows child window
        ImGui::EndChild();
    }
}