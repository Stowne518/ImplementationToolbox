#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include "oneButtonRefresh_window.h"
#include "systemFunctions.h"

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
#include "AppLog.h"
#if !defined(_MSC_VER) || _MSC_VER >= 1800
#include <inttypes.h>       // PRId64/PRIu64, not avail in some MinGW headers.
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


/*
Code to display generic export generator and options
Use this to get around application and perform all functions
*/
void showOneButtonRefreshWindow(bool* p_open, AppLog& log) {
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = false;
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

    // Local variables for scripting
    static char rmstrnName[128], rmsName[128], trnFilePath[256], liveFilePath[256];

    const int DBNAME_LENGTH = 140;
    const int FILEPATH_LENGTH = 210;

    static bool use_work_area = false;
    static bool samePath = false;
    static bool displayScript = false;

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImVec2 display = ImGui::GetIO().DisplaySize;
    // Set fullscreen options
    // DONE: Fix the fullscreen option. Only works one way and cannot go back to a windowed view after setting fullscreen
    //if (use_work_area)
    //{
    //    ImGui::SetNextWindowPos(use_work_area ? main_viewport->WorkPos : main_viewport->Pos);
    //    ImGui::SetNextWindowSize(use_work_area ? main_viewport->WorkSize : main_viewport->Size);
    //}

    //if (!use_work_area)
    //{
    //    // Center window when it opens
    //    ImVec2 center_window = ImGui::GetMainViewport()->GetCenter();
    //    ImGui::SetNextWindowPos(center_window, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    //    ImGui::SetNextWindowSize(ImVec2(display.x * .75, (display.y * .75)), ImGuiCond_Always);
    //}

    //// Begin One Button Refresh window
    //if (!ImGui::Begin(("One Button RMSTRN Refresh"), p_open, window_flags)) {
    //    // Early out if window is collapsed
    //    ImGui::End();
    //    return;
    //}
    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Menu")) {
            if (ImGui::MenuItem("Fullscreen", NULL, &use_work_area));
            ImGui::Separator();
            if (ImGui::MenuItem("Close")) {
                *p_open = false;
            }
            // End menu
            ImGui::EndMenu();
        }

        // End menu bar
        ImGui::EndMenuBar();
    }

    ImGui::TextWrapped("NOT FULLY TESTED, NEEDS FURTHER WORK. HAS BEEN TESTED ON INTERNAL LAB SUCCESSFULLY.");
    ImGui::TextWrapped("Successful test at: Southwest Central Dispatch, IL!");
    ImGui::TextWrapped("Enter the relevant file paths for the local SQL server to complete a one-button refresh for RMS to RMSTRN. Make sure to include a trailing slash on the file path.");
    ImGui::TextWrapped("Currently we don't check that there is enough space before refreshing, so please check that first. When it is determined there is enough space, you can generate the script here and then run the generated SQL script.");
    if (ImGui::CollapsingHeader("Script explanation")) {
        ImGui::BulletText("User enters correct database names and file paths to the folders\nthat will be used when writing bak files then executes the script");
        ImGui::BulletText("Script backs up the RMS and RMSTRN databases and save\nthem as .bak files to the specified path.");
        ImGui::BulletText("Create a temp table that store the training values\nthat it has before refreshing");
        ImGui::BulletText("Refresh the training database with the bak\nfile created from the live RMS database.");
        ImGui::BulletText("Update the newly refreshed training\nsys_cfg table with the training paths");
        ImGui::BulletText("Verify refresh completed by logging in\nto RMS and check values in System Config");
        ImGui::Separator();
        ImGui::Text("Variable Notes");
        ImGui::Text("Set the rms / rmstrn database name");
        ImGui::Text("Find the backup SQL drive on the client site and\nenter the file path to that local folder WITH THE TRAILING SLASH");
        ImGui::Indent(); ImGui::Text("Example = C:\\SQLBackups\\Backups\\"); ImGui::Unindent();
    }

    ImGui::SeparatorText("Database and filesystem variables");
    // Create group for Live RMS data
    ImGui::BeginGroup();
    {
        // Begin group for rms database data
        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();       // Line up text between groups
            ImGui::Text("RMS Database Name");
            ImGui::SetNextItemWidth(DBNAME_LENGTH);
            ImGui::InputText("##rmsdbName", rmsName, IM_ARRAYSIZE(rmsName));
            // End RMS live database data
            ImGui::EndGroup();
        }

        // Put groups on same X-Axis
        ImGui::SameLine(DBNAME_LENGTH + 50);

        // Begin RMS file path group
        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();       // Line up text between groups
            ImGui::Text("RMS Live database backup file path");
            ImGui::SetNextItemWidth(FILEPATH_LENGTH);
            ImGui::InputText("##rmslivefilepath", liveFilePath, IM_ARRAYSIZE(liveFilePath));
            // End File path group
            ImGui::EndGroup();
        }

        // End Live RMS grouping
        ImGui::EndGroup();
    }
    // Begin Training RMS Data group
    ImGui::BeginGroup();
    {
        // Begin Group for training rms db
        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();       // Line up text between groups
            ImGui::Text("RMSTRN Database Name");
            ImGui::SetNextItemWidth(DBNAME_LENGTH);
            ImGui::InputText("##rmstrnname", rmstrnName, IM_ARRAYSIZE(rmstrnName));
            // End RMS training db group
            ImGui::EndGroup();
        }
        ImGui::SameLine(DBNAME_LENGTH + 50);
        // Begin rmstrn file path group
        ImGui::BeginGroup();
        {
            if (samePath) {
                ImGui::AlignTextToFramePadding();       // Line up text between groups
                ImGui::Text("RMS Training database backup file path");
                ImGui::SetNextItemWidth(FILEPATH_LENGTH);
                ImGui::BeginDisabled();
                ImGui::InputText("##rmstrnfilepath", trnFilePath, IM_ARRAYSIZE(trnFilePath));
                ImGui::EndDisabled();
            }
            if (!samePath) {
                ImGui::AlignTextToFramePadding();       // Line up text between groups
                ImGui::Text("RMS Training database backup file path");
                ImGui::SetNextItemWidth(FILEPATH_LENGTH);
                ImGui::InputText("##rmstrnfilepath", trnFilePath, IM_ARRAYSIZE(trnFilePath));
                // End rmstrn file path group
            }
            ImGui::SameLine();

            ImGui::Checkbox("Same path as live", &samePath);
            // If box for same path locations is checked we disable the text input and copy the live path into training
            if (samePath) {
                strncpy_s(trnFilePath, liveFilePath, FILENAME_MAX);
            }
            ImGui::EndGroup();
        }   

        // End Training data group
        ImGui::EndGroup();
    }

    // Create some space between button and group
    ImGui::Dummy(ImVec2(0, 50));

    // Button alignment
    const float button_x = 140;
    const float button_y = 45;
    ImVec2 button_size(button_x, button_y);

    // Set a center point in the window and reduce it by half the button size to center it
    float centerButton = ((ImGui::GetWindowContentRegionMax().x / 2) - (button_x / 2));
    ImGui::Dummy(ImVec2(centerButton, 0));
    ImGui::SameLine();
    // See if we have all data elements filled out before allowing the script to generate to avoid errors
    bool data_check = dataCheck(rmsName, liveFilePath, rmstrnName, trnFilePath);
    if(Button("Generate Script", button_size, data_check)) { ImGui::OpenPopup("OneButtonRefreshScript"); }

    // Center window when it opens
    ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetMainViewport()->Size.x * .7, ImGui::GetMainViewport()->Size.y * .7));

    if (ImGui::BeginPopupModal("OneButtonRefreshScript", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Begin child window to display the script text in
        ImGui::BeginChild("SQLScriptText", ImVec2(ImGui::GetContentRegionMax().x, ImGui::GetContentRegionMax().y - 60), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);

        // Beign on screen print out of refresh scripts
        ImGui::Text("DECLARE @DB1 NVARCHAR(128) = N'%s' 						--RMS Training Database Name", rmstrnName);
        ImGui::Text("DECLARE @DB2 NVARCHAR(128) = N'%s' 						--RMS Live Database Name", rmsName);
        ImGui::Text("DECLARE @BackupPathForDB1 NVARCHAR(256) = N'%s'			--Leave blank to use default backup path on the database.Enter the backup path for the training DB, including trailing slash.", trnFilePath);
        ImGui::Text("DECLARE @BackupPathForDB2 NVARCHAR(256) = N'%s'			--Leave blank to use default backup path on the database.Enter the backup path for the live DB, including trailing slash.Leave blank to use default backup path in SQL", liveFilePath);
        ImGui::Text("\n-- When the variables are correct, click execute and then select the messages tab to see the progress on the refresh in real - time");
        ImGui::Text("\n\n\n\n         -- - DO NOT EDIT BEYOND THIS POINT-- -");
        ImGui::Text("DECLARE @TRNConfigTable sysname = @DB1 + '.dbo.sys_cfg';");
        ImGui::Text("DECLARE @PRDConfigTable sysname = @DB2 + '.dbo.sys_cfg';");
        ImGui::Text("DECLARE @InsertQuery NVARCHAR(MAX) = '");
        ImGui::Text("INSERT INTO #Temp_System_Config_Backup");
        ImGui::Text("([sysname],");
        ImGui::Text("[defavalue],");
        ImGui::Text("[defasize],");
        ImGui::Text("[agency],");
        ImGui::Text("[state],");
        ImGui::Text("[optname],");
        ImGui::Text("[optvalue],");
        ImGui::Text("[notes],");
        ImGui::Text("[defamemo],");
        ImGui::Text("[console],");
        ImGui::Text("[userid],");
        ImGui::Text("[name],");
        ImGui::Text("[ospropname])");
        ImGui::Text("SELECT[sysname],");
        ImGui::Text("[defavalue],");
        ImGui::Text("[defasize],");
        ImGui::Text("[agency],");
        ImGui::Text("[state],");
        ImGui::Text("[optname],");
        ImGui::Text("[optvalue],");
        ImGui::Text("[notes],");
        ImGui::Text("[defamemo],");
        ImGui::Text("[console],");
        ImGui::Text("[userid],");
        ImGui::Text("[name],");
        ImGui::Text("[ospropname] FROM ' + @TRNConfigTable + '';");
        ImGui::Text("\nDECLARE @UpdateTrainingRMSConfig NVARCHAR(MAX) = '");
        ImGui::Text("TRUNCATE TABLE ' + @TRNConfigTable + '");
        ImGui::Text("INSERT INTO ' + @TRNConfigTable + '");
        ImGui::Text("([sysname],");
        ImGui::Text("[defavalue],");
        ImGui::Text("[defasize],");
        ImGui::Text("[agency],");
        ImGui::Text("[state],");
        ImGui::Text("[optname],");
        ImGui::Text("[optvalue],");
        ImGui::Text("[notes],");
        ImGui::Text("[defamemo],");
        ImGui::Text("[console],");
        ImGui::Text("[userid],");
        ImGui::Text("[name],");
        ImGui::Text("[ospropname])");
        ImGui::Text("SELECT[sysname],");
        ImGui::Text("[defavalue],");
        ImGui::Text("[defasize],");
        ImGui::Text("[agency],");
        ImGui::Text("[state],");
        ImGui::Text("[optname],");
        ImGui::Text("[optvalue],");
        ImGui::Text("[notes],");
        ImGui::Text("[defamemo],");
        ImGui::Text("[console],");
        ImGui::Text("[userid],");
        ImGui::Text("[name],");
        ImGui::Text("\n[ospropname] FROM #Temp_System_Config_Backup';");
        ImGui::Text("DECLARE @CheckFilePaths NVARCHAR(MAX) = '");
        ImGui::Text("IF(");
        ImGui::Text("SELECT COUNT(t1.defavalue)");
        ImGui::Text("FROM ' + @TRNConfigTable + ' t1");
        ImGui::Text("WHERE EXISTS(");
        ImGui::Text("SELECT 1");
        ImGui::Text(" FROM ' + @PRDConfigTable + ' t2");
        ImGui::Text("WHERE t1.defavalue = t2.defavalue");
        ImGui::Text(") AND t1.defavalue LIKE '' % \\ % '') > 1");
        ImGui::Text("PRINT(N''Review Training file paths for any left over live paths'')");
        ImGui::Text("\nELSE PRINT(N''Training File Paths successfully updated!'')';");
        ImGui::Text("---- - Begin Script---- -");
        ImGui::Text("USE master;");
        ImGui::Text("\n--Check if the temporary table exists");
        ImGui::Text("IF OBJECT_ID('tempdb..#Temp_System_Config_Backup') IS NOT NULL");
        ImGui::Text("BEGIN");
        ImGui::Text("-- If it exists, drop it");
        ImGui::Text("DROP TABLE #Temp_System_Config_Backup");
        ImGui::Text("END");
        ImGui::Text("\n-- Create the temporary table");
        ImGui::Text("SET ANSI_NULLS ON");
        ImGui::Text("\nSET QUOTED_IDENTIFIER ON");
        ImGui::Text("\nCREATE TABLE #Temp_System_Config_Backup");
        ImGui::Text("(");
        ImGui::Text("[sysname][varchar](30) NOT NULL,");
        ImGui::Text("[defavalue][varchar](100) NOT NULL,");
        ImGui::Text("[defasize][int] NOT NULL,");
        ImGui::Text("[agency][char](4) NOT NULL,");
        ImGui::Text("[state][char](4) NOT NULL,");
        ImGui::Text("[optname][varchar](15) NOT NULL,");
        ImGui::Text("[optvalue][varchar](40) NOT NULL,");
        ImGui::Text("[notes][varchar](200) NOT NULL,");
        ImGui::Text("[defamemo][text] NULL,");
        ImGui::Text("[sys_cfgid][int] IDENTITY(1, 1) NOT NULL,");
        ImGui::Text("[console][char](10) NOT NULL,");
        ImGui::Text("[userid][varchar](100) NOT NULL,");
        ImGui::Text("[name][varchar](50) NOT NULL,");
        ImGui::Text("[ospropname][varchar](50) NOT NULL,");
        ImGui::Text("PRIMARY KEY CLUSTERED");
        ImGui::Text("(");
        ImGui::Text("[sys_cfgid] ASC");
        ImGui::Text(")WITH(PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = ON, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, FILLFACTOR = 75) ON[PRIMARY]");
        ImGui::Text(") ON[PRIMARY] TEXTIMAGE_ON[PRIMARY]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[sysname]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[defavalue]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT((0)) FOR[defasize]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[agency]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[state]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[optname]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[optvalue]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[notes]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[console]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[userid]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[name]");
        ImGui::Text("ALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[ospropname]");
        ImGui::Text("SET IDENTITY_INSERT #Temp_System_Config_Backup OFF");
        ImGui::Text("EXEC sp_executesql @InsertQuery	-- Run the formatted insert query into the temp table");
        ImGui::Text("\nSELECT * FROM #Temp_System_Config_Backup");
        ImGui::Text("\nPRINT(N'Completed Temporary System Configuration table with training values')");
        ImGui::Text("\nDECLARE @CurrentDate NVARCHAR(8) = CONVERT(NVARCHAR(8), GETDATE(), 112) --Current date in YYYYMMDD format");
        ImGui::Text("DECLARE @SQL NVARCHAR(MAX)");
        ImGui::Text("\n--NO EDIT : Append current date to backup paths");
        ImGui::Text("\nSET @BackupPathForDB1 = @BackupPathForDB1 + @DB1 + '_refresh_' + @CurrentDate + '.bak'");
        ImGui::Text("SET @BackupPathForDB2 = @BackupPathForDB2 + @DB2 + '_refresh_' + @CurrentDate + '.bak'");
        ImGui::Text("\n----NO EDIT : Backup Database1");
        ImGui::Text("SET @SQL = 'BACKUP DATABASE ' + QUOTENAME(@DB1) + ' TO DISK = N''' + @BackupPathForDB1 + ''' WITH NOFORMAT, NOINIT, NAME = N''RMS_Training-Full Database Backup'', SKIP, NOREWIND, NOUNLOAD, STATS = 5'");
        ImGui::Text("EXEC sp_executesql @SQL");
        ImGui::Text("\n-- NO EDIT : Backup Database2");
        ImGui::Text("SET @SQL = 'BACKUP DATABASE ' + QUOTENAME(@DB2) + ' TO DISK = N''' + @BackupPathForDB2 + ''' WITH NOFORMAT, NOINIT, NAME = N''RMS_Prod-Full Database Backup'', SKIP, NOREWIND, NOUNLOAD, STATS = 5'");
        ImGui::Text("EXEC sp_executesql @SQL");
        ImGui::Text("USE master;");
        ImGui::Text("\n--NO EDIT : Restore Database1 with Database2 backup");
        ImGui::Text("SET @SQL = 'USE [master]; ALTER DATABASE ' + QUOTENAME(@DB1) + ' SET SINGLE_USER WITH ROLLBACK IMMEDIATE; RESTORE DATABASE ' + QUOTENAME(@DB1) + ' FROM DISK = N''' + @BackupPathForDB2 + ''' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 5; ALTER DATABASE ' + QUOTENAME(@DB1) + ' SET MULTI_USER;'");
        ImGui::Text("EXEC sp_executesql @SQL");
        ImGui::Text("\n-- After refresh is complete change values that were stored before refresh");
        ImGui::Text("\nEXEC sp_executesql @UpdateTrainingRMSConfig");
        ImGui::Text("-- Double check that all values have changed");
        ImGui::Text("-- If this returns anything but a 0 then there may be new values added to training from live that need to be updated");
        ImGui::Text("EXEC sp_executesql @CheckFilePaths");
        ImGui::Text("\n-- Drop our temp table last to remove from database");
        ImGui::Text("DROP TABLE #Temp_System_Config_Backup");
        ImGui::Text("\nPRINT(N'Congratulations refresh completed successfully!')");
        ImGui::Text("GO");

        // End Child window for displaying text
        ImGui::EndChild();

        // Close button to exit the modal
        if (Button("Close", ImVec2(140, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (Button("Copy to clipboard", ImVec2(140, 0))) {
            ImGui::LogToClipboard();
            ImGui::LogText("\nDECLARE @DB1 NVARCHAR(128) = N'%s' 						--Destination Database Name", rmstrnName);
            ImGui::LogText("\nDECLARE @DB2 NVARCHAR(128) = N'%s' 						--Source Live Database Name", rmsName);
            ImGui::LogText("\nDECLARE @BackupPathForDB1 NVARCHAR(256) = N'%s'			--Leave blank to use default backup path on the database.Enter the backup path for the training DB, including trailing slash.", trnFilePath);
            ImGui::LogText("\nDECLARE @BackupPathForDB2 NVARCHAR(256) = N'%s'			--Leave blank to use default backup path on the database.Enter the backup path for the live DB, including trailing slash.Leave blank to use default backup path in SQL", liveFilePath);
            ImGui::LogText("\n-- When the variables are correct, click execute and then select the messages tab to see the progress on the refresh in real - time");
            ImGui::LogText("\n\n\n\n         -- - DO NOT EDIT BEYOND THIS POINT-- -");
            ImGui::LogText("\nDECLARE @TRNConfigTable sysname = @DB1 + '.dbo.sys_cfg';");
            ImGui::LogText("\nDECLARE @PRDConfigTable sysname = @DB2 + '.dbo.sys_cfg';");
            ImGui::LogText("\nDECLARE @InsertQuery NVARCHAR(MAX) = '");
            ImGui::LogText("\nINSERT INTO #Temp_System_Config_Backup");
            ImGui::LogText("(\n[sysname],");
            ImGui::LogText("\n[defavalue],");
            ImGui::LogText("\n[defasize],");
            ImGui::LogText("\n[agency],");
            ImGui::LogText("\n[state],");
            ImGui::LogText("\n[optname],");
            ImGui::LogText("\n[optvalue],");
            ImGui::LogText("\n[notes],");
            ImGui::LogText("\n[defamemo],");
            ImGui::LogText("\n[console],");
            ImGui::LogText("\n[userid],");
            ImGui::LogText("\n[name],");
            ImGui::LogText("\n[ospropname])");
            ImGui::LogText("\nSELECT[sysname],");
            ImGui::LogText("\n[defavalue],");
            ImGui::LogText("\n[defasize],");
            ImGui::LogText("\n[agency],");
            ImGui::LogText("\n[state],");
            ImGui::LogText("\n[optname],");
            ImGui::LogText("\n[optvalue],");
            ImGui::LogText("\n[notes],");
            ImGui::LogText("\n[defamemo],");
            ImGui::LogText("\n[console],");
            ImGui::LogText("\n[userid],");
            ImGui::LogText("\n[name],");
            ImGui::LogText("\n[ospropname] FROM ' + @TRNConfigTable + '';");
            ImGui::LogText("\nDECLARE @UpdateTrainingRMSConfig NVARCHAR(MAX) = '");
            ImGui::LogText("\nTRUNCATE TABLE ' + @TRNConfigTable + '");
            ImGui::LogText("\nINSERT INTO ' + @TRNConfigTable + '");
            ImGui::LogText("([sysname],");
            ImGui::LogText("[defavalue],");
            ImGui::LogText("[defasize],");
            ImGui::LogText("[agency],");
            ImGui::LogText("[state],");
            ImGui::LogText("[optname],");
            ImGui::LogText("[optvalue],");
            ImGui::LogText("[notes],");
            ImGui::LogText("[defamemo],");
            ImGui::LogText("[console],");
            ImGui::LogText("[userid],");
            ImGui::LogText("[name],");
            ImGui::LogText("[ospropname])");
            ImGui::LogText("SELECT[sysname],");
            ImGui::LogText("[defavalue],");
            ImGui::LogText("[defasize],");
            ImGui::LogText("[agency],");
            ImGui::LogText("[state],");
            ImGui::LogText("[optname],");
            ImGui::LogText("[optvalue],");
            ImGui::LogText("[notes],");
            ImGui::LogText("[defamemo],");
            ImGui::LogText("[console],");
            ImGui::LogText("[userid],");
            ImGui::LogText("[name],");
            ImGui::LogText("\n[ospropname] FROM #Temp_System_Config_Backup';");
            ImGui::LogText("\nDECLARE @CheckFilePaths NVARCHAR(MAX) = '");
            ImGui::LogText("\nIF(");
            ImGui::LogText("\nSELECT COUNT(t1.defavalue)");
            ImGui::LogText("\nFROM ' + @TRNConfigTable + ' t1");
            ImGui::LogText("\nWHERE EXISTS(");
            ImGui::LogText("\nSELECT 1");
            ImGui::LogText(" \nFROM ' + @PRDConfigTable + ' t2");
            ImGui::LogText("\nWHERE t1.defavalue = t2.defavalue");
            ImGui::LogText(") AND t1.defavalue LIKE '' % \\ % '') > 1");
            ImGui::LogText("\nPRINT(N''Review Training file paths for any left over live paths'')");
            ImGui::LogText("\nELSE PRINT(N''Training File Paths successfully updated!'')';");
            ImGui::LogText("\n---- - Begin Script---- -");
            ImGui::LogText("\nUSE master;");
            ImGui::LogText("\n--Check if the temporary table exists");
            ImGui::LogText("\nIF OBJECT_ID('tempdb..#Temp_System_Config_Backup') IS NOT NULL");
            ImGui::LogText("\nBEGIN");
            ImGui::LogText("\n-- If it exists, drop it");
            ImGui::LogText("\nDROP TABLE #Temp_System_Config_Backup");
            ImGui::LogText("\nEND");
            ImGui::LogText("\n-- Create the temporary table");
            ImGui::LogText("\nSET ANSI_NULLS ON");
            ImGui::LogText("\nSET QUOTED_IDENTIFIER ON");
            ImGui::LogText("\nCREATE TABLE #Temp_System_Config_Backup");
            ImGui::LogText("\n(");
            ImGui::LogText("[sysname][varchar](30) NOT NULL,");
            ImGui::LogText("\n[defavalue][varchar](100) NOT NULL,");
            ImGui::LogText("\n[defasize][int] NOT NULL,");
            ImGui::LogText("\n[agency][char](4) NOT NULL,");
            ImGui::LogText("\n[state][char](4) NOT NULL,");
            ImGui::LogText("\n[optname][varchar](15) NOT NULL,");
            ImGui::LogText("\n[optvalue][varchar](40) NOT NULL,");
            ImGui::LogText("\n[notes][varchar](200) NOT NULL,");
            ImGui::LogText("\n[defamemo][text] NULL,");
            ImGui::LogText("\n[sys_cfgid][int] IDENTITY(1, 1) NOT NULL,");
            ImGui::LogText("\n[console][char](10) NOT NULL,");
            ImGui::LogText("\n[userid][varchar](100) NOT NULL,");
            ImGui::LogText("\n[name][varchar](50) NOT NULL,");
            ImGui::LogText("\n[ospropname][varchar](50) NOT NULL,");
            ImGui::LogText("\nPRIMARY KEY CLUSTERED");
            ImGui::LogText("(");
            ImGui::LogText("\n[sys_cfgid] ASC");
            ImGui::LogText(")\nWITH(PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = ON, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON, FILLFACTOR = 75) ON[PRIMARY]");
            ImGui::LogText(") \nON[PRIMARY] TEXTIMAGE_ON[PRIMARY]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[sysname]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[defavalue]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT((0)) FOR[defasize]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[agency]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[state]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[optname]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[optvalue]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[notes]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT(' ') FOR[console]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[userid]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[name]");
            ImGui::LogText("\nALTER TABLE[dbo].[#Temp_System_Config_Backup] ADD  DEFAULT('') FOR[ospropname]");
            ImGui::LogText("\nSET IDENTITY_INSERT #Temp_System_Config_Backup OFF");
            ImGui::LogText("\nEXEC sp_executesql @InsertQuery	-- Run the formatted insert query into the temp table");
            ImGui::LogText("\nSELECT * FROM #Temp_System_Config_Backup");
            ImGui::LogText("\nPRINT(N'Completed Temporary System Configuration table with training values')");
            ImGui::LogText("\nDECLARE @CurrentDate NVARCHAR(8) = CONVERT(NVARCHAR(8), GETDATE(), 112) --Current date in YYYYMMDD format");
            ImGui::LogText("\nDECLARE @SQL NVARCHAR(MAX)");
            ImGui::LogText("\n--NO EDIT : Append current date to backup paths");
            ImGui::LogText("\nSET @BackupPathForDB1 = @BackupPathForDB1 + @DB1 + '_refresh_' + @CurrentDate + '.bak'");
            ImGui::LogText("\nSET @BackupPathForDB2 = @BackupPathForDB2 + @DB2 + '_refresh_' + @CurrentDate + '.bak'");
            ImGui::LogText("\n----NO EDIT : Backup Database1");
            ImGui::LogText("\nSET @SQL = 'BACKUP DATABASE ' + QUOTENAME(@DB1) + ' TO DISK = N''' + @BackupPathForDB1 + ''' WITH NOFORMAT, NOINIT, NAME = N''RMS_Training-Full Database Backup'', SKIP, NOREWIND, NOUNLOAD, STATS = 5'");
            ImGui::LogText("\nEXEC sp_executesql @SQL");
            ImGui::LogText("\n-- NO EDIT : Backup Database2");
            ImGui::LogText("\nSET @SQL = 'BACKUP DATABASE ' + QUOTENAME(@DB2) + ' TO DISK = N''' + @BackupPathForDB2 + ''' WITH NOFORMAT, NOINIT, NAME = N''RMS_Prod-Full Database Backup'', SKIP, NOREWIND, NOUNLOAD, STATS = 5'");
            ImGui::LogText("\nEXEC sp_executesql @SQL");
            ImGui::LogText("\nUSE master;");
            ImGui::LogText("\n--NO EDIT : Restore Database1 with Database2 backup");
            ImGui::LogText("\nSET @SQL = 'USE [master]; ALTER DATABASE ' + QUOTENAME(@DB1) + ' SET SINGLE_USER WITH ROLLBACK IMMEDIATE; RESTORE DATABASE ' + QUOTENAME(@DB1) + ' FROM DISK = N''' + @BackupPathForDB2 + ''' WITH FILE = 1, NOUNLOAD, REPLACE, STATS = 5; ALTER DATABASE ' + QUOTENAME(@DB1) + ' SET MULTI_USER;'");
            ImGui::LogText("\nEXEC sp_executesql @SQL");
            ImGui::LogText("\n-- After refresh is complete change values that were stored before refresh");
            ImGui::LogText("\nEXEC sp_executesql @UpdateTrainingRMSConfig");
            ImGui::LogText("\n-- Double check that all values have changed");
            ImGui::LogText("\n-- If this returns anything but a 0 then there may be new values added to training from live that need to be updated");
            ImGui::LogText("\nEXEC sp_executesql @CheckFilePaths");
            ImGui::LogText("\n-- Drop our temp table last to remove from database");
            ImGui::LogText("\nDROP TABLE #Temp_System_Config_Backup");
            ImGui::LogText("\nPRINT(N'Congratulations refresh completed successfully!')");
            ImGui::LogText("\nGO");

            // End log text input for clipboard
            ImGui::LogFinish();
        }

        // End Popup window
        ImGui::EndPopup();
    }

    // End Refresh window
    // ImGui::End();
}

bool dataCheck(char* rmsDb, char* rmsBkup, char* trnDb, char* trnBkup) {
    if (rmsDb[0] == '\0' || rmsBkup[0] == '\0' || trnDb[0] == '\0' || trnBkup[0] == '\0')
        return false;
    else
        return true;
}