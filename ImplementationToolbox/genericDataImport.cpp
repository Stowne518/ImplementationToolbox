#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include "imgui_stdlib.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <chrono>
#include <unordered_set>
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
                    loaded_csv = true;      // Set loaded flag
                    log.AddLog("[INFO] Columns and rows loading finished.\n");
                }
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
                            log.AddLog("[INFO] SQL Table Column loaded: %s\n", destination_column_name[j]);  // Index 0 should be a list of column names
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
    if (ImGui::Button("Display"))
    {
        ImGui::OpenPopup("WindowOptions");
    }
    ImGui::SameLine();
    // Display options to include adduser and addtime
    ImGui::Text("Include:"); ImGui::SameLine();
    if (ImGui::Checkbox("adduser", &adduser)); ImGui::SetItemTooltip("This will automatically include adduser to the insert statement with a default value of 'CSTDATAIMPORT'"); ImGui::SameLine();
    if (ImGui::Checkbox("addtime", &addtime)); ImGui::SetItemTooltip("This will automatically include addtime to the insert statement with a default value of today's date and time");

    if (ImGui::BeginPopup("WindowOptions", ImGuiWindowFlags_NoMove))
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
                if (confirm_mapping)
                {
                    ImGui::BeginDisabled();

                    if (ImGui::Button("Auto-Match"))
                    {
                        auto_map_cols = true;
                    }
                    // Disable confirm mapping button if there are not columns mapped
                    if (!data_mapped_check)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("Confirm Mapping"))
                    {
                        confirm_mapping = true;
                    }

                    if (!data_mapped_check)
                        ImGui::EndDisabled();


                    ImGui::EndDisabled();
				}
                else
                {
                    if (ImGui::Button("Auto-Match"))
                    {
                        auto_map_cols = true;
                    }
                    // Disable confirm mapping button if there are not columns mapped
                    if (!data_mapped_check)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("Confirm Mapping"))
                    {
                        confirm_mapping = true;
                    }
                    if (!data_mapped_check)
                        ImGui::EndDisabled();
                }
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
                source_columns_index, 
                destination_columns_index,
                display_column_rows,
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
                source_columns_index,
                destination_columns_index,
                display_column_rows,
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
                        ImGui::Text("%s", destination_column_name[i]);
                        ImGui::TableNextColumn();
                        ImGui::Text("=");
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", buffer_columns[i]);
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
            if (confirm_mapping)
            {
                if (ImGui::MenuItem("Confirm Data", NULL, &confirm_data, !confirm_data));
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Confirm Data", NULL, &confirm_data);
                ImGui::EndDisabled();
            }
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
        static std::atomic<bool> running = false;       // Is duplicate checking running

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
        if (ImGui::Button("Show Insert Results"))
        {
			show_sql_messages = true;
        }

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
                                if (!inserted[i])
                                {
                                    // Create button to allow individual lines to be submittedd to SQL after it's generated
                                    if (ImGui::Button("SQL"))
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
                                    }
                                    ImGui::SetItemTooltip("Insert this row into SQL");
                                }
                                else
                                {
                                    ImGui::BeginDisabled();
                                    ImGui::Button("SQL");
                                    ImGui::SetItemTooltip("Inserted successfully!");
                                    ImGui::EndDisabled();
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
	dataImport.close();     // close the file
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
    rowData.close();        // Close the file
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

void displayMappingTable(AppLog& log, 
    DisplaySettings& ds, 
    std::vector<std::string>& s_columns, 
    std::vector<std::string>& d_columns_name, 
    std::vector<std::string>& d_columns_type, 
    std::vector<std::string>& d_columns_max, 
    std::vector<std::string>& d_columns_null, 
    std::vector<std::string>& b_columns, 
    std::vector<std::string>& rows,
    std::vector<int>& b_column_index, 
    std::vector<int>& s_columns_index,
    std::vector<int>& d_column_index, 
    int& table_len, 
    bool* nulls, 
    bool* duplicate,
    bool* auto_map,
    bool editable)
{
    std::string value;
    static std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    ImVec2 button_style = ds.getButtonStyle();
    ImGuiListClipper d_clipper;
    ImGuiListClipper s_clipper;
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
        
        copied = true;
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

    if (*auto_map)   // If we click auto-map button run this once
    {
        for (int i = 0; i < d_columns_name.size(); i++)     // Loop the destination columns against all source column labels
        {
            for (int j = 0; j < s_columns_buf.size(); j++)  // Test agains the buffer so we don't directly change source_columns
            {
                if (d_columns_name[i] == s_columns_buf[j])  // Test destination labels against source, if we find one perform mappings before we display table again
                {
                    log.AddLog("[INFO] Auto-match detected match on %s\n", s_columns_buf[j].c_str());
                    // Map out buffer values first
                    b_column_index[i] = j;                  // bufffer_column_index at pos i gets value of current j. This should be the source buf index position of matching label - this will correlate the matching buffer column to SQL with the matching source column
                    log.AddLog("[DEBUG] Mapped source column %s to SQL column %s\n", s_columns_buf[j].c_str(), d_columns_name[i].c_str());
                    b_columns[i] = s_columns_buf[j];        // Set buffer column at i equal to label of source column buffer at j. 
                    s_columns_buf[j] = "";                  // Set newly mapped source column buffer index to blank to indicate a swamp with the b_column
                    if (d_columns_type[i] == "datetime" || d_columns_null[i] == "YES")
                    {
                        log.AddLog("[DEBUG] Datetime or nullable field detected. Marking column null.\n");
                        nulls[i] = true;                    // If column is a datetime field or is nullable auto-mark nullable when mapping
                    }
                    if (i < d_columns_name.size() - 1)
                        log.AddLog("[INFO] Moving to next SQL column %s\n", d_columns_name[i + 1].c_str());
                    break;                                  // End inner loop because we found what we needed - start checking next column
                }
            }
        }
        *auto_map = false;       // Reset to false so we don't keep attempting to auto_map columns after first pass
    }
    d_clipper.Begin(d_columns_name.size());
	//ImGui::BeginChild("DestinationColumnMapping", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
    ImVec2 mapping_tables_outer_size = ImVec2(ImGui::GetContentRegionAvail().x / 2 - 3.60, ImGui::GetContentRegionAvail().y);
    if (ImGui::BeginTable("DestinationMappingTable", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, mapping_tables_outer_size))
    {
        ImGui::TableSetupColumn("Null", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn("Dup", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Table Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Mapped Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        while(d_clipper.Step())
        {
            for (int i = d_clipper.DisplayStart; i < d_clipper.DisplayEnd; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(i);
                ImGui::Checkbox("##nulls", &nulls[i]);
                ImGui::SetItemTooltip("Check this box to include NULLs instead of blanks for data in column '%s'.", d_columns_name[i]);
                log.logStateChange(("%s", d_columns_name[i] + " allows nulls").c_str(), nulls[i]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Checkbox("##Dups", &duplicate[i]);
                ImGui::SetItemTooltip("Check this box to check for duplicates for data in column '%s' before inserting.", d_columns_name[i]);
                log.logStateChange(("%s", d_columns_name[i] + " restricts duplicates on insert.").c_str(), duplicate[i]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("  %s", d_columns_name[i]);
                ImGui::SetItemTooltip("Data Type: %s\nMax Len: %s\nNullable: %s", d_columns_type[i].c_str(), d_columns_max[i].c_str(), d_columns_null[i].c_str());
                ImGui::TableSetColumnIndex(3);
                if (editable)
                {
                    ImGui::Button(b_columns[i].c_str(), button_style);

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
                            if (s_columns_buf[payload_n] != "")
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
                                // Skip if label is blank
                                if (b_columns[i] != "" && b_columns[j] != s_columns[b_column_index[j]])
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
                else
                {
                    ImGui::Text("%s", b_columns[i].c_str());
                }
                ImGui::PopID();
            }
		}d_clipper.End();    // End the list clipper rendering
        // End column mapping
        ImGui::EndTable();
    }
    
	// End destination column mapping child window
    //ImGui::EndChild();
    ImGui::SameLine();
	s_clipper.Begin(s_columns.size());
    //ImGui::BeginChild("SourceColumnMapping", ImVec2(0,0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, mapping_tables_outer_size))
    {
        ImGui::TableSetupColumn(" Source Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Sample data", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        while(s_clipper.Step())
        {
            for (int i = s_clipper.DisplayStart; i < s_clipper.DisplayEnd; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (editable)
                {
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
                }
                else
                {
                    ImGui::Text("%s", s_columns_buf[i].c_str());
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", /*s_columns[i].c_str(),*/ values[i].c_str());
            }
        } s_clipper.End();
        
		// End source column mapping table
        ImGui::EndTable();
    }     
    
    // End Source column mapping
    //ImGui::EndChild();
}

// Function to process data after dependencies are completed
void processData(std::vector<std::string>& data_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& buffer_columns_index,
    std::vector<int>& destination_columns_index,
    std::vector<int>& data_rows_index,
    AppLog& log, 
    bool& cleanup)
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
		std::vector<std::string> values = parseCSVLine(data_rows[i]); // Parse out the CSV line into a vector of strings
        if(!emptyStrings(values))     // Detect if the previously parsed line was empty, if so don't add to data_parsed_final -- #22
        {
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
        }
        else
        {
            log.AddLog("[WARN] Detected empty line in row %i of the CSV file. Skipping...\n", i);
        }
    }
    cleanup = true; // Tell application cleanup has been completed and we won't revisit this section
}

void buildInsertQuery(std::string table_name, 
    std::vector<std::string>& insert_rows, 
    std::vector<int>& d_columns_index, 
    std::vector<std::string>& d_columns, 
    std::vector<std::string>& rows, 
    std::vector<int>& rows_index, 
    std::vector<std::vector<std::string>>& data_parsed_final, 
    bool adduser,
    bool addtime,
    bool* nulls, 
    AppLog& log)
{
	// DONE: Add multi-threading for large files to prevent UI from freezing and allow each insert to print to log as it's written
    // DONE: Changing logic here to only grab mapped data
    // Added integer vectors for tracking index positions of columns mapped, and columns from the table.    
    try
    {
        do
        {
            for (int i = 0; i < data_parsed_final.size(); i++)
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
                if(addtime && adduser)
                {
                    query += " , adduser, addtime"; // Add adduser and addtime at the end of the insert  statement -- will probably need to make this a parameter later
                }
                else if (adduser)
                {
					query += " , adduser"; // Add adduser at the end of the insert statement
                }
                else if (addtime)
                {
					query += " , addtime"; // Add addtime at the end of the insert statement
                }
                query += ") VALUES (";
                for (int j = 0; j < data_parsed_final[i].size(); j++)
                {
					values.push_back(data_parsed_final[i][j]); // Push back the data from the parsed data vector
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
                if(addtime && adduser)
                {
                    query += ",'CSTDATAIMPORT', GETDATE()"; // Add adduser value and run SQL function GETDATE to insert current date/time
				}
				else if (adduser)
				{
					query += ",'CSTDATAIMPORT'"; // Add adduser value
				}
				else if (addtime)
				{
					query += ",GETDATE()"; // Run SQL function GETDATE to insert current date/time
				}
                query += ");";
                insert_rows.push_back(query);
                log.AddLog("[INFO] Insert query generated: %s\n", query.c_str());
            }
        } while (insert_rows.size() < data_parsed_final.size());
    }
	catch (const std::exception& e)
	{
		log.AddLog("[ERROR] Failed to build insert query: %s\n", e.what());
	}
}

/// <summary>
/// Uses data collected from an imported CSV and mapped column headers to build the table where we display the data we mapped and the order it was mapped in.
/// </summary>
/// <param name="log"> - Pass your AppLog object to allow the ability to write log lines as the data is processed for debugging</param>
/// <param name="d_columns"> - vector containing all column headers pulled from the loaded SQL table.</param>
/// <param name="d_columns_index"> - vector containing index positions of each column that was mapped.</param>
/// <param name="rows"> - vector containing data that was read in from the CSV. By this point it should be reduced to only contain data from columns we have mapped to.</param>
/// <param name="rows_index"> - vector containing index positions from the CSV of data columns to track which SQL column it shoudl belong in.</param>
void displayDataTable(
    AppLog& log,
    std::vector<std::vector<std::string>>& d_columns,
    std::vector<int>& d_columns_index,
    std::vector<std::string>& rows,
    std::vector<int>& rows_index,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<std::string>& d_column_max,
    int& table_len,
    bool* nulls,
    bool editable)
{
    bool changed = false;                                                           // Bool to track if a change was made to a field in the stage table
    const int TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 outer_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y);
    //ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * table_len);
    ImGuiListClipper clipper_x;
    // Begin table for displaying and editing actual data, as well as showing it was mapped correctly.
    // DONE fix table display for large imports, fix display for mapping overview to not be in the way(maybe make it it's own window?)
    if (ImGui::BeginTable(
        "DataStaging",
        d_columns_index.size() + 1,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_ContextMenuInBody |
        ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersV,
        outer_size)
        )
    {
        // Setup mapped columns as headers
        ImGui::TableSetupColumn("row",
            ImGuiTableColumnFlags_WidthFixed |
            ImGuiTableColumnFlags_NoResize |
            ImGuiTableColumnFlags_NoHide |
            ImGuiTableColumnFlags_NoHeaderLabel |
            ImGuiTableColumnFlags_NoSort);
        for (int i = 0; i < d_columns_index.size(); i++)
        {
            ImGui::TableSetupColumn(d_columns[0][d_columns_index[i]].c_str());      // Use mapped column names from the SQL table to set up the headers
        }
        ImGui::TableSetupScrollFreeze(1, 1);                                        // Make headers row always visible
        ImGui::TableHeadersRow();                                                   // Tells UI this will be our headers for the table

        clipper_x.Begin(data_parsed_final.size());                                        // Begin the list clipper to handle large data sets
        while (clipper_x.Step())
        {
            // Set up data rows
            for (int i = clipper_x.DisplayStart; i < clipper_x.DisplayEnd; i++)                          // Begin looping through each row of data
            {
                ImGui::TableNextRow();                                                  // Start new row each loop
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled(std::to_string(i).c_str());                         // Create a row counter that we freeze on the side for easy tracking
                // Push row ID to each row
                ImGui::PushID(i);
                for (int k = 0; k < d_columns_index.size(); k++)                        // Begin looping through the columns to populate data in each one
                {
                    ImGui::TableSetColumnIndex(k + 1);                                  // Set the next columns index up
                    // Push value ID of k to each value
                    ImGui::PushID(k);
                    if (editable)
                    {
                        if (nulls[d_columns_index[k]] && data_parsed_final[i][k] == "")
                        {
                            data_parsed_final[i][k] = "NULL";                               // If we marked a column as null required while mapping and it has a blank fill in NULL instead
                        }
                        // DONE: Fix blank row editing probably need to append column name as well as index pos to field
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        std::string tmp = data_parsed_final[i][k];                          // Store current value of field before edits are made for logging
                        ImVec2 textSize = ImGui::CalcTextSize(tmp.c_str());
                        if (tmp != "")
                        {
                            ImGui::SetNextItemWidth(textSize.x + 10);                       // Set width of input box to be the same as the text size
                        }
                        else
                            ImGui::SetNextItemWidth(50);       // Default size if empty string
                        if (d_column_max[d_columns_index[k]] != "")
                        {
                            if (tmp.size() > std::stoi(d_column_max[d_columns_index[k]]))
                            {
                                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));   // If size of string is larger than the column can handle we set a red border around cell
                                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                            }
                        }

                        ImGui::InputText("", &data_parsed_final[i][k], ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase);         // Create box for input lines to apppear in. This displays imported data and allows edits to be made before sending it
                        if (ImGui::IsItemDeactivatedAfterEdit())
                        {
                            changed = true;
                            log.AddLog("[DEBUG] Modified row %i, column %s. Old value: %s; New value: %s;\n", i, (d_columns[0][d_columns_index[k]]).c_str(), tmp.c_str(), data_parsed_final[i][k].c_str());
                            tmp = data_parsed_final[i][k];
                        }

                        if (d_column_max[d_columns_index[k]] != "")
                        {
                            if (tmp.size() > std::stoi(d_column_max[d_columns_index[k]]) && d_column_max[d_columns_index[k]] != "")
                            {
                                ImGui::SetItemTooltip("Column %s exceeds max length of %s", d_columns[0][d_columns_index[k]].c_str(), d_column_max[d_columns_index[k]].c_str());
                                ImGui::PopStyleColor();
                                ImGui::PopStyleVar();
                            }
                        }
                    }
                    else
                    {
                        ImGui::Text("%s", data_parsed_final[i][k].c_str());                       // If data is not editable we display text only
                    }
                    // Pop value ID of k
                    ImGui::PopID();
                }
                // Pop row ID of i
                ImGui::PopID();
            }
        }clipper_x.End();
        
        // End data table
        ImGui::EndTable();
    }
}

bool insertMappedData(Sql& sql, std::string query, std::string& message)
{
	// Execute the query to insert the data
	bool success = sql.executeQuery(query, message);
	return success;
}

void clearMappings(AppLog& log, 
    std::vector<std::string>& source_columns, 
    std::vector<int>& source_columns_index, 
    std::vector<std::vector<std::string>>& destination_columns, 
    std::vector<int>& destination_columns_index, 
    std::vector<std::string>& buffer_columns, 
    std::vector<int>& buffer_columns_index, 
    std::vector<std::string>& data_rows, 
    std::vector<int>& data_rows_index, 
    std::vector<std::string>& insert_rows, 
    std::string& table_name, 
    bool& loaded_csv, 
    bool& load_tables, 
    bool& load_columns,
    bool& confirm_mapping,
    bool& data_processed,
    std::string& filepath,
    bool* nulls,
    bool* dups,
    bool& confirm_data,
    std::vector<std::string>& d_col_name,
    std::vector<std::string>& d_col_type,
    std::vector<std::string>& d_col_max, 
    std::vector<std::string>& d_col_null,
    std::vector<std::string>& query_results,
    std::vector<std::string>& dup_rows,
    std::vector<bool>& inserted,
    bool& dup_check,
    bool& adduser, 
    bool& addtime,
    std::vector<std::string>& sql_message,
    std::vector<std::vector<std::string>>& data_parsed_final)
{
    // Clear null flags
    for (int i = 0; i < 1000; i++)
    {
        if(nulls[i])
            nulls[i] = false;
		if (dups[i])
            dups[i] = false;
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
    d_col_name.clear();
    d_col_max.clear();
    d_col_null.clear();
    d_col_type.clear();
    query_results.clear();
    dup_rows.clear();
    inserted.clear();
    sql_message.clear();
    data_parsed_final.clear();
    
    // Reset all flags
    loaded_csv = false;
    load_tables = false;
    load_columns = false;
    confirm_mapping = false;
    confirm_data = false;
    data_processed = false;
    dup_check = false;
    adduser = false;
    addtime = false;
    filepath.clear();

    // Log the clearing action
    log.AddLog("[INFO] All vectors and flags have been cleared.\n");
}

// Function to parse a single line of CSV data
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string token;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            // Toggle the insideQuotes flag when encountering a quote
            insideQuotes = !insideQuotes;
        }
        else if (c == ',' && !insideQuotes) {
            // If we encounter a comma outside quotes, finalize the current token
            token = trim(token); // Remove leading/trailing/spaced strings
            result.push_back(token);
            token.clear();
        }
        else {
            // Append the character to the current token
            token += c;
        }
    }

    // Add the last token to the result
    result.push_back(token);

    return result;
}

// Helper function to trim spaces at the beginning and end of a string
std::string trim(const std::string& str) {
    const auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == std::string::npos)
        return ""; // No content

    const auto strEnd = str.find_last_not_of(" ");
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

// Function to check if a vector of strings is empty
bool emptyStrings(std::vector<std::string> vec)
{
    bool vectorEmpty = true;                        // Set vectorEmpty to true by default to make the function prove its false
	for (const auto& str : vec)                     // Loop through each string within the vector
	{
        if (!str.empty())
        {
			return vectorEmpty = false;             // As soon as we find a string that isn't empty we can return since we know the vector isn't empty
        }
    }

    return vectorEmpty;                             // Otherwise we return true to show the vector was empty
}

void checkForDuplicates(
    Sql& sql,
    AppLog& log,
    std::string table_name,
    std::vector<std::string>& insert_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& destination_columns_index,
    std::vector<std::vector<std::string>>& destination_columns,
    bool* restrict_duplicates,
    std::vector<std::string>& dup_rows,
    std::vector<bool>& inserted,
    std::atomic<bool>& running
)
{
    // Initialize the vector to store valid rows
    std::vector<std::string> validRows;
    std::lock_guard<std::mutex> lock(sqlMutex);
    running = true;
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
                // TODO optimize SQL query for checking for duplicates by sending all columns at once and checking for only 1 records in the DB instead of all of them
                if (sql.checkDuplicate(table_name, destination_columns[0][destination_columns_index[j]], data_parsed_final[i][j]) > 0)
                {
                    // Add row to new vector for rows that contain illegal duplicate data
                    dup_rows.push_back(insert_rows[i]);
                    duplicate_count++;
                    break;  // Break if we find a duplicate on this column since the whole row is thrown out we can stop checking
                }
            }
        }
        if (duplicate_count == 0)
        {
            validRows.push_back(insert_rows[i]);    // Only keep rows that pass the duplicate check
        }
    }
    running = false;
    insert_rows = validRows;    // Update the insert_rows vector to only contain valid rows
    return;                     // End function after setting new insert_rows value
}

void checkForDuplicates_Thread(
    Sql& sql,
    AppLog& log,
    std::string table_name,
    std::vector<std::string>& insert_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& destination_columns_index,
    std::vector<std::vector<std::string>>& destination_columns,
    bool* restrict_duplicates,
    std::vector<std::string>& dup_rows,
    std::vector<bool>& inserted,
    std::atomic<bool>& running)
{
	std::thread(
        checkForDuplicates, 
        std::ref(sql),
        std::ref(log),
        std::move(table_name),
        std::ref(insert_rows),
        std::ref(data_parsed_final),
        std::ref(destination_columns_index),
        std::ref(destination_columns),
        restrict_duplicates,
        std::ref(dup_rows),
        std::ref(inserted),
        std::ref(running)
    ).detach();
}

