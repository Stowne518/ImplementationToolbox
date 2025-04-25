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
	static int result_quantity;
	static int view_quantity;
	static std::string filter_text;
    static bool view = false;
	// SQL results storage
	static std::vector<int>
		servlogid;
	static std::vector<char> 
        logtype;
		static std::vector<std::string>
		service,
		product,
		logtime,
		descriptn,
		computer;
	static int results[1000];
    static int 
        hovered_row,
        hovered_column;
    static ImGuiTextFilter filter;

    // Log bool state changes
    log.logStateChange("view", view);
    log.logStateChange("refresh", refresh.load());
    log.logStateChange("view_button", view_button.load());
    log.logStateChange("stop_thread", stop_thread.load());

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));  
   ImGui::BeginChild("servlogtop", ImVec2(ImGui::GetWindowSize().x, 0), ImGuiChildFlags_AlwaysUseWindowPadding /*| ImGuiChildFlags_Border*/ | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_None);
   
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
   if (sql._GetConnected())
   {
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
       }
       ImGui::SetItemTooltip("WARNING may cause application latency!");
   }
   else
   {
       refresh.store(false);
       ImGui::BeginDisabled();
       bool refresh_value = false;
       log.logStateChange("refresh_value", refresh_value);
       ImGui::Checkbox("Auto-refresh", &refresh_value);
       ImGui::EndDisabled();
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
   if (result_quantity > 1000)			// If we go over 1000 reset to 1000
	   result_quantity = 1000;
   else if (result_quantity < 1)		// If we go below 1 reset to 1
	   result_quantity = 1;

   // Add spacing to push the button to the bottom  
   ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));  

   if(sql._GetConnected())
   {
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
               servlog.setServlogid(sql, result_quantity);
               servlog.setService(sql, result_quantity);
               servlog.setProduct(sql, result_quantity);
               servlog.setLogtime(sql, result_quantity);
               servlog.setLogtype(sql, result_quantity);
               servlog.setDescriptn(sql, result_quantity);
               servlog.setComputer(sql, result_quantity);
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

               log.AddLog("[INFO] Successfully retrieved and stored servlog records locally.\n");
           }
           catch (std::exception& ex)
           {
               log.AddLog("[ERROR] Unable to get data from servlog: %s", ex.what());
               view_button.store(false);
               view = false;
           }
	   }
       
   }
   else
   {
	   ImGui::BeginDisabled();
	   ImGui::Button("View");
       ImGui::SetTooltip("Connect to SQL server to continue");
	   ImGui::EndDisabled();
   }
   // DONE: Add filter settings to exclude/include specific columns for filtering - utlized ImGui table context menu to allow right-clicking the headers to hide the unwanted ones
   ImGui::SameLine(); ImGui::Text("Filter"); ImGui::SameLine();
   filter.Draw("##Filter", ImGui::GetContentRegionAvail().x);

   // End Child window for top of servlog  
   ImGui::EndChild();  

   // Open a second child window for the lower half of the window which will be for the table view
   //ImGui::BeginChild("servlogbottom", ImVec2(ImGui::GetWindowSize().x, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
   ImVec2 table_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5);
   // Begin SQL table structure for servlog
   if (ImGui::BeginTable("servlogsql", COLUMNS, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_Sortable | ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg /*| ImGuiTableFlags_NoSavedSettings*/, table_size))  
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

         if (view)  
         {  
             for (int i = 0; i < view_quantity; i++)  
             {
                 // TODO filter specific columns 
                 if(filter.PassFilter(std::to_string(servlogid[i]).c_str()) ||
                    filter.PassFilter(service[i].c_str()) ||
                    filter.PassFilter(product[i].c_str()) ||
                    filter.PassFilter(logtime[i].c_str()) || 
                    filter.PassFilter(std::to_string(logtype[i]).c_str()) ||
                    filter.PassFilter(descriptn[i].c_str()) ||
                    filter.PassFilter(computer[i].c_str())
                 )
                 {
                     ImGui::TableNextRow();
                     ImGui::TableSetColumnIndex(0);
                     ImGui::BeginDisabled();
                     ImGui::Text("%i", i + 1);
                     ImGui::EndDisabled();
                     ImGui::TableSetColumnIndex(1);
                     ImGui::Text("%i", servlogid[i]);
                     ImGui::TableSetColumnIndex(2);
                     ImGui::Text("%s", service[i].c_str());
                     ImGui::TableSetColumnIndex(3);
                     ImGui::Text("%s", product[i].c_str());
                     ImGui::TableSetColumnIndex(4);
                     ImGui::Text(logtime[i].c_str());
                     ImGui::TableSetColumnIndex(5);
                     ImGui::Text("%c", logtype[i]);
                     ImGui::TableSetColumnIndex(6);
                     ImGui::Text(descriptn[i].c_str());
                     ImGui::TableSetColumnIndex(7);
                     ImGui::Text("%s", computer[i].c_str());
                     if (ImGui::TableGetHoveredRow() == i + 1)
                     {
                         if(settings.getDarkMode() == 1)
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(66, 150, 255, 150));    // Light gray color
                         else
                             ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(153, 204, 255, 255));   // Light blue color
                         hovered_row = ImGui::TableGetHoveredRow() + 1;
                     }
                     hovered_column = ImGui::TableGetHoveredColumn();
                 }
             }  
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
         ImGui::Text("Connection Status:"); ImGui::SameLine();
         if (sql._GetConnected())
             DisplayColoredText("Connected!", sql._GetConnected());
         else
             DisplayColoredText("Not Connected.", sql._GetConnected());
         ImGui::TableNextColumn();
         ImGui::Text(sql._GetSource().c_str());
         ImGui::TableNextColumn();
         ImGui::Text(sql._GetUsername().c_str());
         ImGui::TableNextColumn();
         ImGui::Text(sql._GetDatabase().c_str());
         ImGui::TableNextColumn();
         ImGui::Text("Row: %i, Column: %i", hovered_row - 1, hovered_column);
         ImGui::TableNextColumn();
         ImGui::Text("%i rows", servlogid.size());
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

// Ensure the thread is stopped when the application exits
//struct ThreadGuard
//{
//    ~ThreadGuard()
//    {
//        StopTimerThread();
//    }
//} thread_guard;

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
	service = sql.returnStrQry(sql._GetConnectionString(), "service", "servlog", quant);
}

std::vector<std::string> ServlogTable::getService()
{
	return service;
}

void ServlogTable::setProduct(Sql& sql, int quant)
{
	product = sql.returnStrQry(sql._GetConnectionString(), "product", "servlog", quant);
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

void ServlogTable::setLogtype(Sql& sql, int quant)  
{  
   // Convert string to char  
   std::vector<std::string> tmp = sql.returnStrQry(sql._GetConnectionString(), "logtype", "servlog", quant);
   std::vector<char> char_vector_buff;  

   for (const auto& str : tmp)  
   {  
       if (!str.empty())  
       {  
           char buff = str[0]; // Extract the first character of the string  
           char_vector_buff.push_back(buff);  
       }  
   }  
   logtype = char_vector_buff;
}

std::vector<char> ServlogTable::getLogtype()
{
	return logtype;
}

void ServlogTable::setDescriptn(Sql& sql, int quant)
{
	descriptn = sql.returnStrQry(sql._GetConnectionString(), "descriptn", "servlog", quant);
}

std::vector<std::string> ServlogTable::getDescriptn()
{
	return descriptn;
}

void ServlogTable::setComputer(Sql& sql, int quant)
{
	computer = sql.returnStrQry(sql._GetConnectionString(), "computer", "servlog", quant);
}

std::vector<std::string> ServlogTable::getComputer()
{
	return computer;
}

