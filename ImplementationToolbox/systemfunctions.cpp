#include "systemFunctions.h"

#include "AppLog.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>

/// <summary>
/// Function used to check the main directory path exists for us to save various files into it. Returns true if it is able to create the directory or already exists. Returns false if it can't create it and doesn't exist
/// </summary>
/// <param name="path">= the directory path that we check to save profiles to</param>
/// <returns></returns>
bool createDirectory(std::string& path, AppLog& log) {
    std::string sqlOutputSubDir = path + "SQL Output\\";
    std::string profileOutputDir = path + "Profiles\\";
    std::string dataImportDir = path + "DataImport\\";

    // Unit Paths
    std::string unitCSVDir = path + "Units\\Unit\\";
    std::string agencyCSVDir = path + "Units\\Agencies\\";
    std::string groupsCSVDir = path + "Units\\Groups\\";

    // Map Paths
    std::string
        map_dir = path + "Maps\\",
        beats_dir = map_dir + "Beats\\",
        district_dir = map_dir + "Districts\\",
        station_dir = map_dir + "Stations\\",
        geoprox_dir = map_dir + "GeoProx\\",
        responseplan_dir = map_dir + "ResponsePlans\\",
        reportingarea_dir = map_dir + "ReportingAreas\\";

    std::string connStrDir = path + "ConnectionStrings\\";
    // Check for the primary directory we will use for program if not there we try to create it
    try
    {
        if (!std::filesystem::exists(path)) {
            log.AddLog("[INFO] Working directory not found... attempting to create at: %s\n", path.c_str());
            // Try and create it
            if (std::filesystem::create_directory(path)) {
				log.AddLog("[INFO] Working directory successfully created at: %s\n", path.c_str());
                // Create sql output sub directory so we can save out output files there.
                std::filesystem::create_directory(sqlOutputSubDir);
				log.AddLog("[INFO] SQL Output directory successfully created at: %s\n", sqlOutputSubDir.c_str());
                // Create profile sub directories
                std::filesystem::create_directory(profileOutputDir);
				log.AddLog("[INFO] Profile Output directory successfully created at: %s\n", profileOutputDir.c_str());
                // Create connection string directory
                std::filesystem::create_directory(connStrDir);
                log.AddLog("[INFO] Connection String directory successfully created at: %s\n", connStrDir.c_str());
                std::filesystem::create_directory(dataImportDir);
				log.AddLog("[INFO] Data Import directory successfully created at: %s\n", dataImportDir.c_str());
                // Return true if we make it here without exception
                log.AddLog("[INFO] All directories successfully created.\n");
                return true;               
            }
            else {
                log.AddLog("[INFO] Working Directory already exists.\n");
                return false;
            }
        }
	}
	catch (std::filesystem::filesystem_error& fse)
	{
		log.AddLog("[ERROR] Filesystem error: %s\n", fse.what());
		return false;
	}
	catch (std::exception& e)
	{
		log.AddLog("[ERROR] General error: %s\n", e.what());
		return false;
	}
    // Create subdirectories if they don't exists in the working dir
    try
    {
        if (!std::filesystem::exists(sqlOutputSubDir))
        {
			// Create sql output sub directory so we can save out output files there.
			log.AddLog("[INFO] SQL Output directory not found... attempting to create at: %s\n", sqlOutputSubDir.c_str());
            std::filesystem::create_directory(sqlOutputSubDir);
			log.AddLog("[INFO] SQL Output directory successfully created at: %s\n", sqlOutputSubDir.c_str());
        }
        if (!std::filesystem::exists(profileOutputDir))
        {
			// Create profile sub directories
			log.AddLog("[INFO] Profile Output directory not found... attempting to create at: %s\n", profileOutputDir.c_str());
            std::filesystem::create_directory(profileOutputDir);
			log.AddLog("[INFO] Profile Output directory successfully created at: %s\n", profileOutputDir.c_str());
        }
        if (!std::filesystem::exists(connStrDir))
        {
			// Create connection string directory
			log.AddLog("[INFO] Connection String directory not found... attempting to create at: %s\n", connStrDir.c_str());
            std::filesystem::create_directories(connStrDir);
			log.AddLog("[INFO] Connection String directory successfully created at: %s\n", connStrDir.c_str());
        }
        if (!std::filesystem::exists(dataImportDir))
        {
			// Create data import directory
			log.AddLog("[INFO] Data Import directory not found... attempting to create at: %s\n", dataImportDir.c_str());
            std::filesystem::create_directory(dataImportDir);
			log.AddLog("[INFO] Data Import directory successfully created at: %s\n", dataImportDir.c_str());
        }
        // Return true if we make it here without exception
		log.AddLog("[INFO] All subdirectories successfully found/created.\n");
        return true;
    }
    catch (std::filesystem::filesystem_error& fse)
    {
        log.AddLog("[ERROR] Filesystem error: %s\n", fse.what());
        return false;
    }
    catch (std::exception& e)
    {
        log.AddLog("[ERROR] General error: %s\n", e.what());
        return false;
    }

    // Default return
    return false;
}

// Function to check in the list of generic export fields were correctly read in.
bool genericFieldCheck() {
    std::vector<std::string> fieldTest;

    fieldTest = readFieldList();
    if (fieldTest.empty())
        return false;
    else
        return true;

    // Return false by default
    return false;
}

// Display text as either green or red by passing true/false respectively
void DisplayColoredText(const char* text, bool isGreen) {
    ImVec4 color = isGreen ? ImVec4(0.0f, 0.75f, 0.0f, 1.0f) : ImVec4(0.75f, 0.0f, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

void displayUpdates() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, (ImVec2(0, 5)));
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::Bullet(); ImGui::TextWrapped("Added new directories for Data Import CSV files in the working directory.");
        ImGui::Bullet(); ImGui::TextWrapped("New Feature: User settings.\nThese settings are now allowing for more customization than before. Windows can now be affixed to anchor points in the main window or to other modules. This creates a viewport as well, so windows can now be moved outside the main display area. It will also track the position and size of the main window and reopen to those specifications each time the program is opened now.");
        ImGui::Bullet(); ImGui::TextWrapped("New feature: Debug logging. This window can be opened from the main menu bar under view. It will show the current state of the program and any errors that occur.\nThis should allow for reduced testing time and faster troubleshooting moving forward.");
        ImGui::Bullet(); ImGui::TextWrapped("Debug log has to be retroactively added to support all corners of the program. This will be an ongoing process and it is still limited to the following areas:");
        
        ImGui::Indent(); ImGui::Bullet(); ImGui::TextWrapped("SQL Connection");
        ImGui::Bullet(); ImGui::TextWrapped("Directory creation at launch.");
        ImGui::Bullet(); ImGui::TextWrapped("Most main screen variables");
        ImGui::Bullet(); ImGui::TextWrapped("Generic Data Import");
        ImGui::Bullet(); ImGui::TextWrapped("User Settings");
        ImGui::Bullet(); ImGui::TextWrapped("System Functions");
        ImGui::Bullet(); ImGui::TextWrapped("Servlog viewer");
        ImGui::Unindent();

        ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Added new feature for SQL quick connection. It will read in the list of saved connection strings (if you have any) and then attempt to connect automatically usinng the one you click on.");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Added new text filters to drop downs in Generic data import and SQL quick connection options.");
        ImGui::Spacing();
    }
    ImGui::SeparatorText("RMS/JMS");
    if (ImGui::CollapsingHeader("Generic Export Generator")) {
        ImGui::Spacing();
        ImGui::Bullet(); ImGui::TextWrapped("No updates this version.");
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("One Button Database Refresh")) {
        ImGui::Spacing();
        ImGui::Bullet(); ImGui::TextWrapped("No updates this version.");
        ImGui::Spacing();
    }
    ImGui::SeparatorText("CAD");
    if (ImGui::CollapsingHeader("Servlog Viewer"))
    {
        ImGui::Spacing();
        ImGui::TextWrapped("New Feature: Servlog Viewer");
        ImGui::Bullet(); ImGui::TextWrapped("This module is a display window for the SQL table in the CAD database called servlog");
        ImGui::Bullet(); ImGui::TextWrapped("It's a fully functional database view window that allows you to run a query against the database directly from the application and view the results.");
        ImGui::Bullet(); ImGui::TextWrapped("Some features of this module include:");
        ImGui::Indent();
        ImGui::Bullet(); ImGui::TextWrapped("Custom where clauses or logtime specific where clauses.");
        ImGui::Bullet(); ImGui::TextWrapped("Order by any column ascending or descending.");
        ImGui::Bullet(); ImGui::TextWrapped("Filtering available across all results.You can also selectively filter on specific columns.");
        ImGui::Bullet(); ImGui::TextWrapped("SQL style status bar on the bottom row showing information like connection status or server / user you're connected with.");
        ImGui::Unindent();
        ImGui::Bullet(); ImGui::TextWrapped("This module also features an auto-refresh timer, for continued monitoring of the table as interfaces run to get real-time updates as they are happening without needing a SQL trace or continually monitoring a SQL query page and manually hitting refresh.");
        ImGui::Spacing();
    }
    ImGui::SeparatorText("SQL");
    if (ImGui::CollapsingHeader("Generic Data Import"))
    {
        ImGui::Spacing();
        ImGui::TextWrapped("New Feature: Generic Data Import");
        ImGui::Bullet(); ImGui::TextWrapped("The module will read in a CSV file and allow you to map the columns to the database table.");
        ImGui::Bullet(); ImGui::TextWrapped("The module will also allow you to select the table you want to import into after connecting to SQL.");
        ImGui::Bullet(); ImGui::TextWrapped("After loading CSV and SQL Tables you can begin column mapping by either dragging and dropping source columns onto the SQL columns, or use the auto-map feature to attempt and automatically handle 1:1 matches between column names.");
        ImGui::Bullet(); ImGui::TextWrapped("Added ability to check for duplicates in the SQL table before inserting the data.");
        ImGui::Bullet(); ImGui::TextWrapped("Added ability to set columns as nullable before inserting data.This will also change any blank fields to NULL in the data staging table before creating insert statements.");
        ImGui::Bullet(); ImGui::TextWrapped("Data staging table now checks for column max length, and if it detects a field with a larger size than the max it will highlight the field in red for review.");
        ImGui::Bullet(); ImGui::TextWrapped("Insert review table is now split up between two tabs. Insert rows shows all insert statements generated that pass duplicate validation, the duplicate rows tab shows the rows that were removed after finding a duplicate match on one of the columns marked restricted for duplicates.");
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("SQL Query Builder")) {
        ImGui::Spacing();
        ImGui::Bullet(); ImGui::TextWrapped("No updates this version.");
        ImGui::Spacing();
    }
    ImGui::Spacing();

    // End spacing style
    ImGui::PopStyleVar();
}

void showDisabledButton(static char label[], ImVec2 size) {
    ImGui::BeginDisabled();
    ImGui::Button(label, size);
    ImGui::EndDisabled();
}

std::vector<std::string > getListOfConnStrings()
{
    std::string connStrDir = "C:\\ImplementationToolbox\\ConnectionStrings\\";
    std::vector<std::string> connStrFiles;
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(connStrDir))
            if (entry.path().extension() == ".str")
                connStrFiles.push_back(entry.path().filename().string());
    }
    catch (const std::exception&)
    {
		std::cerr << "[ERROR] Failed to read connection string directory. Check that it exists.\n";
    }
    
    return connStrFiles;
}

/// <summary>
/// Add basic button with custom label and default size
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <returns>true if clicked, false if not</returns>
bool Button(const char* label)
{
    if (ImGui::Button(label))
        return true;
    else
        return false;
}

/// <summary>
/// Add basic button with custom label and default size and ability to enable/disable with a state bool
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="disabled">- bool that determines if it appears disabled</param>
/// <returns>true if clicked, false if not</returns>
bool Button(const char* label, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
        ImGui::Button(label);
        ImGui::EndDisabled();
        return false;
    }
    else
    {
        return ImGui::Button(label);
    }
    return false;
}

/// <summary>
/// Add basic button with custom label and custom size
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="size">- ImVec2 size of the button to display</param>
/// <returns>true if clicked, false if not</returns>
bool Button(const char* label, const ImVec2 size)
{
    if (ImGui::Button(label, size))
        return true;
    else
        return false;
}

/// <summary>
/// Add basic button with custom label, custom size, and a flag to enable or disable the button
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="size">- ImVec2 size of the button to display</param>
/// <param name="disabled">- bool that determines if it appears disabled</param>
/// <returns>true if clicked, false if not</returns>
bool Button(const char* label, const ImVec2 size, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
        ImGui::Button(label, size);
        ImGui::EndDisabled(); 
        return false;
    }
    else
    {
        if (ImGui::Button(label, size))
        {
            return true;
        }        
    }
    return false;
}

/// <summary>
///  Adds a button labeled with char* passed that will flip the state of the bool you pass it. Displays the default size of button
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="trigger">- the bool to flip current state of</param>
/// <returns>boolean</returns>
bool ButtonTrigger(const char* label, bool* trigger)
{
    if (ImGui::Button(label))
    {
		*trigger = !(*trigger); // Toggle the trigger state
        return true;
    }
    else
        return false;
}

/// <summary>
/// Adds a button labeled with char* passed that will flip the state of the bool you pass it. Displays the passed size of button
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="trigger">- the bool to flip current state of</param>
/// <returns>boolean</returns>
bool ButtonTrigger(const char* label, bool* trigger, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
        ImGui::Button(label);
        ImGui::EndDisabled();
        return false;
    }
    else
    {
        if (trigger != nullptr) // Check if the pointer is not null  
        {
            if (ImGui::Button(label))
            {
                *trigger = !(*trigger);
                return true;
            }
        }
        else
        {
            return false;
        }
    }
}

/// <summary>
/// Adds a button labeled with char* passed that will flip the state of the bool you pass it. Displays the passed size of button
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="trigger">- the bool to flip current state of</param>
/// <param name="size">- ImVec2 size of the button to display</param>
/// <returns>boolean</returns>
bool ButtonTrigger(const char* label, bool* trigger, const ImVec2 size)
{
    if (ImGui::Button(label, size))
    {
        *trigger = !(*trigger); // Toggle the trigger state
        return true;
    }
    else
        return false;
}

/// <summary>
/// Adds a button labeled with char* passed that will flip the state of the bool you pass it. Displays the passed size of button
/// </summary>
/// <param name="label">- the text displayed in the button</param>
/// <param name="trigger">- the bool to flip current state of</param>
/// <param name="size">- ImVec2 size of the button to display</param>
/// <param name="disabled">- bool that determines if it appears disabled</param>
/// <returns>boolean</returns>
bool ButtonTrigger(const char* label, bool* trigger, const ImVec2 size, bool disabled)
{  
   if (disabled) 
   {  
       ImGui::BeginDisabled();
       ImGui::Button(label, size);
       ImGui::EndDisabled();
       return false;
   }  
   else  
   {  
       if (trigger != nullptr) // Check if the pointer is not null  
       {
           if (ImGui::Button(label, size))
           {
               *trigger = !(*trigger);
               return true;
           }
       }
       else
       {
           return false;
       }
   }  
}