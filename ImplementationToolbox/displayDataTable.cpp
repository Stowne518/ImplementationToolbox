#include "genericDataImport.h"
#include "AppLog.h"
#include "imgui_stdlib.h"


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