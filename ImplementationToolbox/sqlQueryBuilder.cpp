#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include "sqlQueryBuilder.h"

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
void showSqlQueryBuilderWindow(bool* p_open) {
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
    //if (!ImGui::Begin(("SQL Query Builder"), p_open, window_flags)) {
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
        if (ImGui::BeginMenu("Help", false)) {
            // TODO - add SQL tips and knowledge to a popup window in here
            if (ImGui::MenuItem("Join Help")) {

            }
            ImGui::EndMenu();
        }

        // End menu bar
        ImGui::EndMenuBar();
    }

    ImGui::TextWrapped("Module is WIP.");
    ImGui::TextWrapped("This module will assist with creating complex queries.");
    ImGui::TextWrapped("The idea is that you will enter the table name and columns you want and it will generate a query with an update, join, insert, etc. based on what you need.");
    ImGui::TextWrapped("Because this module is relatively untested PLEASE USE CAUTION AND THOROUGHLY INSPECT ALL SQL QUERIES BEFORE RUNNING THEM.");


    // Constants
    const int TABLE_NAME_SIZE = 256;                                                            // Size of table name in the FOR clause
    const int JOIN_TABLES_SIZE = 250;                                                           // Size of table names allowed in join tables
    const int WHERE_CLAUSE_SIZE = 612;                                                          // Size of array for where clauses
    const int WHERE_COLUMN_SIZE = 75;                                                           // Size of where columns name character size
    const int MAX_COLUMNS = 15;                                                                 // Maximum number of columns allowed for in the where clause
    const int COLUMNS_NAME_SIZE = 250;                                                          // Size of column names array
    const int MAX_HEIGHT_LINES = 10;                                                            // How many columns we display in the UI before a scroll bar is used
    const int MAX_WHERE_HEIGHT = 8;                                                             // How large we display the where clause window before adding scrollbar
    const int TABLE_NAME_INPUTBOX_WIDTH = 225;                                                  // How long to visually make the input box for entering table names
    const int COLUMN_NAME_INPUTBOX_WIDTH = 225;                                                 // How long visually the input box for column names appears

    // Variables for sql query designer
    const char* statements[] = { "SELECT", "UPDATE", "DELETE", "INSERT", "TRUNCATE TABLE" };    // Begin list items for dropdown box
    const char* operators[] = { "=", ">", "<", ">=", "<=", "!=", "IN" };                        // Operators used in SQL Queries
    static int operator_current[MAX_COLUMNS] = {};                                              // Track the currently selected operator
    const char* andor[] = { "AND", "OR" };                                                      // Operators for additional where clauses
    static int andor_current[MAX_COLUMNS] = {};                                                 // Tracks the currently select AND/OR operator
    static int statements_current = 0;                                                          // Track the currently selected statement option
    const char* top[] = { "TOP 1", "TOP 10", "TOP 50", "TOP 1000", "TOP X", "ALL" };            // Options to select only top x records or all of them
    static int top_current = 5;                                                                 // Track the currently selected 'top' option - Default to ALL
    static int top_custom = 0;                                                                  // Store the amount of records to select as a custom value
    static char table_name[TABLE_NAME_SIZE] = "";                                               // char arrays to store text input
    static char whereColumn[MAX_COLUMNS][WHERE_COLUMN_SIZE] = {};                               // 2d array to store column names added to where clause
    static char whereClause[MAX_COLUMNS][WHERE_CLAUSE_SIZE] = {};                               // 2d array to store where clasuses added
    static int where_count = 0;                                                                 // Count how many where clauses have been added
    static char columns[MAX_COLUMNS][COLUMNS_NAME_SIZE] = {};                                   // 2-d char array to store column inputs
    static int draw_lines = 0;                                                                  // Number of columns that will be used                                                          
    static bool single_quotes[MAX_COLUMNS] = {};                                                // Should where clause surround with single quotes?
    static bool joinTbl = false;                                                                // Marker to decide if user will be joining tables together
    static char join_tables[2][JOIN_TABLES_SIZE];                                               // Char arary to store table names for joins
    static char join_columns[2][JOIN_TABLES_SIZE];                                              // Names of the 2 columns to join on
    static bool rename_column[MAX_COLUMNS] = {};                                                // Tells program if user plans to rename column in query
    static char alt_column_name[MAX_COLUMNS][COLUMNS_NAME_SIZE] = {};                           // Store alternate column names
    const char* joinType[] = {"LEFT", "RIGHT", "INNER", "OUTER"};                              // Join options for select statement
    static int currentJoin = 0;                                                                 // Track currently selected join type
    static bool whereJoin[MAX_COLUMNS] = {};                                                    // Used to indicate if the selected where clause is against the joined table
    static bool showAllColumns = false;


    ImGui::SeparatorText("Statement Selection");
    ImGui::SetNextItemWidth(strlen(statements[statements_current]) * 12);
    ImGui::Combo("##sqlstatementtype", &statements_current, statements, IM_ARRAYSIZE(statements), ImGuiComboFlags_WidthFitPreview);
    ImGui::Spacing();
    switch (statements_current) {
    case 0: {
        ImGui::SeparatorText("Choose columns to select (15 max)");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
        ImGui::DragInt("##column_count", &draw_lines, 0.2f, 0, 15); HelpMarker("Click and drag to select number of columns. 0 will include them all.");
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeight() * 1), ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * MAX_HEIGHT_LINES));
        // DONE: Fix app crashing when any child window is open and the app is minimized. Just removed child window altogether. Not really needed when a simple table can replace it
        if (draw_lines > 0) {
            /*
            Decided to use a table instead of child window since it was causing crashing when the program minimizes
            */
            if (ImGui::BeginTable("##custom_columns", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV)) {
                ImGui::TableSetupColumn("Column Name", NULL, COLUMN_NAME_INPUTBOX_WIDTH);
                ImGui::TableSetupColumn("Rename");
                //ImGui::TableSetupColumn("Alt name", NULL, COLUMN_NAME_INPUTBOX_WIDTH);
                ImGui::TableHeadersRow();

                for (int i = 0; i < draw_lines; i++)
                {
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(COLUMN_NAME_INPUTBOX_WIDTH); ImGui::InputTextWithHint(("##column %i" + std::to_string(i)).c_str(), "Enter column name", columns[i], COLUMNS_NAME_SIZE);
                    ImGui::TableNextColumn(); 
                    if (rename_column[i]) {
                        ImGui::SetNextItemWidth(COLUMN_NAME_INPUTBOX_WIDTH); ImGui::InputTextWithHint(("##Altname%i" + std::to_string(i)).c_str(), "Enter the alternate name here", alt_column_name[i], COLUMNS_NAME_SIZE);
                        ImGui::SameLine(); ImGui::Checkbox(("Rename##%i" + std::to_string(i)).c_str(), &rename_column[i]);
                    }
                    else {
                        ImGui::Checkbox(("Rename##%i" + std::to_string(i)).c_str(), &rename_column[i]);
                    }
                }
                // End column table
                ImGui::EndTable();
            }
            ImGui::Checkbox("Show all columns", &showAllColumns); HelpMarker("This option with format the query so the specific columns you've chose will appear first, and then all the columns in the table will come after them.");
        }
        ImGui::Spacing();
        ImGui::SeparatorText("Choose how many records you want displayed"); 
        ImGui::SetNextItemWidth(strlen(top[top_current]) * 12 + 15);
        ImGui::Combo("##top", &top_current, top, IM_ARRAYSIZE(top)); HelpMarker("Used to return the specified number of records.\nUseful if you expect to get a large amount of results in your query.");
        if (top_current == 4) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(75);
            ImGui::InputInt("##topvalue", &top_custom, 0, 0); HelpMarker("Enter a custom number to select.");
        }
        else
            top_custom = 0;
        ImGui::Spacing();
        // FROM Section
        ImGui::SeparatorText("Table Name");
        ImGui::Text("FROM");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(TABLE_NAME_INPUTBOX_WIDTH);
        ImGui::InputTextWithHint("##FROM", "Enter your table name here", table_name, TABLE_NAME_SIZE);

        // JOIN Section
        ImGui::SameLine(); ImGui::Checkbox("Join tables?", &joinTbl);
        if (joinTbl) {
            ImGui::Text("Join Type"); ImGui::SameLine(); ImGui::SetNextItemWidth(75); ImGui::Combo("##joinType", &currentJoin, joinType, IM_ARRAYSIZE(joinType)); ImGui::SameLine(); ImGui::Text("\tTable to join"); ImGui::SameLine(); ImGui::SetNextItemWidth(JOIN_TABLES_SIZE); ImGui::InputTextWithHint("##jointables", "Enter table name to join", *join_tables, JOIN_TABLES_SIZE); ImGui::Spacing(); ImGui::Spacing();
            ImGui::Text("Enter relational columns between tables"); HelpMarker("See the \'help\' section in the menu bar for more information.");
            if (ImGui::BeginTable("join_on_columns", 2, ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_HighlightHoveredColumn)) {
                ImGui::TableSetupColumn("Table 1 Column"); ImGui::TableSetupColumn("Table 2 Column"); ImGui::TableHeadersRow();
                ImGui::TableNextColumn(); ImGui::SetNextItemWidth(COLUMN_NAME_INPUTBOX_WIDTH); ImGui::InputTextWithHint("##joincolumn1", "Enter column from first table", join_columns[0], JOIN_TABLES_SIZE);
                ImGui::TableNextColumn(); ImGui::SetNextItemWidth(COLUMN_NAME_INPUTBOX_WIDTH);;  ImGui::InputTextWithHint("##joincolumn2", "Enter column from second table", join_columns[1], JOIN_TABLES_SIZE);
                ImGui::EndTable();
            }
            ImGui::Spacing();
        }
        else
            ImGui::Spacing();

        // WHERE Section
        ImGui::SeparatorText("Where clause");
        static bool add_where = false;
        if (where_count < 15) {
            if (ImGui::Button("Add where clause", ImVec2(140, 30))) {
                add_where = true;
                where_count++;
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("Maximum Reached", ImVec2(140,30));
            ImGui::EndDisabled();
        }
        // Display add and remove buttons after a where clause has been added
            if (add_where && where_count >= 1) {
                ImGui::SameLine();
                if (ImGui::Button("Remove where clause", ImVec2(140, 30))) {
                    if (add_where < 1)          // If we get less than 1 where clause we hide this button and the where clause data entry boxes
                        add_where = false;
                    else                        // Otherwise, we reduce the number of where clauses included
                        where_count--;
                }
            }
        if (add_where) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeight() * 1), ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * MAX_WHERE_HEIGHT));
            ImGui::BeginChild("Where clauses", ImVec2(-FLT_MIN, 0.0f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
                for (int i = 0; i < where_count; i++) {
                    if (i == 0) {
                        ImGui::Text("WHERE");   // Only display where on the first line
                    }
                    else {
                        // Display option for AND/OR for subsequent lines after first where clause since that is the only way to add extra conditions in SQL
                        ImGui::SetNextItemWidth(50);
                        ImGui::Combo(("##ANDOR" + std::to_string(i)).c_str(), &andor_current[i], andor, IM_ARRAYSIZE(andor));
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(135);
                    ImGui::InputTextWithHint(("##WHERE" + std::to_string(i)).c_str(), "Enter column here", whereColumn[i], IM_ARRAYSIZE(whereColumn));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(50);
                    ImGui::Combo(("##operators1" + std::to_string(i)).c_str(), &operator_current[i], operators, IM_ARRAYSIZE(operators));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(175);
                    ImGui::InputTextWithHint(("##whereclause" + std::to_string(i)).c_str(), "Enter where clause here", whereClause[i], WHERE_CLAUSE_SIZE);
                    ImGui::SameLine();
                    ImGui::Checkbox(("Single Quotes##" + std::to_string(i)).c_str(), &single_quotes[i]); HelpMarker("Add single quotes around the where clause.\nFor the IN operator, enter a comma separated list with no parenthesis and check the single quotes box to auto-format");
                    
                    if (joinTbl) {
                        ImGui::SameLine();                        
                        ImGui::Checkbox(("Join Table##" + std::to_string(i)).c_str(), &whereJoin[i]); HelpMarker("This where clause will be applied using the join table instead of the primary from table.");
                    }
                }
                // End child window for where clause
            ImGui::EndChild();
        }

        // Button alignment
        const float button_x = 140;
        const float button_y = 50;
        ImVec2 button_size(button_x, button_y);
        // Set a center point in the window and reduce it by half the button size to center it
        float centerButton = ((ImGui::GetWindowContentRegionMax().x / 2) - (button_x / 2));
        ImGui::Dummy(ImVec2(10,35));   // Create some space for button
        ImGui::Dummy(ImVec2(centerButton, 0));
        ImGui::SameLine();
        if (ImGui::Button("Generate SQL Query", button_size)) {
            ImGui::OpenPopup("GenerateSQLWindow");
        }

        // Center window when it opens
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("GenerateSQLWindow", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            // SELECT Statement generation
            if (statements_current == 0) {
                // STATEMENT
                ImGui::Text("%s", statements[statements_current]);
                // TOP selection
                if (top_current != 5) {
                    ImGui::SameLine();
                    ImGui::Text("%s", top[top_current]);

                    // Add columns after the top value
                    if (draw_lines > 0) {
                        for (int i = 0; i < draw_lines; i++)
                        {
                            // If there is more than 1 column add commas between column names unless it's the last column
                            if (draw_lines >= 2 && (1 + i) < draw_lines) {
                                ImGui::Text("%s,", columns[i]);
                                ImGui::SameLine();
                            }
                            else if(showAllColumns)
                                ImGui::Text("%s, *", columns[i]);
                            else
                                ImGui::Text("%s", columns[i]);
                        }
                    }
                }
                else if (draw_lines > 0 && top_current == 5) {
                    for (int i = 0; i < draw_lines; i++)
                    {
                        // If there is more than 1 column add commas between column names unless it's the last column
                        if (draw_lines >= 2 && (1 + i) < draw_lines) {
                            ImGui::Text("%s,", columns[i]);
                            ImGui::SameLine();
                        }
                        else
                            ImGui::Text("%s, *", columns[i]);
                    }
                }
                else
                    ImGui::Text("*");

                              

                // FROM
                ImGui::Text("FROM %s", table_name);

                // JOIN
                if (joinTbl) {
                    ImGui::Text("%s", joinType[currentJoin]); ImGui::SameLine(); ImGui::Text("JOIN"); ImGui::SameLine(); ImGui::Text("%s", join_tables[0]);
                    ImGui::Text("ON"); ImGui::SameLine(); ImGui::Text("%s.%s = %s.%s", table_name, join_columns[0], join_tables[0], join_columns[1]);
                }

                // If no where column was specified then we don't include it
                if (where_count > 0) {
                    // WHERE
                    for (int i = 0; i < where_count; i++) {
                        // Handle all single quotes first
                        if (single_quotes[i]) {
                            // Handle first where clause that is also marked for single quotes
                            if (i == 0) {
                                // If first operator is IN marked for single quotes and is first in line - format list and display it
                                if (operators[operator_current[i]] == "IN") {
                                    char output[600];                                           // Used to make sure there is room in the array to display our formatted list
                                    formatList(whereClause[i], output, sizeof(output));         // Format IN list
                                    ImGui::Text("WHERE %s ", whereColumn[i]); ImGui::SameLine(); ImGui::Text("%s", operators[operator_current[i]]); ImGui::SameLine(); ImGui::Text(" (%s)", output);
                                }
                                // Otherwise display the first where clause with normal quotes around it
                                else {
                                    ImGui::Text("WHERE %s %s '%s'", whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                }
                            }
                            // Handle IN single quote formatted list if it's not the first where clause - DONE
                            else if (operators[operator_current[i]] == "IN") {
                                char output[600];                                           // Used to make sure there is room in the array to display our formatted list
                                formatList(whereClause[i], output, sizeof(output));         // Format IN list
                                ImGui::Text("  %s %s %s (%s)", andor[andor_current[i]], whereColumn[i], operators[operator_current[i]], output);
                            }
                            // Handle all left over single quote combinations
                            else {
                                ImGui::Text("  %s %s %s '%s'", andor[andor_current[i]], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                            }
                        }
                        // Handle clauses without quotes
                        else {
                            // Handle first where clause
                            if (i == 0) {
                                // Handle join table where clauses
                                if (whereJoin[i]) {
                                    ImGui::Text("WHERE %s.%s %s %s", join_tables[0], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                }
                                else
                                    ImGui::Text("WHERE %s.%s %s %s", table_name, whereColumn[i], operators[operator_current[i]], whereClause[i]);
                            }
                            // Catch the other clauses with AND/OR instead of WHERE
                            else {
                                // Add table names in case a joined table is added and we need to pull data from that table
                                if(whereJoin[i]) {
                                    ImGui::Text("  %s %s.%s %s %s", andor[andor_current[i]], join_tables[0], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                }
                                else
                                    ImGui::Text("  %s %s.%s %s %s", andor[andor_current[i]], table_name, whereColumn[i], operators[operator_current[i]], whereClause[i]);
                            }
                        }
                    }
                }

                // Close button to exit the modal
                if (ImGui::Button("Close", ImVec2(140, 0))) { ImGui::CloseCurrentPopup(); } ImGui::SameLine();

                // Button for clipboard copying
                if (ImGui::Button("Copy to Clipboard", ImVec2(140, 0))) {
                    ImGui::LogToClipboard();
                    // SELECT Statement generation
                    if (statements_current == 0) {
                        // STATEMENT
                        ImGui::LogText("%s ", statements[statements_current]);
                        // TOP selection
                        if (top_current != 5) {
                            ImGui::SameLine();
                            ImGui::LogText("%s\n", top[top_current]);

                            // Add columns after the top value
                            if (draw_lines > 0) {
                                for (int i = 0; i < draw_lines; i++)
                                {
                                    // If there is more than 1 column add commas between column names unless it's the last column
                                    if (draw_lines >= 2 && (1 + i) < draw_lines) {
                                        ImGui::LogText("%s, ", columns[i]);
                                    }
                                    else if (showAllColumns)
                                        ImGui::LogText("%s, *", columns[i]);
                                    else
                                        ImGui::LogText("%s", columns[i]);
                                }
                            }
                        }
                        else if (draw_lines > 0 && top_current == 5) {
                            for (int i = 0; i < draw_lines; i++)
                            {
                                // If there is more than 1 column add commas between column names unless it's the last column
                                if (draw_lines >= 2 && (1 + i) < draw_lines) {
                                    ImGui::LogText("%s, ", columns[i]);
                                }
                                else
                                    ImGui::LogText("%s, *", columns[i]);
                            }
                        }
                        else
                            ImGui::LogText("*");

                        // FROM
                        ImGui::LogText("\nFROM %s", table_name);

                        // JOIN
                        if (joinTbl) {
                            ImGui::LogText("\n%s JOIN %s", joinType[currentJoin], join_tables);
                            ImGui::LogText("\nON "); ImGui::SameLine(); ImGui::LogText("%s.%s = %s.%s", table_name, join_columns[0], join_tables[0], join_columns[1]);
                        }

                        // If no where column was specified then we don't include it
                        if (where_count > 0) {
                            // WHERE
                            for (int i = 0; i < where_count; i++) {
                                // Handle all single quotes first
                                if (single_quotes[i]) {
                                    // Handle first where clause that is also marked for single quotes
                                    if (i == 0) {
                                        // If first operator is IN marked for single quotes and is first in line - format list and display it
                                        if (operators[operator_current[i]] == "IN") {
                                            char output[600];                                           // Used to make sure there is room in the array to display our formatted list
                                            formatList(whereClause[i], output, sizeof(output));         // Format IN list
                                            ImGui::LogText("\nWHERE %s ", whereColumn[i]); ImGui::SameLine(); ImGui::LogText("%s", operators[operator_current[i]]); ImGui::SameLine(); ImGui::LogText(" (%s)", output);
                                        }
                                        // Otherwise display the first where clause with normal quotes around it
                                        else {
                                            ImGui::LogText("\nWHERE %s %s '%s'", whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                        }
                                    }
                                    // Handle IN single quote formatted list if it's not the first where clause - DONE
                                    else if (operators[operator_current[i]] == "IN") {
                                        char output[600];                                           // Used to make sure there is room in the array to display our formatted list
                                        formatList(whereClause[i], output, sizeof(output));         // Format IN list
                                        ImGui::LogText(" \n%s %s %s (%s)", andor[andor_current[i]], whereColumn[i], operators[operator_current[i]], output);
                                    }
                                    // Handle all left over single quote combinations
                                    else {
                                        ImGui::LogText(" \n%s %s %s '%s'", andor[andor_current[i]], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                    }
                                }
                                // Handle clauses without quotes
                                else {
                                    // Handle first where clause
                                    if (i == 0) {
                                        // Handle join table where clauses
                                        if (whereJoin[i]) {
                                            ImGui::LogText("\nWHERE %s.%s %s %s", join_tables[0], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                        }
                                        else
                                            ImGui::LogText("\nWHERE %s.%s %s %s", table_name, whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                    }
                                    // Catch the other clauses with AND/OR instead of WHERE
                                    else {
                                        // Add table names in case a joined table is added and we need to pull data from that table
                                        if (whereJoin[i]) {
                                            ImGui::LogText(" \n%s %s.[dbo].%s %s %s", andor[andor_current[i]], join_tables[0], whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                        }
                                        else
                                            ImGui::LogText(" \n%s %s.[dbo].%s %s %s", andor[andor_current[i]], table_name, whereColumn[i], operators[operator_current[i]], whereClause[i]);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // End Popup Window
            ImGui::EndPopup();
        }
    }
        // End case 0 switch
        break;
    case 1:

        // End case 1 switch
        break;
    };

    // End the SQL Wizard Window
    // ImGui::End();
}
