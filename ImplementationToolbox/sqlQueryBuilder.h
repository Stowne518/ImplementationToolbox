#include "imgui.h"
struct AppLog;

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

void showSqlQueryBuilderWindow(bool*, AppLog&);

void generateSQLQuery(
    const int statements_current, 
    const char* statements[],           // array of string pointers
    const int top_current,
    const char* top[],                  // array of string pointers
    const int draw_lines,
    char columns[][COLUMNS_NAME_SIZE],  // 2D array for column names
    const bool& showAllColumns,
    char table_name[],                  // char array for table name
    const bool joinTbl,
    const char* joinType[],             // array of string pointers
    const int currentJoin,
    char join_tables[][JOIN_TABLES_SIZE], // 2D array for join tables
    char join_columns[][JOIN_TABLES_SIZE], // 2D array for join columns
    const int where_count,
    const bool* single_quotes,
    const char* operators[],            // array of string pointers
    const int* operator_current,
    const char whereClause[][WHERE_CLAUSE_SIZE],
    const char whereColumn[][WHERE_COLUMN_SIZE],
    const char* andor[],                // array of string pointers
    const int andor_current[MAX_COLUMNS],
    const bool* whereJoin);

/// <summary>
/// Used to format a comma separated list to be used with an 'IN' operator in SQL by adding single quotes around each value in the list
/// </summary>
/// <param name="input"> = char array that we get from user input as our starting string of text</param>
/// <param name="output"> = The formatted list with single quotes added to either side of commas as well as the first character and last character</param>
/// <param name="outputSize"> = Predetermined array size we used to make sure the output string doesn't get too large</param>
static void formatList(const char* input, char* output, int outputSize) {
    int j = 0;
    output[j++] = '\'';
    for (int i = 0; input[i] != '\0'; ++i) {
        if (input[i] == ',') {
            if (j + 4 >= outputSize) break; // Ensure there's enough space
            output[j++] = '\'';
            output[j++] = ',';
            //output[j++] = ' ';  Don't need this extra space here
            output[j++] = '\'';
        }
        else {
            if (j + 1 >= outputSize) break; // Ensure there's enough space
            output[j++] = input[i];
        }
    }
    if (j + 1 < outputSize) {
        output[j++] = '\'';
    }
    output[j] = '\0'; // Null-terminate the string
}

/// <summary>
// Used to display a (?) symbol beside an item. Pass text you want displayed on mouse hover
/// </summary>
/// <param name="desc"> = text passed to the function that will be displayed when the question mark symbol is hovered.</param>
static void HelpMarker(const char* desc) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
