#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "..\imgui.h"
#include "readFieldList.h"
#include "Profile.h"
#include "fieldOrderTable.h"

// System Includes
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#include <stdint.h>         // intptr_t
#include <iostream>         // File read/write for field list table
#include <vector>           // Store field options in vector
#include <string>
#include <cstring>
#include <algorithm>
#if !defined(_MSC_VER) || _MSC_VER >= 1800
#include <inttypes.h>       // PRId64/PRIu64, not avail in some MinGW headers.
#include <filesystem>
#include <iostream>
#include <cstdio>
#endif


// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to an 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                     // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                           // yes, they are more terse.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"        // warning: 'xx' is deprecated: The POSIX name for this..   // for strdup used in demo code (so user can copy & paste the code)
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type
#pragma clang diagnostic ignored "-Wformat-security"                // warning: format string is not a string literal
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor    // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wunused-macros"                  // warning: macro is not used                               // we define snprintf/vsnprintf on Windows so they are available, but not always used.
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                   // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat-security"          // warning: format string is not a string literal (potentially insecure)
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wmisleading-indentation"   // [__GNUC__ >= 6] warning: this 'if' clause does not guard this statement      // GCC 6.0+ only. See #883 on GitHub.
#endif

// Play it nice with Windows users (Update: May 2018, Notepad now supports Unix-style carriage returns!)
#ifdef _WIN32
#define IM_NEWLINE  "\r\n"
#else
#define IM_NEWLINE  "\n"
#endif

// Helpers
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf    _snprintf
#endif
#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

// Format specifiers for 64-bit values (hasn't been decently standardized before VS2013)
#if !defined(PRId64) && defined(_MSC_VER)
#define PRId64 "I64d"
#define PRIu64 "I64u"
#elif !defined(PRId64)
#define PRId64 "lld"
#define PRIu64 "llu"
#endif

// Helpers macros
// We normally try to not use many helpers in imgui_demo.cpp in order to make code easier to copy and paste,
// but making an exception here as those are largely simplifying code...
// In other imgui sources we can use nicer internal functions from imgui_internal.h (ImMin/ImMax) but not in the demo.
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

// Enforce cdecl calling convention for functions called by the standard library,
// in case compilation settings changed the default to e.g. __vectorcall
#ifndef IMGUI_CDECL
#ifdef _MSC_VER
#define IMGUI_CDECL __cdecl
#else
#define IMGUI_CDECL
#endif
#endif

// Function prototypes
std::vector<std::string> readFieldList();
void reset();
void saveGenericExport(int);
void saveGenericFields(int);

//Globals

// Constants - match SQL columnn limits of genexprt
const int MAX_EXPORT_NAME = 10;
const int MAX_SFTP_USER = 25;
const int MAX_SFTP_PASS = 20;
const int MAX_CHAR_SIZE = 64;
const int LONG_CHAR_SIZE = 128;
const int MAX_SELECTED_SIZE = 120;

// Program string Variables
static char
exportName[MAX_EXPORT_NAME],                    // Name of the export
sftpUser[MAX_SFTP_USER],                        // User name for the SFTP site
sftpPass[MAX_SFTP_PASS],                        // Password for the SFTP site
sftpIp[LONG_CHAR_SIZE],                         // IP Address for SFTP site
sftpTargetDir[MAX_CHAR_SIZE],                   // Target directory to place files in at the SFTP site
agency[5] = "",                                 // Agency code for agency exporting files
filePrefix[MAX_CHAR_SIZE],                      // Prefix value to add to the generated file
label[MAX_SELECTED_SIZE][100],                  // Labels for available fields to display in the grid
rmsDb[15],                                      // RMS Database name
delimiter[1],                                   // Delimeter character to use between filed values
collectiontag[MAX_CHAR_SIZE],                   // Tag used to edit the group tag in an XML file ex: <Inmates> is the default
itemtag[MAX_CHAR_SIZE],                         // Tag used to edit the individual tags in the XML file ex: <Inmate> is the default
altNames[MAX_SELECTED_SIZE][50],                // Alternate names to give to each field
profileName[MAX_CHAR_SIZE];                     // Name of the file to save the profile to


static bool                                   // Check box boolean options
isPinReq = false,                             // Determines if a PIN field will be included
includeHousing = false,                       // Determines if the housing fields will be included in the list of available fields
exportNow = false,                            // Skips the export timer option and will immediately export when a change is detected to a monitored field
combineHousing = false,                       // Combines housing fields into one field
combineStApt = false,                         // Combines the street and apartment fields
retainHistory = false,                        // Retains historical files instead of deleting after a successful transfer
secureSSN = false,                            // Only displays the last 4 numbers in a SSN on the export file
/* Only enable if fileOptn = 2, 3, 5*/
noQuotes = false,                             // Removes all double quotes from field values in a flat file
selected[MAX_SELECTED_SIZE] = { 0 };          // Array used to determine which fields have been selected

// Radio Button/Combo drop down int values
static int
pinType,                        // Value used to determine what data the PIN field will contain
fileType,                       // Value used to determine what file type will be exported
recordOptn,                     // Value used to determine what record option is used
fileOptn,                       // Value used to determine if all inmates or only active/changed records are exported
createOptn,                     // Value used to determine when the file is created
dateFormat,                     // Used to set the date format on file names, only for flat files
isSftp = 2,                     // Used to determine if the export will be using FTP or SFTP
writeDelay = 0,                 // Used to set a delay on when the file is written
order[130] = { 0 },             // Array that stores the order the fields will be exported with
genexptid;                      // Value that stores what the genexprtid is in SQL

// Concrete values that are always the same in every export
const static int
actcodetyp = 2,
cashBalFmt = 0,
inactive = 0;

static char
activeField = 'A',              // Concrete value for active fields
inActiveField = 'I';            // Concrete value for inactive fields

// Create static profile object
static Profile profile;

// Global vectors used for field selection
std::vector<std::string> fields = readFieldList();
static std::vector<std::string> selectedFieldName;
static std::vector<int> selectedId;
static bool profileLoaded = false;     // Check if incoming data was from a profile that was loaded from a file

/*
    Function used to get a list of profile files in the temp directory and display them in a Combo box that we then pass to import and bring the data in.
*/
std::string displayFileName(const std::string& directory) {
    static std::string chosenName;
    std::vector<std::string> fileNames;
    for (const auto& entry : std::filesystem::directory_iterator(directory + "Profiles\\"))
    {
        std::string filename = entry.path().filename().string();
        size_t pos = filename.find("_profiles.txt");
        if (pos != std::string::npos) {
            std::string displayName = filename.substr(0, pos);
            fileNames.push_back(displayName);
        }
    }

    // Convert std::vector<std::string> to array of const char* for ImGui display
    std::vector<const char*> fileNameCStr;
    for (const auto& name : fileNames) {
        fileNameCStr.push_back(name.c_str());
    }

    // Display the Combo box if we find a profile has been created otherwise we show text instead
    static int currentItem = 0;
    if (!fileNameCStr.empty() && ImGui::BeginCombo("##Select a Profile", fileNameCStr[currentItem])) {
        for (int n = 0; n < fileNameCStr.size(); n++) {
            bool isSelected = (currentItem == n);
            if (ImGui::Selectable(fileNameCStr[n], isSelected)) {
                currentItem = n;
                chosenName = fileNameCStr[n];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        // End Combo Box for profile drop down
        ImGui::EndCombo();
    }
    else if(fileNameCStr.empty())
        ImGui::TextWrapped("No profiles found. Use Export profile first to generate a profile text file.");

    // Return the currently selected name in the combo box
    return chosenName;
}

// Option to delete existing profile
bool deleteProfile(const std::string& directory, std::string& profileName) {
    std::string filename = directory + profileName;

    return std::filesystem::remove(filename);
}

// Used to reset any selected fields back to false and clear the field list
void clearSelectedFields() {
    for (int i = 0; i < IM_ARRAYSIZE(selected); i++)
    {
        if (selected[i]) {
            // Set all selected fields back to false
            selected[i] = false;
            profile.setSelected(i, false);

            // Set any previosuly assigned orders back to 0
            order[i] = 0;
            profile.setOrder(i, 0);
        }
    }
    // Clear out list of ids
    selectedId.clear();
    selectedFieldName.clear();
}

/*
Code to display generic export generator and options
Use this to get around application and perform all functions
*/
void showGenericExportWindow(bool* p_open) {
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = true;
    static bool no_collapse = true;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool unsaved_document = false;
    static bool no_saved_settings = true;
    static bool no_decoration = false;
    static bool no_input = true;
    static bool no_label = true;

    static bool show_export_profile_window = false;
    static bool show_import_profile_window = false;

    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;
    if (no_saved_settings)  window_flags |= ImGuiWindowFlags_NoSavedSettings;
    if (no_decoration)      window_flags |= ImGuiWindowFlags_NoDecoration;
    if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

    static bool use_work_area = false;

    // Set fullscreen options
    // DONE: Fix the fullscreen option. Only works one way and cannot go back to a windowed view after setting fullscreen
    //if (use_work_area)
    //{        
    //    ImGui::SetNextWindowPos(use_work_area ? main_viewport->WorkPos : main_viewport->Pos);
    //    ImGui::SetNextWindowSize(use_work_area ? main_viewport->WorkSize : main_viewport->Size);
    //}

    //if(!use_work_area)
    //{
    //    // Center window when it opens
    //    ImVec2 center_window = ImGui::GetMainViewport()->GetCenter();
    //    ImGui::SetNextWindowPos(center_window, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    //    ImGui::SetNextWindowSize(ImVec2(display.x * .75, (display.y * .75)), ImGuiCond_Always);
    //}        

        // Open Import profile Window
        if (show_import_profile_window)
            ImGui::OpenPopup("Load Profile Settings");

        // Center window when it opens
        ImVec2 Import_Export_center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(Import_Export_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // Import profile popup window
        if (ImGui::BeginPopupModal("Load Profile Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Load a saved profile");
            // ImGui::Text("Files are saved to C:\\ImplementationToolkit\\ for now due to permission issues with OneDrive");
            std::string selected_profile = displayFileName("C:\\ImplementationToolbox\\Profiles\\") + "_profiles.txt";
            // Currently we only save one profile, may adjust to save profiles to multiple files
            if (ImGui::Button("Load profile")) {
                // Clear any selected fields before loading new profile
                clearSelectedFields();
                // Load values in from file
                if (profile.importProfiles(&profile, selected_profile)) {
                    // Set char array values returned by the import
                    std::strncpy(exportName, profile.getExportName(), MAX_CHAR_SIZE);
                    std::strncpy(sftpUser, profile.getSftpUser(), MAX_CHAR_SIZE);
                    std::strncpy(sftpPass, profile.getSftpPass(), MAX_CHAR_SIZE);
                    std::strncpy(sftpIp, profile.getSftpUser(), LONG_CHAR_SIZE);
                    std::strncpy(sftpTargetDir, profile.getSftpTargetDir(), MAX_CHAR_SIZE);
                    std::strncpy(agency, profile.getAgency(), 5);
                    std::strncpy(filePrefix, profile.getFilePrefix(), MAX_CHAR_SIZE);
                    // Copy selected values into the selected array
                    for (int i = 0; i < MAX_SELECTED_SIZE; i++) {
                        selected[i] = profile.getSelected(i);
                    }
                    std::strncpy(rmsDb, profile.getRmsDb(), 15);
                    std::strncpy(delimiter, profile.getDelimiter(), 1);
                    std::strncpy(profileName, profile.getProfileName(), MAX_CHAR_SIZE);
                    isPinReq = profile.getIsPinReq();
                    includeHousing = profile.getIncludeHousing();
                    exportNow = profile.getExportNow();
                    combineHousing = profile.getCombineHousing();
                    combineStApt = profile.getCombineStApt();
                    retainHistory = profile.getRetainHistory();
                    secureSSN = profile.getSecureSSN();
                    noQuotes = profile.getNoQuotes();
                    pinType = profile.getPinType();
                    fileType = profile.getFileType();
                    recordOptn = profile.getRecordOptn();
                    fileOptn = profile.getFileOptn();
                    createOptn = profile.getCreateOptn();
                    dateFormat = profile.getDateFormat();
                    isSftp = profile.getIsSftp();
                    writeDelay = profile.getWriteDelay();
                    genexptid = profile.getGenexptid();

                    // Set profile loaded = true since we loaded this data from a file, it is not static so it gets reset after the data is all in from the file and we can selected new fields
                    profileLoaded = true;

                    // Select fields coming from the last profile load if there was one
                    for (int i = 0; i < fields.size(); i++)
                    {
                        int orderNum = profile.getOrder(i);
                        if (profile.getOrder(i + 1) != 0) {
                            selectedId.push_back(orderNum);
                            selectedFieldName.push_back(fields[orderNum]);
                        }
                        // Set the last item in the list and then break loop since we're done with ordering.
                        else {
                            selectedId.push_back(orderNum);
                            selectedFieldName.push_back(fields[orderNum]);
                            break;
                        }
                    }                    

                    // Close pop up if loaded successfully
                    show_import_profile_window = false;
                    ImGui::CloseCurrentPopup();
                }
                
            }
            

            ImGui::SameLine();
            if (ImGui::Button("Delete Profile")) {
                ImGui::SetNextWindowPos(Import_Export_center);
                ImGui::OpenPopup("Confirm Delete");
            }
            ImGui::SameLine();
            if (ImGui::Button("Close Window")) {
                show_import_profile_window = false;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::BeginPopupModal("Confirm Delete")) {
                ImGui::Text("Are you sure you want to delete %s", selected_profile.c_str());
                if (ImGui::Button("Yes")) {
                    deleteProfile("C:\\ImplementationToolbox\\Profiles\\", selected_profile);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No"))
                    ImGui::CloseCurrentPopup();

                // End delete confirmation popup
                ImGui::EndPopup();
            }

            // End Import popup window
            ImGui::EndPopup();
        }


        // Open export profile window
        if(show_export_profile_window)
            ImGui::OpenPopup("Save Profile Settings");

        // Export profile popup window
        if (ImGui::BeginPopupModal("Save Profile Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Save your current profile settings:");
            static int editProfileName = 0;
            // Set profile name equal to export name once so the user can edit it afterward if they want
            if (editProfileName == 0) {
                std::strncpy(profileName, exportName, MAX_CHAR_SIZE);
                editProfileName = 1;
            }
            ImGui::Text("Export Name:"); ImGui::SameLine();
            ImGui::InputText("##ExportName", profileName, MAX_CHAR_SIZE);
            profile.setProfileName(profileName);

            if (ImGui::Button("Save Profile")) {
                for (int i = 0; i < selectedId.size(); i++) {
                    //  update profile order with the latest order from field selection
                    profile.setOrder(i, selectedId[i]);
                }
                profile.exportProfiles(&profile);

                show_export_profile_window = false;
                // End save window after successful save
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close Window")) {
                show_export_profile_window = false;
                ImGui::CloseCurrentPopup();
            }

            // End popup window for exporting profiles
            ImGui::EndPopup();
        }        

        ImGui::SeparatorText("Profile Management");
        // Intro text when the tool opens up
        if (ImGui::Button("Save Profile")) {
            show_export_profile_window = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Profile")) {
            show_import_profile_window = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Profile")) {
            reset();
        }

        // Start popup for confirmation that profile values reset successfully
        if (ImGui::BeginPopup("Profile Reset")) {
            ImGui::Text("Profile values reset!");

            // End reset popup
            ImGui::EndPopup();
        }

        ImGui::SeparatorText("Generic Export Generator");
        if (ImGui::CollapsingHeader("Generic Export Generator Tool Information")) {
            ImGui::TextWrapped("This tool is designed to help generate and edit generic exports in ONESolution JMS easily, quickly, and correctly every time using SQL scripts.");
            ImGui::TextWrapped("Before any script from this tool can be run you MUST log in to JMS and add then saving a blank generic export."
                "No infomration is required to be added to it, but the generic export ID MUST be generated, and the fields must also be populated before attempting to run these queries.");
            ImGui::TextWrapped("To start, expand the generic export configuration header to begin configuring the generic export.");
            ImGui::Text("* = required field");
        }
        if (ImGui::CollapsingHeader("Generic Export Configuration")) {
            // 1. Generic export naming and vendor connectivity
            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Setup")) {
                    ImGui::Text("*Export Name"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(300);
                    ImGui::InputTextWithHint("##Export Name", "Only text, no symbols or special chars", exportName, IM_ARRAYSIZE(exportName), ImGuiInputTextFlags_CharsUppercase);
                    profile.setExportName(exportName);
                    ImGui::Text("*Agency Code"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(50);
                    ImGui::InputText("##Agency Code", agency, IM_ARRAYSIZE(agency), ImGuiInputTextFlags_CharsUppercase | ImGuiColorEditFlags_NoLabel);
                    profile.setAgency(agency);
                    ImGui::SeparatorText("FTP/SFTP Options");
                    ImGui::RadioButton("None", &isSftp, 2); ImGui::SameLine();
                    ImGui::RadioButton("FTP", &isSftp, 0); ImGui::SameLine();
                    ImGui::RadioButton("SFTP", &isSftp, 1);
                    profile.setIsSftp(isSftp);
                    if (isSftp == 0 || isSftp == 1) {
                        ImGui::Text("*Username"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputText("##Username", sftpUser, IM_ARRAYSIZE(sftpUser)); ImGui::SameLine();
                        ImGui::Text("*Password"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputText("##Password", sftpPass, IM_ARRAYSIZE(sftpPass));

                        // SFTP/FTP site info
                        ImGui::Text("*Host address"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(300);
                        ImGui::InputText("##Host Address", sftpIp, IM_ARRAYSIZE(sftpIp));
                        ImGui::Text("Target Directory"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputText("##Target Directory", sftpTargetDir, IM_ARRAYSIZE(sftpTargetDir));
                        // Update profile with sftp data
                        profile.setSftpUser(sftpUser);
                        profile.setSftpPass(sftpPass);
                        profile.setSftpIp(sftpIp);
                        profile.setSftpTargetDir(sftpTargetDir);
                    }
                    ImGui::Text("Enter client's RMS DB name*:"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputText("##rmsdb", rmsDb, IM_ARRAYSIZE(rmsDb));
                    profile.setRmsDb(rmsDb);

                    ImGui::Text("Enter Generic Export ID to update*:"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(75);
                    ImGui::InputInt("##genexprtid", &genexptid, 1, 1);
                    profile.setGenexptid(genexptid);

                    // End Naming & Connectivity TreeNode
                    ImGui::EndTabItem();
                }

                // 2. File Options
                if (ImGui::BeginTabItem("File Options")) {
                    ImGui::BeginGroup();
                    {
                        ImGui::SeparatorText("Generated File Type");
                        ImGui::RadioButton("XML*", &fileType, 1); ImGui::SameLine();
                        if (fileType == 1) {
                            ImGui::Text("Collection Tag"); ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputText("##collectiontag", collectiontag, IM_ARRAYSIZE(collectiontag)); 
                            ImGui::SetItemTooltip("Collection tag changes what the first tag is labeled as in the XML that contains the rest of the child elements. Default = Inmates"); ImGui::SameLine();
                            ImGui::Text("Item Tag"); ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputText("##itemtag", itemtag, IM_ARRAYSIZE(itemtag));
                            ImGui::SetItemTooltip("Item tag sets the label of each inmate tag. Default = Inmate");
                        }
                        else {
                            ImGui::BeginDisabled();
                            ImGui::Text("Collection Tag"); ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputText("##collectiontag", collectiontag, IM_ARRAYSIZE(collectiontag));
                            ImGui::SetItemTooltip("Collection tag changes what the first tag is labeled as in the XML that contains the rest of the child elements. Default = Inmates"); ImGui::SameLine();
                            ImGui::Text("Item Tag"); ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputText("##itemtag", itemtag, IM_ARRAYSIZE(itemtag));
                            ImGui::SetItemTooltip("Item tag sets the label of each inmate tag. Default = Inmate");
                            ImGui::EndDisabled();
                        }
                        // Delimted file options group
                        ImGui::BeginGroup();
                        ImGui::Text("Delimited");
                        // Begin group for comma, tab, and other buttons
                        ImGui::RadioButton("Comma*", &fileType, 2);
                        ImGui::RadioButton("Tab*", &fileType, 3);
                        // Begin other file type group
                        ImGui::BeginGroup();
                        ImGui::RadioButton("Other*", &fileType, 5);
                        profile.setFileType(fileType);
                        ImGui::SetItemTooltip("Other allows you to specify a custom delimeter character. Always exports as a .txt file");
                        ImGui::SameLine();
                        if (fileType == 5) {
                            ImGui::SetNextItemWidth(25);
                            ImGui::InputText("*Delimiter", delimiter, IM_ARRAYSIZE(delimiter) + 1, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
                            profile.setDelimiter(delimiter);
                        }
                        else {
                            ImGui::SetNextItemWidth(25);
                            ImGui::BeginDisabled();
                            ImGui::InputText("*Delimeter", delimiter, IM_ARRAYSIZE(delimiter) + 1, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
                            ImGui::EndDisabled();
                            delimiter[1] = {};
                        }
                        // End other file option group
                        ImGui::EndGroup();
                        ImGui::SameLine();
                        // Begin group for disable quotes to put on same line as buttons
                        ImGui::BeginGroup();
                        if (fileType == 2 || fileType == 3 || fileType == 5) {
                            ImGui::Checkbox("Disable Quotes?", &noQuotes);
                            ImGui::SetItemTooltip("Available for Comma, Tab, and Other");
                            profile.setNoQuotes(noQuotes);
                        }
                        else {
                            ImGui::BeginDisabled();
                            ImGui::Checkbox("Disable Quotes?", &noQuotes);
                            ImGui::SetItemTooltip("Available for Comma, Tab, and Other");
                            ImGui::EndDisabled();
                        }
                        // End disable quotes group
                        ImGui::EndGroup();

                        ImGui::RadioButton("Fixed*", &fileType, 4);                        

                        // End delimited file options group
                        ImGui::EndGroup();

                        // End main file options group
                        ImGui::EndGroup();
                    }
                    // Put File options & file creation on one X-axis
                    ImGui::SameLine();

                    // Begin file creation options group
                    ImGui::BeginGroup();
                    {
                        // File creation options
                        ImGui::Text("\n\nFile Creation Options");
                        ImGui::RadioButton("Single file for All Inmates*", &createOptn, 1);
                        ImGui::RadioButton("Single file for each inmate*", &createOptn, 2);
                        if (createOptn == 2) {
                            ImGui::SameLine();
                            ImGui::Text("File Prefix"); ImGui::SameLine(); ImGui::SetNextItemWidth(75); ImGui::InputText("##Prefix", filePrefix, 2);
                            ImGui::SetItemTooltip("Optional prefix available for individual files*");
                            profile.setFilePrefix(filePrefix);
                        }
                        ImGui::RadioButton("Separate file for booked and released*", &createOptn, 3);
                        profile.setCreateOptn(createOptn);
                        ImGui::Text("File Write Delay"); ImGui::SameLine(); ImGui::SetNextItemWidth(75); ImGui::InputInt("##WriteDelay", &writeDelay, 1);
                        profile.setWriteDelay(writeDelay);

                        // End File creation group
                        ImGui::EndGroup();
                    }
                    ImGui::BeginGroup();
                    {
                        // Record option
                        ImGui::SeparatorText("Records Options");
                        ImGui::RadioButton("All Records*", &recordOptn, 1); ImGui::SameLine();
                        ImGui::BeginGroup();
                        {
                            ImGui::RadioButton("Changed Records Only*", &recordOptn, 2);
                            if (recordOptn == 2) {
                                ImGui::Text("Export immediately"); ImGui::SameLine(); ImGui::Checkbox("##ExportImmediately", &exportNow);
                                ImGui::SetItemTooltip("This will export records as soon as a change is detected\ninstead of writing on a timed interval only.");
                                // Update export now on profile
                                profile.setExportNow(exportNow);
                            }

                            // End export immediately group
                            ImGui::EndGroup();
                        }
                        ImGui::SameLine();
                        ImGui::RadioButton("All active records", &recordOptn, 3);

                        // Update profile with record option
                        profile.setRecordOptn(recordOptn);

                        // File naming convention
                        ImGui::SeparatorText("File naming convention");
                        ImGui::RadioButton("Continuous overwrite*", &fileOptn, 1); ImGui::SameLine();
                        ImGui::SetItemTooltip("Writes to one file with the same name");
                        ImGui::RadioButton("Unique file name*", &fileOptn, 2);
                        profile.setFileOptn(fileOptn);
                        ImGui::SetItemTooltip("Writes a unique file each time with time stamp");
                        if (fileOptn == 2) {
                            ImGui::Text("*Choose date/time format");
                            ImGui::RadioButton("01/01/2001*", &dateFormat, 1); ImGui::SameLine();
                            ImGui::RadioButton("20010101*", &dateFormat, 2); ImGui::SameLine();
                            ImGui::RadioButton("2001-01-01*", &dateFormat, 3);
                            profile.setDateFormat(dateFormat);
                        }
                        else {
                            ImGui::BeginDisabled();
                            ImGui::Text("*Choose date/time format");
                            ImGui::RadioButton("01/01/2001*", &dateFormat, 1); ImGui::SameLine();
                            ImGui::RadioButton("20010101*", &dateFormat, 2); ImGui::SameLine();
                            ImGui::RadioButton("2001-01-01*", &dateFormat, 3);
                            ImGui::EndDisabled();
                        }

                        // End Record option group
                        ImGui::EndGroup();
                    }

                    // End TreeNode
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Field Selection")) {
                    ImGui::BeginGroup();
                    {
                        ImGui::SeparatorText("Field Options");
                        ImGui::Checkbox("Extra PIN", &isPinReq);
                        profile.setIsPinReq(isPinReq);
                        if (isPinReq) {
                            ImGui::SameLine();
                            ImGui::RadioButton("SSN*", &pinType, 1); ImGui::SameLine();
                            ImGui::RadioButton("Name ID*", &pinType, 2); ImGui::SameLine();
                            ImGui::RadioButton("Booking ID*", &pinType, 3);
                            profile.setPinType(pinType);
                        }
                        // Clear pinType if extra pin isn't required
                        else {
                            ImGui::SameLine();
                            ImGui::BeginDisabled();
                            ImGui::RadioButton("SSN*", &pinType, 1); ImGui::SameLine();
                            ImGui::RadioButton("Name ID*", &pinType, 2); ImGui::SameLine();
                            ImGui::RadioButton("Booking ID*", &pinType, 3);
                            ImGui::EndDisabled();
                            pinType = 0;
                        }
                        ImGui::Checkbox("Combine Street and Apt #", &combineStApt);
                        profile.setCombineStApt(combineStApt);
                        ImGui::BeginGroup();
                        {
                            ImGui::Checkbox("Include housing options?", &includeHousing);
                            profile.setIncludeHousing(includeHousing);
                            if (includeHousing) {
                                ImGui::SameLine();
                                ImGui::Checkbox("Combine housing fields?", &combineHousing);
                                profile.setCombineHousing(combineHousing);
                            }
                            // Clear combined housing option if include housing isn't true
                            else {
                                ImGui::BeginDisabled();
                                ImGui::SameLine();
                                ImGui::Checkbox("Combine housing fields?", &combineHousing);
                                profile.setCombineHousing(combineHousing);
                                ImGui::EndDisabled();
                                combineHousing = false;
                            }
                            ImGui::EndGroup();
                        }

                        // End field options group
                        ImGui::EndGroup();
                    }

                    ImGui::Spacing();

                    ImGui::SeparatorText("Field Selection");
                    
                    /* Had to use a different method to get unique label names in the list
                    *  We are receiving a list of vector strings from the field list function
                    *  Then we created our 2D array of chars
                    *  This is what we use to case the strings onto, since Selectable() needs a char array for the strings
                    *  Then use strcpy to copy the string into the char array so we can pass it to Selectable() and it will be happy
                    */
                    for (int i = 0; i < fields.size(); i++)
                    {
                        strcpy(label[i], fields[i].c_str());
                        profile.setLabel(i, fields[i].c_str());
                    }

                    // When button is clicked we loop over all array options and reset the selected value to false, clearing the board
                    if (ImGui::Button("Clear Selections")) {
                        clearSelectedFields();
                    }

                    // Sizing for table to fit in window
                    ImVec2 windowSize = ImVec2(ImGui::GetMainViewport()->Size.x, 0);

                    // Begin table for selectable field items
                    if (ImGui::BeginTable("Select Fields", 6, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedSame, windowSize)) {
                        for (int i = 0; i < fields.size(); i++)
                        {
                            ImGui::TableNextColumn();
                            if (!includeHousing && fields[i] == "LocFac" || !includeHousing && fields[i] == "LocSec" || !includeHousing && fields[i] == "LocUnit" || !includeHousing && fields[i] == "LocBed") {
                                ImGui::BeginDisabled();
                                ImGui::Text(label[i]);
                                ImGui::SetItemTooltip("Enable include housing options to select field");
                                ImGui::EndDisabled();
                            }
                            else if (!isPinReq && fields[i] == "Pin") {
                                ImGui::BeginDisabled();
                                ImGui::Text("Pin");
                                ImGui::SetItemTooltip("Enable Extra PIN option to select field");
                                ImGui::EndDisabled();
                            }
                            else {
                                ImGui::Selectable(label[i], &selected[i]);
                            }
                            // If data was loaded from a profile we don't want to insert it into selectedIds or selectedFieldName twice.
                            if (selected[i] && !profileLoaded) {
                                if (std::find(selectedId.begin(), selectedId.end(), i) == selectedId.end()) {
                                    selectedId.push_back(i);
                                    selectedFieldName.push_back(label[i]);
                                    profile.setSelected(i, selected[i]);
                                }
                            }
                            else if(!profileLoaded) {
                                // Find the index of i in selectedId
                                auto it = std::find(selectedId.begin(), selectedId.end(), i);
                                if (it != selectedId.end()) {
                                    // Calculate the index position
                                    int index = std::distance(selectedId.begin(), it);
                                    // Remove the element from selectedId
                                    selectedId.erase(it);
                                    // Remove the corresponding element from selectedFieldName
                                    selectedFieldName.erase(selectedFieldName.begin() + index);
                                }
                                selected[i] = false;
                                profile.setSelected(i, false);
                                profile.setOrder(i, 0);
                            }
                            // After adding fields from a profile we break this loop and set it back to normal functionality by resetting profileLoaded
                            else {
                                profileLoaded = false;
                                break;
                            }
                        }

                        // End Field table
                        ImGui::EndTable();
                    }
                    // TODO: fix ordering fields
                    ImGui::BeginGroup();
                    // Leaving as is for now
                    ImGui::SeparatorText("Set field order:");
                    ImGui::SetItemTooltip("Sort functionality not available yet. The order will update as field order is changed in the future");
                    if (ImGui::BeginTable("fieldOrder", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX /* | ImGuiTableFlags_Sortable */ )) {
                        ImGui::TableSetupColumn("Field");
                        ImGui::TableSetupColumn("Order");
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableHeadersRow();
                        for (int i = 0; i < selectedId.size(); i++)
                        {
                            ImGui::TableNextColumn();
                            ImGui::Text(selectedFieldName[i].c_str());
                            ImGui::TableNextColumn();
                            // If there isn't already an order value we set it equal to the next available value
                            if (order[i] == 0)
                                order[i] = i;
                            // Disable the custom order input for now until sorting works. Currently, it will order based on the order the fields are clicked in.
                            ImGui::BeginDisabled();
                            ImGui::InputInt(("##order" + std::to_string(i)).c_str(), &order[i], 0, 0);
                            ImGui::EndDisabled();
                        }

                        // End field order table
                        ImGui::EndTable();
                    }

                    // Attempting to figure out table sorting
                    // It's one of the most complex aspects of this framework so I'll need to keep tinkering periodically until it clicks
                    //ImGui::SameLine();

                    //std::vector<TableItem> items;
                    //// Populate the sorted table order with order from selected fields
                    //for (int i = 0; i < selectedId.size(); i++)
                    //{
                    //        items.push_back({ selectedFieldName[i], i });
                    //}

                    //// Display sorted re-ordering table
                    //showTable(items);

                    ImGui::EndGroup();

                    // End Field selection tab node
                    ImGui::EndTabItem();
                }
            }
                
                // End tab bar
                ImGui::EndTabBar();
        }
        // Display of currently entered options in an overview format.
        ImGui::BeginGroup();
        {
            if(ImGui::TreeNode("Overview of Export"))
            {
                ImGui::SeparatorText("Setup");
                ImGui::Text("Export Name:%s | Agency Code = %s", exportName, agency);
                ImGui::Separator();
                ImGui::Text("Transfer Protocol:"); ImGui::SameLine();
                switch (isSftp) {
                case 0:
                    ImGui::Text("FTP");
                    break;
                case 1:
                    ImGui::Text("SFTP");
                    break;
                }
                ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Username:"); ImGui::SameLine(); ImGui::Text(sftpUser); ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Password:"); ImGui::SameLine(); ImGui::Text(sftpPass); ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Address:"); ImGui::SameLine(); ImGui::Text(sftpIp); ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine(); ImGui::Text("Target Dir:"); ImGui::SameLine(); ImGui::Text(sftpTargetDir);
                ImGui::Separator();
                ImGui::Text("File Type:"); ImGui::SameLine();
                switch (fileType)
                {
                case 1:
                    ImGui::Text("XML"); ImGui::SameLine(); ImGui::Text("| Collector tag:"); ImGui::SameLine();
                    if (collectiontag[0] == '\0')
                        ImGui::Text("Inmates");
                    else
                        ImGui::Text(collectiontag);
                    ImGui::SameLine();
                    ImGui::Text("| Item tag:");
                    ImGui::SameLine();
                    if (itemtag[0] == '\0')
                        ImGui::Text("Inmate");
                    else
                        ImGui::Text(itemtag);
                    break;
                case 2:
                    ImGui::Text("Comma");
                    break;
                case 3:
                    ImGui::Text("Tab");
                    break;
                case 4:
                    ImGui::Text("Fixed");
                    break;
                case 5:
                    ImGui::Text("Other"); ImGui::SameLine(); ImGui::Text("| Delimiter:"); ImGui::SameLine(); ImGui::Text(delimiter);
                    break;
                default:
                    ImGui::Text("");
                    break;
                }
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
                ImGui::Text("Quotes Disabled?"); ImGui::SameLine();
                if (fileType == 2 || fileType == 3 || fileType == 5) {
                    switch (noQuotes)
                    {
                    case true:
                        ImGui::Text("Yes");
                        break;
                    case false:
                        ImGui::Text("No");
                        break;
                    }
                }
                else
                    ImGui::Text("N/A");
                ImGui::Text("How created:"); ImGui::SameLine();
                switch (createOptn)
                {
                case 1:
                    ImGui::Text("Single file for all inmates");
                    break;
                case 2:
                    ImGui::Text("Changed records only"); ImGui::SameLine();
                    ImGui::Text("File Prefix:%s", filePrefix);
                    break;
                case 3:
                    ImGui::Text("All active records");
                    break;
                default:
                    break;
                }
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
                ImGui::Text("Write delay: %i", writeDelay);
                ImGui::Separator();
                ImGui::Text("Extra PIN:"); ImGui::SameLine();
                switch (isPinReq)
                {
                case true:
                    ImGui::Text("Yes:");
                    ImGui::SameLine();
                    switch (pinType)
                    {
                    case 1:
                        ImGui::Text("SSN");
                        break;
                    case 2:
                        ImGui::Text("Name ID");
                        break;
                    case 3:
                        ImGui::Text("Booking ID");
                        break;
                    default:
                        ImGui::Text("None Selected");
                        break;
                    }
                    break;
                case false:
                    ImGui::Text("No");
                }
                ImGui::Text("Combine Street and Apt number:"); ImGui::SameLine();
                switch (combineStApt)
                {
                case true:
                    ImGui::Text("Yes");
                    break;
                default:
                    ImGui::Text("No");
                    break;
                }
                ImGui::Text("Include housing:"); ImGui::SameLine();
                switch (includeHousing)
                {
                case true:
                    ImGui::Text("Yes");
                    break;
                default:
                    ImGui::Text("No");
                    break;
                }
                if (profile.getIncludeHousing()) {
                    ImGui::Text("Combine hosuing fields:"); ImGui::SameLine();
                    switch (profile.getCombineHousing())
                    {
                    case true:
                        ImGui::Text("Yes");
                        break;
                    default:
                        ImGui::Text("No");
                        break;
                    }
                }
                //End overview tree node
                ImGui::TreePop();
            }
            // End Overview Group
            ImGui::EndGroup();
        }

            if (ImGui::Button("Generate Export"))
                ImGui::OpenPopup("SQL Script Output");
            ImGui::SameLine();
            if (ImGui::Button("Generate Field List"))
                ImGui::OpenPopup("SQL Fields");

            // Center window when it opens
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionMax().x * .8, ImGui::GetContentRegionMax().y * .8));
            // Begin the field list and order SQL Script output
            if (ImGui::BeginPopupModal("SQL Script Output", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                // Begin child window to display sql script
                ImGui::BeginChild("SQL Output", ImVec2(ImGui::GetContentRegionMax().x, ImGui::GetContentRegionMax().y - 55), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                // Begin the Generic Export option SQL Script output
                ImGui::Text("--Updating the genexprt database with all provided information");
                ImGui::Text("Use %s", rmsDb);
                ImGui::Text("BEGIN TRANSACTION");
                ImGui::Text("UPDATE genexprt SET filename = '%s' WHERE genexprtid = %i", exportName, genexptid);
                ImGui::Text("UPDATE genexprt SET username = '%s' WHERE genexprtid = %i", sftpUser, genexptid);
                ImGui::Text("UPDATE genexprt SET password = '%s' WHERE genexprtid = %i", sftpPass, genexptid);
                ImGui::Text("UPDATE genexprt SET ipaddress = '%s' WHERE genexprtid = %i", sftpIp, genexptid);
                ImGui::Text("UPDATE genexprt SET targetdir = '%s' WHERE genexprtid = %i", sftpTargetDir, genexptid);
                ImGui::Text("UPDATE genexprt SET agency = '%s' WHERE genexprtid = %i", agency, genexptid);
                if (isSftp != 2)
                {
                    ImGui::Text("UPDATE genexprt SET issftp = '%i' WHERE genexprtid = %i", isSftp, genexptid);
                }
                switch (isPinReq) {
                case true:
                    ImGui::Text("UPDATE genexprt SET ispinreq = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET ispinreq = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                ImGui::Text("UPDATE genexprt SET pintype = '%i' WHERE genexprtid = %i", pinType, genexptid);
                switch (includeHousing) {
                case true:
                    ImGui::Text("UPDATE genexprt SET inclhousng = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET inclhousng = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                switch (combineHousing) {
                case true:
                    ImGui::Text("UPDATE genexprt SET combhousng = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET combhousng = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                switch (combineStApt) {
                case true:
                    ImGui::Text("UPDATE genexprt SET combstrapt = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET combstrapt = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                ImGui::Text("UPDATE genexprt SET actcodetyp = '2' WHERE genexprtid = %i", genexptid);
                ImGui::Text("UPDATE genexprt SET recordoptn = '%i' WHERE genexprtid = %i", recordOptn, genexptid);
                ImGui::Text("UPDATE genexprt SET filetype = '%i' WHERE genexprtid = %i", fileType, genexptid);
                ImGui::Text("UPDATE genexprt SET fileoptn = '%i' WHERE genexprtid = %i", fileOptn, genexptid);
                switch (exportNow) {
                case true:
                    ImGui::Text("UPDATE genexprt SET exprtnow = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET exprtnow = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                ImGui::Text("UPDATE genexprt SET createoptn = '%i' WHERE genexprtid = %i", createOptn, genexptid);
                ImGui::Text("UPDATE genexprt SET cashbalfmt = '0' WHERE genexprtid = %i", genexptid);
                ImGui::Text("UPDATE genexprt SET dateformat = '%i' WHERE genexprtid = %i", dateFormat, genexptid);
                ImGui::Text("UPDATE genexprt SET writedelay = '%i' WHERE genexprtid = %i", writeDelay, genexptid);
                switch (retainHistory) {
                case true:
                    ImGui::Text("UPDATE genexprt SET retainhist = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET retainhist = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                switch (noQuotes) {
                case true:
                    ImGui::Text("UPDATE genexprt SET noquotes = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET noquotes = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                switch (secureSSN) {
                case true:
                    ImGui::Text("UPDATE genexprt SET securessn = '1' WHERE genexprtid = %i", genexptid);
                    break;
                case false:
                    ImGui::Text("UPDATE genexprt SET securessn = '0' WHERE genexprtid = %i", genexptid);
                    break;
                }
                ImGui::Text("UPDATE genexprt SET inactive = '0' WHERE genexprtid = %i", genexptid);
                ImGui::Text("UPDATE genexprt SET expchar1 = '%c' WHERE genexprtid = %i", activeField, genexptid);
                ImGui::Text("UPDATE genexprt SET expchar2 = '%c' WHERE genexprtid = %i", inActiveField, genexptid);
                ImGui::Text("UPDATE genexprt SET delimiter = '%s' WHERE genexprtid = %i", delimiter, genexptid);
                ImGui::Text("--If anything looks incorrect, use rollback to undo insert, or commit to apply changes");
                ImGui::Text("SELECT * FROM genexprt WHERE genexprtid = %i", genexptid);
                ImGui::Text("--COMMIT");
                ImGui::Text("--ROLLBACK");

                // End the sql output child window
                ImGui::EndChild();

                if (ImGui::BeginTable("ButtonBar", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoHostExtendX /*| ImGuiTableFlags_Borders*/, ImVec2(ImGui::GetWindowSize().x, 0))) {
                    ImGui::TableNextColumn();
                    // Button to copy text to clipboard for easy pasting
                    if (ImGui::Button("Copy to clipboard", ImVec2(140, 0))) { saveGenericExport(1); }
                    ImGui::TableNextColumn();
                    if (ImGui::Button("Save to file", ImVec2(140, 0))) { 
                        saveGenericExport(0); 
                        static bool saved = false;
                        saved = true;
                        if (saved) {
                            ImGui::Text("File saved at: C:\\ImplementationToolbox\\SQL Output\\ % s_GenExprtScript.sql", exportName);
                        }
                    }
                    ImGui::TableNextColumn();
                    ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionMax().x - 453, 0));
                    ImGui::TableNextColumn();
                    // Close button to exit the modal
                    if (ImGui::Button("Close", ImVec2(140, 0))) { ImGui::CloseCurrentPopup(); }
                    
                    // End button bar table
                    ImGui::EndTable();
                }                

                // End Generic Export SQL window
                ImGui::EndPopup();
            }

            // Center window when it opens
            ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionMax().x * .8, ImGui::GetContentRegionMax().y * .8));
            if (ImGui::BeginPopupModal("SQL Fields", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                // Begin SQL Script generation
                ImGui::BeginChild("SqlScriptGeneration", ImVec2(ImGui::GetContentRegionMax().x, ImGui::GetContentRegionMax().y - 55), NULL, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::Text("--Make sure you're modifying the correct export before running the rest of the script ");
                if (rmsDb == "")
                    ImGui::Text("USE <enter database name>;");
                else
                    ImGui::Text("USE %s;", rmsDb);
                ImGui::Text("SELECT * FROM genexprt WHERE genexprtid = %i", genexptid, "\n");
                ImGui::Text("BEGIN TRANSACTION");
                ImGui::Text("UPDATE genflds SET altfldname = 'x' WHERE fldname NOT IN (");
                // Get the list of fields from selected list
                for (int i = 0; i < selectedId.size(); i++)
                {
                    ImGui::SameLine();
                    if (i > 0) {
                        ImGui::Text(",'%s'", selectedFieldName[i].c_str());
                    }
                    // Don't include a comma on the first one
                    else {
                        ImGui::Text("'%s'", selectedFieldName[i].c_str());
                    }
                }
                ImGui::SameLine();
                ImGui::Text(")\n");
                ImGui::Text("AND genexprtid = %i", genexptid);
                ImGui::Text("GO");
                ImGui::Text("--The following script segment will set the field order of each field based on the order you selected them in initially");

                // Write out all the field names, the order they are presented in, and which ID they belong to
                for (int i = 0; i < selectedId.size(); i++)
                {
                    ImGui::Text("UPDATE genflds SET fieldorder = %i WHERE fldname = '%s' AND genexprtid = %i", i, selectedFieldName[i].c_str(), genexptid);
                }

                ImGui::Text("--Run the select to make sure your update was correct before committing");
                ImGui::Text("SELECT * FROM genflds WHERE genexprtid = %i AND altfldname != 'x' ORDER BY fieldorder ASC", genexptid);
                ImGui::Text("--If update was correct run commit, otherwise run a ROLLBACK to undo it");
                ImGui::Text("--ROLLBACK");
                ImGui::Text("--COMMIT");

                // End the script generator child window
                ImGui::EndChild();

                if (ImGui::BeginTable("ButtonBar2", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoHostExtendX /*| ImGuiTableFlags_Borders*/, ImVec2(ImGui::GetWindowSize().x, 0))) {
                    ImGui::TableNextColumn();
                    // Button to copy text to clipboard for easy pasting
                    if (ImGui::Button("Copy to clipboard", ImVec2(140, 0))) { saveGenericFields(1); }
                    ImGui::TableNextColumn();
                    if (ImGui::Button("Save to file", ImVec2(140, 0))) { 
                        saveGenericFields(0); 
                        static bool saved = false;
                        saved = true;
                        if (saved) {
                            ImGui::Text("File saved at: C:\\ImplementationToolbox\\SQL Output\\ % s_GenFldsScript.sql", exportName);
                        }
                    }
                    ImGui::TableNextColumn();
                    ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionMax().x - 453, 0));
                    ImGui::TableNextColumn();
                    // Close button to exit the modal
                    if (ImGui::Button("Close", ImVec2(140, 0))) { ImGui::CloseCurrentPopup(); }

                    // End button bar table
                    ImGui::EndTable();
                }
                // End Popup Modal
                ImGui::EndPopup();
            }
    // End Generic Export Window
    //ImGui::End();
}

/*
    This method resets all local variables to blank.
    Due to the way we assign profile variables to the local values, this also resets the profile object values at the same time.
*/
void reset() {
    std::memset(exportName, 0, sizeof(exportName));
    std::memset(sftpUser, 0, sizeof(sftpUser));
    std::memset(sftpPass, 0, sizeof(sftpPass));
    std::memset(sftpIp, 0, sizeof(sftpIp));
    std::memset(sftpTargetDir, 0, sizeof(sftpTargetDir));
    std::memset(agency, 0, sizeof(agency));
    std::memset(filePrefix, 0, sizeof(filePrefix));
    std::memset(label, 0, sizeof(label));
    std::memset(rmsDb, 0, sizeof(rmsDb));
    std::memset(delimiter, 0, sizeof(delimiter));
    std::memset(profileName, 0, sizeof(profileName));
    std::memset(selected, 0, sizeof(selected));
    std::memset(order, 0, sizeof(order));

    isPinReq = false;
    includeHousing = false;
    exportNow = false;
    combineHousing = false;
    combineStApt = false;
    retainHistory = false;
    secureSSN = false;
    noQuotes = false;

    pinType = 0;
    fileType = 0;
    recordOptn = 0;
    fileOptn = 0;
    createOptn = 0;
    dateFormat = 0;
    isSftp = 2;
    writeDelay = 0;
    genexptid = 0;

    ImGui::OpenPopup("Profile Reset");
}

/// <summary>
/// Send an int through to tell it if you're logging to clipboard or the file.
/// </summary>
/// <param name="option"> 0 represents logging sql to a file, 1 represents logging to clipboard</param>
void saveGenericExport(int option) {
    char fileNameBuffer[1024];
    sprintf(fileNameBuffer, "C:\\ImplementationToolbox\\SQL Output\\%s_GenExprtScript.sql", exportName);
    if (option == 0)
        ImGui::LogToFile(0, fileNameBuffer);
    // Begin the Generic Export option SQL Script output to clipboard
    else if (option == 1)
        ImGui::LogToClipboard();

    ImGui::LogText("--Updating the genexprt database with all provided information\n");
    ImGui::LogText("Use %s\n", rmsDb);
    ImGui::LogText("BEGIN TRANSACTION\n");
    ImGui::LogText("UPDATE genexprt SET filename = '%s' WHERE genexprtid = %i\n", exportName, genexptid);
    ImGui::LogText("UPDATE genexprt SET username = '%s' WHERE genexprtid = %i\n", sftpUser, genexptid);
    ImGui::LogText("UPDATE genexprt SET password = '%s' WHERE genexprtid = %i\n", sftpPass, genexptid);
    ImGui::LogText("UPDATE genexprt SET ipaddress = '%s' WHERE genexprtid = %i\n", sftpIp, genexptid);
    ImGui::LogText("UPDATE genexprt SET targetdir = '%s' WHERE genexprtid = %i\n", sftpTargetDir, genexptid);
    ImGui::LogText("UPDATE genexprt SET agency = '%s' WHERE genexprtid = %i\n", agency, genexptid);
    ImGui::LogText("UPDATE genexprt SET issftp = '%i' WHERE genexprtid = %i\n", isSftp, genexptid);
    switch (isPinReq) {
    case true:
        ImGui::LogText("UPDATE genexprt SET ispinreq = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET ispinreq = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    ImGui::LogText("UPDATE genexprt SET pintype = '%i' WHERE genexprtid = %i\n", pinType, genexptid);
    switch (includeHousing) {
    case true:
        ImGui::LogText("UPDATE genexprt SET inclhousng = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET inclhousng = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    switch (combineHousing) {
    case true:
        ImGui::LogText("UPDATE genexprt SET combhousng = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET combhousng = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    switch (combineStApt) {
    case true:
        ImGui::LogText("UPDATE genexprt SET combstrapt = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET combstrapt = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    ImGui::LogText("UPDATE genexprt SET actcodetyp = '2' WHERE genexprtid = %i\n", genexptid);
    ImGui::LogText("UPDATE genexprt SET recordoptn = '%i' WHERE genexprtid = %i\n", recordOptn, genexptid);
    ImGui::LogText("UPDATE genexprt SET filetype = '%i' WHERE genexprtid = %i\n", fileType, genexptid);
    ImGui::LogText("UPDATE genexprt SET fileoptn = '%i' WHERE genexprtid = %i\n", fileOptn, genexptid);
    switch (exportNow) {
    case true:
        ImGui::LogText("UPDATE genexprt SET exprtnow = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET exprtnow = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    ImGui::LogText("UPDATE genexprt SET createoptn = '%i' WHERE genexprtid = %i\n", createOptn, genexptid);
    ImGui::LogText("UPDATE genexprt SET cashbalfmt = '0' WHERE genexprtid = %i\n", genexptid);
    ImGui::LogText("UPDATE genexprt SET dateformat = '%i' WHERE genexprtid = %i\n", dateFormat, genexptid);
    ImGui::LogText("UPDATE genexprt SET writedelay = '%i' WHERE genexprtid = %i\n", writeDelay, genexptid);
    switch (retainHistory) {
    case true:
        ImGui::LogText("UPDATE genexprt SET retainhist = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET retainhist = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    switch (noQuotes) {
    case true:
        ImGui::LogText("UPDATE genexprt SET noquotes = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET noquotes = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    switch (secureSSN) {
    case true:
        ImGui::LogText("UPDATE genexprt SET securessn = '1' WHERE genexprtid = %i\n", genexptid);
        break;
    case false:
        ImGui::LogText("UPDATE genexprt SET securessn = '0' WHERE genexprtid = %i\n", genexptid);
        break;
    }
    ImGui::LogText("UPDATE genexprt SET inactive = '0' WHERE genexprtid = %i\n", genexptid);
    ImGui::LogText("UPDATE genexprt SET expchar1 = '%c' WHERE genexprtid = %i\n", activeField, genexptid);
    ImGui::LogText("UPDATE genexprt SET expchar2 = '%c' WHERE genexprtid = %i\n", inActiveField, genexptid);
    ImGui::LogText("UPDATE genexprt SET delimiter = '%s' WHERE genexprtid = %i\n", delimiter, genexptid);
    ImGui::LogText("--If anything looks incorrect, use rollback to undo insert, or commit to apply changes\n");
    ImGui::LogText("SELECT * FROM genexprt WHERE genexprtid = %i\n", genexptid);
    ImGui::LogText("--COMMIT\n");
    ImGui::LogText("--ROLLBACK");
    ImGui::LogFinish();

    
}
void saveGenericFields(int option) {
    char fileNameBuffer[1024];
    sprintf(fileNameBuffer, "C:\\ImplementationToolbox\\SQL output\\%s_GenFldsScript.sql", exportName);
    if (option == 0)
        ImGui::LogToFile(0, fileNameBuffer);
    else if(option == 1)
        ImGui::LogToClipboard();
    // Begin SQL Script generation logging to clipboard
    ImGui::LogText("--Make sure you're modifying the correct export before running the rest of the script \n");
    if (profile.getRmsDb() == " ")
        ImGui::LogText("USE <enter database name>;\n");
    else
        ImGui::LogText("USE %s;\n", rmsDb);

    ImGui::LogText("SELECT * FROM genexprt WHERE genexprtid = %i\n", genexptid);
    ImGui::LogText("BEGIN TRANSACTION\n");
    ImGui::LogText("UPDATE genflds SET altfldname = 'x' WHERE fldname NOT IN (");
    // Get the list of fields from selected list
    // Get the list of fields from selected list
    for (int i = 0; i < selectedId.size(); i++)
    {
        ImGui::SameLine();
        if (i > 0) {
            ImGui::Text(",'%s'", selectedFieldName[i].c_str());
        }
        // Don't include a comma on the first one
        else {
            ImGui::Text("'%s'", selectedFieldName[i].c_str());
        }
    }
    ImGui::SameLine();
    ImGui::LogText(")\n");
    ImGui::LogText("AND genexprtid = %i\n", genexptid);
    ImGui::LogText("GO\n");
    ImGui::LogText("--The following script segment will set the field order of each field based on the order you selected them in initially\n");
    // Write out all the field names, the order they are presented in, and which ID they belong to
    for (int i = 0; i < selectedId.size(); i++)
    {
        ImGui::Text("UPDATE genflds SET fieldorder = %i WHERE fldname = '%s' AND genexprtid = %i", i, selectedFieldName[i].c_str(), genexptid);
    }
    ImGui::LogText("--Run the select to make sure your update was correct before committing\n");
    ImGui::LogText("SELECT * FROM genflds WHERE genexprtid = %i AND altfldname != 'x' ORDER BY fieldorder ASC\n", genexptid);
    ImGui::LogText("--If update was correct run commit, otherwise run a ROLLBACK to undo it\n");
    ImGui::LogText("--ROLLBACK\n");
    ImGui::LogText("--COMMIT");
    ImGui::LogFinish();
}