#include "imgui.h"
struct AppLog;

void showSqlQueryBuilderWindow(bool*, AppLog&);

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
