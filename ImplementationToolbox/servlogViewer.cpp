#include "servlogViewer.h"
#include "Sql.h"
#include "AppLog.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "systemFunctions.h"
#include "UserSettings.h"
#include <numeric>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>  
#include <iomanip>  

// Global or static variables
static std::atomic<bool> refresh(false);        // Indicates if auto-refresh is enabled
static std::atomic<int> refresh_freq(0);        // Frequency in seconds
static std::atomic<bool> view_button(false);    // Indicates when the timer expires
static std::atomic<int> countdown(0);           // Track the countdown in seconds to next refresh
static std::thread refresh_thread;              // Timer thread
static std::atomic<bool> stop_thread(false);    // Flag to stop the thread

void servlogViewer(bool* open, Sql& sql, AppLog& log, UserSettings& settings)  
{  
	ServlogTable servlog;
	const int COLUMNS = 8;
    const int MAX_RETURN = 1000;
	static int result_quantity;
	static int view_quantity;
	static std::string filter_text;
    static bool view = false;
    static std::string column_order;
    static std::string where = "";
    static bool addwhere = false;
    // Specific column filters
    static bool
        servlogid_filt = true,
        service_filt = true,
        product_filt = true,
        logtime_filt = true,
        logtype_filt = true,
        descriptn_filt = true,
        computer_filt = true;
    static bool changed = false;
    bool frameskip = true;      // Used for logging assistance to let the switch statements happen before we log to debug logger
    // Asc/Desc
    static int ascdesc = 0;
    static std::string ascendingdescending;
    static int order_by = 1;
    static bool
        servlogid_ord = true,
        service_ord = true,
        product_ord,
        logtime_ord,
        logtype_ord,
        descriptn_ord,
        computer_ord;
	// SQL results storage
	static std::vector<int>
		servlogid;
	static std::vector<std::string>
		service,
		product,
		logtime,
		descriptn,
        logtype,
		computer;
	static int results[1000];
    static int 
        hovered_row,
        hovered_column;
    static std::vector<int> filtered_rows;
    static int filtered_rows_tmp = 0;
    static int displayed_rows;
    static ImGuiTextFilter filter;

    // Log bool state changes
    log.logStateChange("view", view);
    log.logStateChange("refresh", refresh.load());
    log.logStateChange("addwhere", addwhere);
    log.logStateChange("servlogid_filt", servlogid_filt);
    log.logStateChange("service_filt", service_filt);
    log.logStateChange("product_filt", product_filt);
    log.logStateChange("logtime_filt", logtime_filt);
    log.logStateChange("logtype_filt", logtype_filt);
    log.logStateChange("descrptn_filt", descriptn_filt);
    log.logStateChange("computer_filt", computer_filt);
    log.logStateChange("view_button", view_button.load());
    log.logStateChange("stop_thread", stop_thread.load());

    // SQL connection required before app can be used - disable if not connected
    if (!sql._GetConnected())
        ImGui::BeginDisabled();

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));  
   ImGui::BeginChild("servlogtop", ImVec2(ImGui::GetWindowSize().x, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_MenuBar);
   
   // Show the menu bar across the top of the window
   showMenuBar(servlogid, service, product, logtime, logtype, descriptn, computer, filter, log);

   // Int input for user to select row count returned
   ImGui::SetNextItemWidth(100);
   static int rows_buf = 0;
   ImGui::InputInt("Row Count", &rows_buf, 1, 100);
   if (ImGui::IsItemDeactivatedAfterEdit())
   {
       if(rows_buf!= result_quantity)
       {
           result_quantity = rows_buf;
           log.AddLog("[INFO] Row count set to %i\n", result_quantity);
       }
   }
   ImGui::SetItemTooltip("Max 1000 rows");
   ImGui::SameLine();
   // Auto-refresh checkbox
   
    bool refresh_value = refresh.load();
    log.logStateChange("refresh_value", refresh_value);
    if (ImGui::Checkbox("Auto-refresh", &refresh_value))
    {
        refresh.store(refresh_value);
        if (refresh_value)
        {
            StartTimerThread(log); // Start the timer thread
            log.AddLog("[INFO] Countdown value set to: %i seconds\n", countdown.load());
        }
        else
        {
            log.AddLog("[INFO] Attempting to stop automatic refresh thread.\n");
            StopTimerThread(log); // Stop the timer thread
        }
        ImGui::SetItemTooltip("WARNING may cause application latency!");
    }
    ImGui::SameLine();

    if (refresh && sql._GetConnected())
    {
        static int frequency_buf = 0; // Initialize buffer with 0
        ImGui::SetNextItemWidth(50);
        ImGui::InputInt("Frequency(seconds)", &frequency_buf, 0, 0);
        ImGui::SetItemTooltip("WARNING may cause application latency!");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            refresh_freq.store(frequency_buf); // Update the atomic variable
            log.AddLog("[INFO] Refresh frequency updated to: %i seconds\n", refresh_freq.load());
        }

        // Start the timer thread if not already running
        if (!refresh_thread.joinable())
        {
            log.AddLog("[DEBUG] Timing thread not running, attempting to start timer.\n");
            StartTimerThread(log);
        }

        ImGui::SameLine();
        // Display the countdown
        int remaining_time = countdown.load();
        ImGui::Text("Next refresh in: %d seconds", remaining_time);
    }
    else
    {
        refresh_freq.store(0); // Reset frequency if auto-refresh is disabled
        ImGui::BeginDisabled();
        int frequency_buf = 0;
        refresh_freq.store(0);
        ImGui::SetNextItemWidth(50);
        ImGui::InputInt("Frequency(seconds)", &frequency_buf, 0, 0);
        // Display the countdown
        int remaining_time = countdown.load();
        ImGui::SameLine();
        ImGui::Text("Next refresh in: %d seconds", remaining_time);
        ImGui::EndDisabled();

        // Stop the timer thread if refresh is disabled
        StopTimerThread(log);
    }

    // Maintain min/max count
    if (result_quantity > MAX_RETURN)			// If we go over 1000 reset to 1000
    {
        result_quantity = MAX_RETURN; rows_buf = MAX_RETURN;
    }
    else if (result_quantity < 1)		// If we go below 1 reset to 1
    {
        result_quantity = 1; rows_buf = 1;
    }

    // Add query/filtering options here
    switch (order_by)
    {
    case 1:
        column_order = "servlogid";
        break;
    case 2:
        column_order = "service";
        break;
    case 3:
        column_order = "product";
        break;
    case 4:
        column_order = "logtime";
        break;
    case 5:
        column_order = "logtype";
        break;
    case 6:
        column_order = "descriptn";
        break;
    case 7:
        column_order = "computer";
        break;
    default:
        column_order = "servlogid";
        break;
    }
    switch (ascdesc)
    {
    case 0:
        ascendingdescending = "ASC";
        if (changed) { frameskip = false; changed = false; }
        break;
    case 1:
        ascendingdescending = "DESC";
        if (changed) { frameskip = false; changed = false; }
        break;
    }
    if (ImGui::Button("WHERE"))
    {
        addwhere = true;
    }

    where = AddWhere(log, addwhere);

    ImGui::SameLine();
    if (ImGui::Button("ORDER BY"))
    {
        ImGui::OpenPopup("OrderBy");
    }
    if(ImGui::BeginPopup("OrderBy"))
    {
        ImGui::BeginGroup();    // Group for column ordering
        ImGui::Text("Order By"); ImGui::Separator();
        ImGui::RadioButton("servlogid", &order_by, 1); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::RadioButton("service", &order_by, 2); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::RadioButton("product", &order_by, 3); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::RadioButton("logtime", &order_by, 4); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::RadioButton("logtype", &order_by, 5); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::BeginDisabled(); ImGui::RadioButton("desriptn", &order_by, 6); ImGui::EndDisabled();
        ImGui::RadioButton("computer", &order_by, 7); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); frameskip = false; }
        ImGui::EndGroup();      // End column ordering group
        ImGui::SameLine();
        ImGui::BeginGroup();    // Group for ascending/descending
        ImGui::Text("ASC/DESC");
        ImGui::RadioButton("Ascending", &ascdesc, 0); if (ImGui::IsItemClicked() && frameskip) { log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascendingdescending.c_str()); changed = true; }
        ImGui::RadioButton("Descending", &ascdesc, 1);
        ImGui::EndGroup();      // End ascending/descending group
        ImGui::EndPopup();
    }
    
    if(where == "")
    {
        ImGui::TextWrapped("Query to be run on view: SELECT TOP %i servlogid, service, product, logtime, logtype, descriptn, computer FROM servlog WITH (NOLOCK) ORDER BY %s %s", result_quantity, column_order.c_str(), ascendingdescending.c_str());
    }
    else
    {
        ImGui::TextWrapped("Query to be run on view: SELECT TOP %i servlogid, service, product, logtime, logtype, descriptn, computer FROM servlog WITH (NOLOCK) %s ORDER BY %s %s", result_quantity, where.c_str(), column_order.c_str(), ascendingdescending.c_str());
    }

    // Add spacing to push the button to the bottom  
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

    ImGui::SameLine();
    ImGui::Separator();
    // This will be the button to run the select query against servlog with any provided parameters
    if (ImGui::Button("View"))
    {
        view_button.store(true); // Set the atomic variable
    }
    // Check if the button or timer fired to rerun queries and get data
    if (view_button.load())
    {
        log.AddLog("[INFO] View button clicked/timer fired - attempting to get new values from servlog.\n");
        try
        {
            // Get fresh data from servlog
            if(where == "")
                servlog.setServlogData(sql, column_order, result_quantity, ascdesc);
            else
                servlog.setServlogData(sql, column_order, result_quantity, ascdesc, where);

            log.AddLog("[INFO] Updating local storage with values from servlog.\n");
            // Store servlog data locally for display
            servlogid = servlog.getServlogid();
            service = servlog.getService();
            product = servlog.getProduct();
            logtime = servlog.getLogtime();
            logtype = servlog.getLogtype();
            descriptn = servlog.getDescriptn();
            computer = servlog.getComputer();

            // Set view to true to say we've ran the query at least once to display table
            view = true;

            // Set view_button back to false so e don't keep running this until the timer fires or button is pressed
            view_button.store(false);

            // Set view quantity to store the amount of records that were selected at the time the view button was pressed.
            log.AddLog("[INFO] Checking row count against actual record count returned when querying servlog.\n");
            // This way, if the user changes the row count before hitting view we don't crash
            if (result_quantity > servlogid.size())  // Use servlogid to make sure we returned as many records as we're requesting
            {
                log.AddLog("[WARN] More rows requested than servlog contains. Setting row count to servlog size: %i.\n", servlogid.size());
                view_quantity = servlogid.size();    // If not we will crash going out of bounds. Use the size of servlogid instead since there will always be 1 servlogid to a row
            }
            else
                view_quantity = result_quantity;
            displayed_rows = view_quantity;          // Init displayed_rows with view_quantity before taking out filtered rows
            log.AddLog("[INFO] Successfully retrieved and stored servlog records locally.\n");
        }
        catch (std::exception& ex)
        {
            log.AddLog("[ERROR] Unable to get data from servlog: %s", ex.what());
            view_button.store(false);
            view = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        servlogid.clear();
        service.clear();
        product.clear();
        logtime.clear();
        logtype.clear();
        descriptn.clear();
        computer.clear();
        filter.Clear();
        view_quantity = 0;
        displayed_rows = 0;
    }
    ImGui::SameLine();
    if (ImGui::BeginPopup("Filters"))
    {
        ImGui::Text("Filter Columns");
        ImGui::Separator();
        if (ImGui::Button("All", ImVec2(50, 0)))
        {
            servlogid_filt = true;
            service_filt = true;
            product_filt = true;
            logtime_filt = true;
            logtype_filt = true;
            descriptn_filt = true;
            computer_filt = true;
        } ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(50, 0)))
        {
            servlogid_filt = false;
            service_filt = false;
            product_filt = false;
            logtime_filt = false;
            logtype_filt = false;
            descriptn_filt = false;
            computer_filt = false;
        }         
        ImGui::Checkbox("servlogid", &servlogid_filt);
        ImGui::Checkbox("service", &service_filt);
        ImGui::Checkbox("product", &product_filt);
        ImGui::Checkbox("logtime", &logtime_filt);
        ImGui::Checkbox("logtype", &logtype_filt);
        ImGui::Checkbox("descriptn", &descriptn_filt);
        ImGui::Checkbox("computer", &computer_filt);

        // End Filters popup
        ImGui::EndPopup();
    }
    
    // DONE: Add filter settings to exclude/include specific columns for filtering - utlized ImGui table context menu to allow right-clicking the headers to hide the unwanted ones
    ImGui::SameLine(); if (ImGui::Button("Filter")) { ImGui::OpenPopup("Filters"); }; ImGui::SameLine();
    filter.Draw("##Filter, (Filter records here include,-exclude)", ImGui::GetContentRegionAvail().x);

    // End Child window for top of servlog  
    ImGui::EndChild();

    ImVec2 table_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5f);
    // Begin SQL table structure for servlog
    if (ImGui::BeginTable("servlogsql", 
        COLUMNS, 
        ImGuiTableFlags_SizingFixedFit | 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_Borders | 
        ImGuiTableFlags_ScrollY | 
        ImGuiTableFlags_ScrollX | 
        ImGuiTableFlags_Sortable | 
        ImGuiTableFlags_HighlightHoveredColumn | 
        ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ContextMenuInBody, 
        table_size))
    {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn("servlogid", ImGuiTableColumnFlags_DefaultSort, ImGuiSortDirection_Ascending);
        ImGui::TableSetupColumn("service");
        ImGui::TableSetupColumn("product");
        ImGui::TableSetupColumn("logtime");
        ImGui::TableSetupColumn("logtype");
        ImGui::TableSetupColumn("descriptn");
        ImGui::TableSetupColumn("computer");
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableHeadersRow();

        // Handle sorting  
        // DONE finish sorting on the rest of the columns
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
        {
            if (sort_specs->SpecsDirty)
            {
                const ImGuiTableColumnSortSpecs* spec = sort_specs->Specs;
                if (spec->ColumnIndex == 1) // servlogid
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(servlogid.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return servlogid[a] < servlogid[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return servlogid[a] > servlogid[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 2) // service
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(service.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return service[a] < service[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return service[a] > service[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 3) // product
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(product.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return product[a] < product[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return product[a] > product[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 4) // logtime
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(logtime.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return logtime[a] < logtime[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return logtime[a] > logtime[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 5) // logtype
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(logtype.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return logtype[a] < logtype[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return logtype[a] > logtype[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 6) // descriptn
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(descriptn.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return descriptn[a] < descriptn[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return descriptn[a] > descriptn[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                else if (spec->ColumnIndex == 7) // computer
                {
                    // Create an index vector to track the original order
                    std::vector<size_t> indices(computer.size());
                    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

                    // Sort the indices based on the selected column
                    if (spec->SortDirection == ImGuiSortDirection_Ascending)
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return computer[a] < computer[b];
                            });
                    }
                    else
                    {
                        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                            return computer[a] > computer[b];
                            });
                    }

                    // Reorder all columns based on the sorted indices
                    auto reorder = [&](auto& column) {
                        std::vector<std::decay_t<decltype(column[0])>> temp(column.size());
                        for (size_t i = 0; i < indices.size(); ++i)
                        {
                            temp[i] = column[indices[i]];
                        }
                        column = std::move(temp);
                        };

                    reorder(servlogid);
                    reorder(service);
                    reorder(product);
                    reorder(logtime);
                    reorder(logtype);
                    reorder(descriptn);
                    reorder(computer);
                }
                // Reset sort_specs Dirty to false since they are sorted now
                sort_specs->SpecsDirty = false;
            }
        }
        static int j = 0;
        static int f = 0;
        if (view)
        {
            for (int i = 0; i < view_quantity; i++)
            {
                // DONE filter specific columns 
                if (
                    filter.PassFilter(std::to_string(servlogid[i]).c_str()) && servlogid_filt ||
                    filter.PassFilter(service[i].c_str()) && service_filt ||
                    filter.PassFilter(product[i].c_str()) && product_filt ||
                    filter.PassFilter(logtime[i].c_str()) && logtime_filt ||
                    filter.PassFilter(logtype[i].c_str()) && logtype_filt ||
                    filter.PassFilter(descriptn[i].c_str()) && descriptn_filt ||
                    filter.PassFilter(computer[i].c_str()) && computer_filt
                    )
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImGui::BeginDisabled();
                    ImGui::Text("%i", j + 1);
                    ImGui::EndDisabled();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%i", servlogid[i]);
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(std::to_string(servlogid[i]).c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", std::to_string(servlogid[i]).c_str());
                    }
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", service[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(service[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", service[i].c_str());
                    }
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", product[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(product[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", product[i].c_str());
                    }
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text(logtime[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(logtime[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", logtime[i].c_str());
                    }
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%s", logtype[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(logtype[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", logtype[i].c_str());
                    }
                    // Column 7 index
                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text(descriptn[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(descriptn[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", descriptn[i].c_str());
                    }
                    // Column 8 Index
                    ImGui::TableSetColumnIndex(7);
                    ImGui::Text("%s", computer[i].c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        ImGui::SetClipboardText(computer[i].c_str());
                        log.AddLog("[INFO] Copied text \"%s\" to clipboard.\n", computer[i].c_str());
                    }

                    hovered_row = ImGui::TableGetHoveredRow();
                    hovered_column = ImGui::TableGetHoveredColumn();

                    if (hovered_row == j + 1)
                    {
                        if (settings.getDarkMode() == 1)
                        {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(66, 150, 255, 150));    // Light blue color
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(103, 52, 235, 200), hovered_column);
                        }
                        else
                        {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(153, 204, 255, 255));   // Light blue color
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(103, 52, 235, 200), hovered_column);

                        }
                    }
                    j++;   // increment after displaying a row for hover highlight accuracy with filter
                }
                else
                {
                    f++;   // Add 1 value for filtered rows that are skipped over
                }

                displayed_rows = view_quantity - f;
            }
            j = 0;     // Reset row counter
            f = 0;     // Reset filtered row counter
        }
        // End Results table  
        ImGui::EndTable();
    }
    ImVec2 table_info_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing());
    if (ImGui::BeginTable("tablestats", 6, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersInnerV, table_info_size))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));      // Always display black text here since we keep the yellow background in either color setting
        ImGui::TableNextRow();
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, 102, 255));
        ImGui::TableNextColumn();
        // Display status text of SQL connection
        ImGui::Text("Status:"); ImGui::SameLine();
        if (sql._GetConnected())
            DisplayColoredText("Connected!", sql._GetConnected());
        else
            DisplayColoredText("Not Connected.", sql._GetConnected());
        ImGui::TableNextColumn();
        // Right align all text after the status field
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(sql._GetSource().c_str()).x - ImGui::GetStyle().CellPadding.x);
        ImGui::Text(sql._GetSource().c_str());
        ImGui::TableNextColumn();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(sql._GetUsername().c_str()).x - ImGui::GetStyle().CellPadding.x);
        ImGui::Text(sql._GetUsername().c_str());
        ImGui::TableNextColumn();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(sql._GetDatabase().c_str()).x - ImGui::GetStyle().CellPadding.x);
        ImGui::Text(sql._GetDatabase().c_str());
        ImGui::TableNextColumn();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize("Row: 1000, Column: 8").x - ImGui::GetStyle().CellPadding.x);
        if (hovered_row == -1 || hovered_column == -1)
        {
			ImGui::Text("Row: N/A, Column: N/A");
        }
        else
        {
            ImGui::Text("Row: %i, Column: %i", hovered_row, hovered_column);
        }
        ImGui::TableNextColumn();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize("1000 rows").x - ImGui::GetStyle().CellPadding.x);
        ImGui::Text("%i rows", displayed_rows);
        ImGui::PopStyleColor();    // End black text styling
        // End Table Info table
        ImGui::EndTable();
    }
    // End 10,10 window padding within child windows
    ImGui::PopStyleVar();  

    // Ensure the thread is stopped when the application exits
    if (!*open)
    {
        log.AddLog("[INFO] Detected module closure, attempting to stop any open timing threads.\n");
        StopTimerThread(log);
    }

    if (!sql._GetConnected())
    {
        refresh.store(false);
        refresh_freq.store(0);
        ImGui::EndDisabled();
    }
}   // End Servlogviewer

// Function to start the timer thread  
void StartTimerThread(AppLog& log)  
{  
   // Stop any existing thread  
   log.AddLog("[INFO] Stopping any existing threads\n");  
   StopTimerThread(log);  

   // Start a new thread  
   stop_thread.store(false);  
   refresh_thread = std::thread([&log]() { // Capture log by reference  
       while (!stop_thread.load())  
       {  
           if (refresh.load() && refresh_freq.load() > 0)  
           {  
               log.AddLog("[DEBUG] Starting countdown\n");  
               countdown.store(refresh_freq.load()); // Initialize countdown to refresh frequency  
               for (int i = refresh_freq.load(); i > 0 && !stop_thread.load(); --i)  
               {  
                   std::this_thread::sleep_for(std::chrono::seconds(1));  
                   countdown.store(i - 1); // Decrement countdown  
                   log.AddLog("[DEBUG] Countdown updated: %d seconds remaining\n", i - 1);  
               }  
               log.AddLog("[INFO] Countdown finished, flipping view_button variable to run query.\n");  
               if (!stop_thread.load())  
               {  
                   view_button.store(true); // Flip the bool when the timer expires  
                   log.AddLog("[INFO] Successfully started timing thread\n");  
               }  
           }  
           else  
           {  
               countdown.store(0); // Reset countdown if refresh is disabled  
               log.AddLog("[DEBUG] Refresh disabled or frequency is zero, sleeping briefly.\n");  
               std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Avoid busy-waiting  
           }  
       }  
       log.AddLog("[INFO] Timer thread exiting.\n");  
   });  
}

void showMenuBar(const std::vector<int>& servlogid,
                const std::vector<std::string>& service,
                const std::vector<std::string>& product,
                const std::vector<std::string>& logtime,
                const std::vector<std::string>& logtype,
                const std::vector<std::string>& descriptn,
                const std::vector<std::string>& computer,
                const ImGuiTextFilter& filter,
                AppLog& log)
{
   if (ImGui::BeginMenuBar())
   {
       if (ImGui::BeginMenu("File"))
       {
           if (ImGui::BeginMenu("Save", !refresh))
           {
               if (ImGui::Selectable("Save to CSV"))
               {
                   SaveFilteredDataToCSV(servlogid, service, product, logtime, logtype, descriptn, computer, filter, log);
               }
               // End save menu
               ImGui::EndMenu();
           }
           if (refresh)
               ImGui::SetItemTooltip("Disable auto-refresh to enable save.");

           // End File menu
           ImGui::EndMenu();
       }

       // End Menu bar
       ImGui::EndMenuBar();
   }
}

// Function to stop the timer thread
void StopTimerThread(AppLog& log)
{
    if (refresh_thread.joinable())
    {
        stop_thread.store(true);
        refresh_thread.join();
        log.AddLog("[INFO] Successfully stopped open timing thread\n");
    }
}

// Get servlogid from servlog in struct
void ServlogTable::setServlogid(Sql& sql, int quant)
{
	servlogid = sql.returnIntQry(sql._GetConnectionString(), "servlogid", "servlog", quant);
}

std::vector<int> ServlogTable::getServlogid()
{
	return servlogid;
}

void ServlogTable::setService(Sql& sql, int quant)
{
	service = sql.returnStrQry(sql._GetConnectionString(), "service", "servlog", quant, 0);
}

std::vector<std::string> ServlogTable::getService()
{
	return service;
}

void ServlogTable::setProduct(Sql& sql, int quant)
{
	product = sql.returnStrQry(sql._GetConnectionString(), "product", "servlog", quant, 0);
}

std::vector<std::string> ServlogTable::getProduct()
{
	return product;
}

void ServlogTable::setLogtime(Sql& sql, int quant)
{
	logtime = sql.returnDtStrQry(sql._GetConnectionString(), "logtime", "servlog", quant);
}

std::vector<std::string> ServlogTable::getLogtime()
{
	return logtime;
}

std::vector<std::string> ServlogTable::getLogtype()
{
    return logtype;
}

void ServlogTable::setDescriptn(Sql& sql, int quant)
{
    descriptn = sql.returnStrQry(sql._GetConnectionString(), "descriptn", "servlog", quant, 0);
}

std::vector<std::string> ServlogTable::getDescriptn()
{
    return descriptn;
}

void ServlogTable::setComputer(Sql& sql, int quant)
{
    computer = sql.returnStrQry(sql._GetConnectionString(), "computer", "servlog", quant, 0);
}

std::vector<std::string> ServlogTable::getComputer()
{
    return computer;
}

void ServlogTable::setServlogData(Sql& sql, std::string column, int quantity, int ascdesc /*std::vector<int>& servlogid, std::vector<std::string>& service, std::vector<std::string>& product, std::vector<std::string>& logtime, std::vector<std::string>& descriptn, std::vector<std::string>& computer, std::vector<char>& logtype*/)
{
    sql.servLogQuery(sql._GetConnectionString(), column, quantity, ascdesc, servlogid, service, product, logtime, logtype, descriptn, computer);
}

void ServlogTable::setServlogData(Sql& sql, std::string column, int quantity, int ascdesc, std::string where)
{
    sql.servLogQuery(sql._GetConnectionString(), column, quantity, ascdesc, where, servlogid, service, product, logtime, logtype, descriptn, computer);
}



void SaveFilteredDataToCSV(const std::vector<int>& servlogid,
    const std::vector<std::string>& service,
    const std::vector<std::string>& product,
    const std::vector<std::string>& logtime,
    const std::vector<std::string>& logtype,
    const std::vector<std::string>& descriptn,
    const std::vector<std::string>& computer,
    const ImGuiTextFilter& filter,
    AppLog& log)
{
    const std::string filePath = "C:\\ImplementationToolbox\\servlog.csv";

    try
    {
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            log.AddLog("[ERROR] Unable to open file for writing: %s\n", filePath.c_str());
            return;
        }

        // Write CSV header  
        file << "servlogid,service,product,logtime,logtype,descriptn,computer\n";

        // Write rows that pass the filter  
        for (size_t i = 0; i < servlogid.size(); ++i)
        {
            if (
                filter.PassFilter(std::to_string(servlogid[i]).c_str()) ||
                filter.PassFilter(service[i].c_str()) ||
                filter.PassFilter(product[i].c_str()) ||
                filter.PassFilter(logtime[i].c_str()) ||
                filter.PassFilter(logtype[i].c_str()) ||
                filter.PassFilter(descriptn[i].c_str()) ||
                filter.PassFilter(computer[i].c_str())
                )
            {
                file << servlogid[i] << ','
                    << '"' << service[i] << '"' << ','
                    << '"' << product[i] << '"' << ','
                    << '"' << logtime[i] << '"' << ','
                    << '"' << logtype[i] << '"' << ','
                    << '"' << descriptn[i] << '"' << ','
                    << '"' << computer[i] << '"' << '\n';
            }
        }

        file.close();
        log.AddLog("[INFO] Successfully saved data to: %s\n", filePath.c_str());
    }
    catch (const std::exception& ex)
    {
        log.AddLog("[ERROR] Exception occurred while saving to CSV: %s\n", ex.what());
    }
}

std::string AddWhere(AppLog& log, bool& where_open)
{
    static std::string tmp_where = "";
    static std::string where = "";
    static std::string logtime_start, logtime_end;
    const float BUTTON_WIDTH = 50.0f;
    static int selection = 0;
    if(where_open)
        ImGui::OpenPopup("Custom Where");

    if (ImGui::BeginPopupModal("Custom Where", &where_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::RadioButton("Date Range", &selection, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Custom", &selection, 1);
        if (selection == 0)
        {
            ImGui::SetNextItemWidth(200);
            ImGui::InputTextWithHint("##starttime", "Ex: 2025-01-01 00:00:00", &logtime_start); ImGui::SameLine(); ImGui::Text("AND"); ImGui::SameLine(); ImGui::SetNextItemWidth(200);
            ImGui::InputTextWithHint("##endtime", "Ex: 2025-01-01 23:59:59", &logtime_end);
        }

        if (selection == 1)
        {
            ImGui::Text("Enter a custom where statement to add onto the servlog query.\n!!DO NOT INCLUDE WHERE OR ORDER BY ONLY THE STATEMENT ITSELF!!");
            ImGui::Text("WHERE"); ImGui::SameLine();
            ImGui::InputTextMultiline("##WHERE", &tmp_where /*ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 40.0f)*/);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));  // Reduce padding in buttons to make CLEAR fit better with other buttons

        if (ImGui::Button("DONE", ImVec2(BUTTON_WIDTH, 0)))
        {
            if (selection == 0)
                where = "WHERE logtime BETWEEN '" + logtime_start + "' AND '" + logtime_end + "'";
            else
                where = "WHERE " + tmp_where;
            log.AddLog("[INFO] Custom WHERE added to select statement: %s\n", where.c_str());

            // End PushStyleVar early
            ImGui::PopStyleVar();

            // End group early if DONE is clicked
            //ImGui::EndGroup();

            // Close popup when change accepted
            ImGui::CloseCurrentPopup();

            // End popup early when editing where value and clicking done
            ImGui::EndPopup();

            // Set to false so we don't reopen window
            where_open = false;

            // return value entered
            return where;
        } ImGui::SameLine();
        if (ImGui::Button("CLEAR", ImVec2(BUTTON_WIDTH, 0)))
        {
            if (selection == 0)
            {
                logtime_end = "";
                logtime_start = "";
                where = "";
            }
            else
            {
                tmp_where = "";
                where = "";
            }
        } ImGui::SameLine();
        if (ImGui::Button("CLOSE", ImVec2(BUTTON_WIDTH, 0)))
        {
            where_open = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();

            // End Where popup
        ImGui::EndPopup();
    }
    // Default return
    return where;
}


void LogChanges(AppLog& log, int* column, int* order)
{
    std::string ascdesc = "";
    std::string column_order;
    static int column_int = 0;
    static int order_by = 0;
    switch (*column)
    {
    case 1:
        column_order = "servlogid";
        break;
    case 2:
        column_order = "service";
        break;
    case 3:
        column_order = "product";
        break;
    case 4:
        column_order = "logtime";
        break;
    case 5:
        column_order = "logtype";
        break;
    case 6:
        column_order = "descriptn";
        break;
    case 7:
        column_order = "computer";
        break;
    default:
        column_order = "servlogid";
        break;
    }
    switch (*order)
    {
    case 0:
        ascdesc = "ASC";
        break;
    case 1:
        ascdesc = "DESC";
        break;
    default:
        break;
    }
    if(order_by != *order || column_int != *column)
    {
        order_by = *order;
        column_int = *column;
        log.AddLog("[INFO] Order by %s %s\n", column_order.c_str(), ascdesc.c_str());
    }
}