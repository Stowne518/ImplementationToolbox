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
        ImGui::Bullet(); ImGui::TextWrapped("Upgraded entire program to DX12 from DX9. This is an effort to modernize somewhat and make portability better. Should reduce external needed dependancies.");
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("Planned Features")) {
        ImGui::Spacing();
        ImGui::Bullet(); ImGui::TextWrapped("New Functionality: Generic Data import - saved user preferences specific to the module. Things like displayed windows, button size, etc.");
        ImGui::Bullet(); ImGui::TextWrapped("New Feature: Get Implementation Files. The idea with this module is that it will check the cleaninstall folder for all interfaces and files and allow you to pick one and download it's contents locally for ease of uploading to a client or keeping files up to date.");
        ImGui::Bullet(); ImGui::TextWrapped("New Feature: Get Implementation Files - Keep up to date. When this is selected and you open the application, it will automatically check the interface folder you have configured to keep up to date and replace the older version with the newest files automatically.");
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
        ImGui::TextWrapped("No updates this version.");
        ImGui::Spacing();
    }
    ImGui::SeparatorText("SQL");
    if (ImGui::CollapsingHeader("Generic Data Import"))
    {
        ImGui::Spacing();
        ImGui::TextWrapped("Generic Data Import");
        ImGui::Bullet(); ImGui::TextWrapped("Added popout window to the data staging table for easier viewing of data on smaller screens.");
        ImGui::Bullet(); ImGui::TextWrapped("Refactored some logic and broke up large files for faster processing time and easier readability of functionality.");
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

void showDisabledButton(const char* label, ImVec2 size) {
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