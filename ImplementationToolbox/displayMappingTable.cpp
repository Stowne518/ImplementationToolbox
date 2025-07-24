#include "genericDataImport.h"
#include "systemFunctions.h"
#include "AppLog.h"

void displayMappingTable(AppLog& log,
    DisplaySettings& ds,
    std::vector<std::string>& s_columns,
    std::vector<std::string>& d_columns_name,
    std::vector<std::string>& d_columns_type,
    std::vector<std::string>& d_columns_max,
    std::vector<std::string>& d_columns_null,
    std::vector<std::string>& b_columns,
    std::vector<std::string>& rows,
    std::vector<int>& b_column_index,
    std::vector<int>& s_columns_index,
    std::vector<int>& d_column_index,
    int& table_len,
    bool* nulls,
    bool* duplicate,
    bool* auto_map,
    bool editable)
{
    std::string value;
    static std::vector<std::string> values;
    static std::vector<std::string>s_columns_buf;
    static bool copied = false;
    ImVec2 button_style = ds.getButtonStyle();
    ImGuiListClipper d_clipper;
    ImGuiListClipper s_clipper;
    // Fix for issue #14: Clear out buffer if clear button has been clicked and we clear out the source columns from the CSV
    if (s_columns.size() == 0)
    {
        s_columns_buf.clear();
        copied = false;
    }
    // Copy current source columns to source column buffer
    if (!copied && s_columns.size() > 0)
    {
        s_columns_buf = s_columns;
        // Populate vector with first row of data for preview
        for (int i = 0; i < rows.size(); i++)
        {
            std::stringstream ss(rows[i]);
            while (std::getline(ss, value, ','))
            {
                values.push_back(value);
            }
        }
        // If there are more columns than sample data fill in blank to prevent crashing
        if (rows.size() < s_columns.size())
        {
            for (int i = rows.size(); i < s_columns.size(); i++)
            {
                values.push_back("");
            }
        }

        copied = true;
    }
    // Prepopulate b_column_index with -1 to show no mapping has been done yet
    if (b_column_index.size() < b_columns.size())
    {
        for (int i = 0; i < b_columns.size(); i++)
        {
            b_column_index.push_back(-1);
        }
    }

    /*
    * Created flexible mapping tables
    * We check the number of columns imported from the source and destination tables
    * and build tables tailored specifically to their column counts
    * This way we don't blow up if the column numbers aren't exactly the same.
    * We can also expand/contract based on the number of columns imported.
    */
    static bool allow_nulls[256] = {};
    const int TEXT_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

    if (*auto_map)   // If we click auto-map button run this once
    {
        for (int i = 0; i < d_columns_name.size(); i++)     // Loop the destination columns against all source column labels
        {
            for (int j = 0; j < s_columns_buf.size(); j++)  // Test agains the buffer so we don't directly change source_columns
            {
                if (d_columns_name[i] == s_columns_buf[j])  // Test destination labels against source, if we find one perform mappings before we display table again
                {
                    log.AddLog("[INFO] Auto-match detected match on %s\n", s_columns_buf[j].c_str());
                    // Map out buffer values first
                    b_column_index[i] = j;                  // bufffer_column_index at pos i gets value of current j. This should be the source buf index position of matching label - this will correlate the matching buffer column to SQL with the matching source column
                    log.AddLog("[DEBUG] Mapped source column %s to SQL column %s\n", s_columns_buf[j].c_str(), d_columns_name[i].c_str());
                    b_columns[i] = s_columns_buf[j];        // Set buffer column at i equal to label of source column buffer at j. 
                    s_columns_buf[j] = "";                  // Set newly mapped source column buffer index to blank to indicate a swamp with the b_column
                    if (d_columns_type[i] == "datetime" || d_columns_null[i] == "YES")
                    {
                        log.AddLog("[DEBUG] Datetime or nullable field detected. Marking column null.\n");
                        nulls[i] = true;                    // If column is a datetime field or is nullable auto-mark nullable when mapping
                    }
                    if (i < d_columns_name.size() - 1)
                        log.AddLog("[INFO] Moving to next SQL column %s\n", d_columns_name[i + 1].c_str());
                    break;                                  // End inner loop because we found what we needed - start checking next column
                }
            }
        }
        *auto_map = false;       // Reset to false so we don't keep attempting to auto_map columns after first pass
    }
    d_clipper.Begin(d_columns_name.size());
    //ImGui::BeginChild("DestinationColumnMapping", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
    ImVec2 mapping_tables_outer_size = ImVec2(ImGui::GetContentRegionAvail().x / 2 - 3.60, ImGui::GetContentRegionAvail().y);
    if (ImGui::BeginTable("DestinationMappingTable", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, mapping_tables_outer_size))
    {
        ImGui::TableSetupColumn("Null", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn("Dup", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Table Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Mapped Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        while (d_clipper.Step())
        {
            for (int i = d_clipper.DisplayStart; i < d_clipper.DisplayEnd; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(i);
                ImGui::Checkbox("##nulls", &nulls[i]);
                ImGui::SetItemTooltip("Check this box to include NULLs instead of blanks for data in column '%s'.", d_columns_name[i]);
                log.logStateChange(("%s", d_columns_name[i] + " allows nulls").c_str(), nulls[i]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Checkbox("##Dups", &duplicate[i]);
                ImGui::SetItemTooltip("Check this box to check for duplicates for data in column '%s' before inserting.", d_columns_name[i]);
                log.logStateChange(("%s", d_columns_name[i] + " restricts duplicates on insert.").c_str(), duplicate[i]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("  %s", d_columns_name[i]);
                ImGui::SetItemTooltip("Data Type: %s\nMax Len: %s\nNullable: %s", d_columns_type[i].c_str(), d_columns_max[i].c_str(), d_columns_null[i].c_str());
                ImGui::TableSetColumnIndex(3);
                if (editable)
                {
                    ImGui::Button(b_columns[i].c_str(), button_style);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_Column"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(int));
                            int payload_n = *(const int*)payload->Data;
                            // Using a swap method to manage column labels
                            const std::string tmp = b_columns[i];
                            b_columns[i] = s_columns_buf[payload_n];
                            // If source column[0] swaps with buffer [1] we need to know that column [0] from source maps to column [1] for SQL columns
                            // Use this to set the source column index position to the buffer button it swapped places with
                            // Verify against the s_column vector at the end and correct any mismatched values
                            if (s_columns_buf[payload_n] != "")
                            {
                                b_column_index[i] = payload_n;
                                log.AddLog("[INFO] Source column index added to buffer column index %i = %i\n", i, payload_n);
                            }
                            else
                            {
                                b_column_index[i] = -1;                 // Set buffer back to -1 to indicate no mapping if we swap with a source column that is blank
                            }

                            s_columns_buf[payload_n] = tmp;             // Moving s_column swap after we store the index location to buffer column

                            // Verify newly assigned buffer index against untouched source index
                            log.AddLog("[DEBUG] Verifying buffer index position against source columns index...\n");
                            for (int j = 0; j < b_column_index.size(); j++)
                            {
                                // Skip if label is blank
                                if (b_columns[i] != "" && b_columns[j] != s_columns[b_column_index[j]])
                                {
                                    // If the buffer label isn't equal to the source label we know the source buffer has been misaligned.
                                    // Correct buffer label by comparing to source labels and use that position
                                    log.AddLog("[WARN] Detected mismatched indexes! Attempting to match now...\n");
                                    auto it = std::find(s_columns.begin(), s_columns.end(), b_columns[j]);          // Find label value of buffer column at position j within the source column vector
                                    int s_col = std::distance(s_columns.begin(), it);                               // Find distance to label located before and return position number
                                    b_column_index[j] = s_col;                                                      // Update buffer column to have correct index from source vector
                                    log.AddLog("[DEBUG] Buffer column index matched %i set to source column index %i\n", j, s_col);
                                }
                            }
                            log.AddLog("[DEBUG] Buffer column index positions verified against source columns index successfully.\n");
                            // Display final b_column_index values, when we're done it should match the order and source index of each column added
                            log.AddLog("[DEBUG] b_column_index positions =  ");
                            for (int i = 0; i < b_column_index.size(); i++)
                            {
                                if (i == b_column_index.size() - 1)
                                    log.AddLog("%i", b_column_index[i]);
                                else
                                    log.AddLog("%i, ", b_column_index[i]);
                            }
                            log.AddLog("\n");
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
                else    // If not editable we just display the label as text for easier rendering
                {
                    ImGui::Text("%s", b_columns[i].c_str());
                }
                ImGui::PopID();
            }
        }d_clipper.End();    // End the list clipper rendering
        // End column mapping
        ImGui::EndTable();
    }

    // End destination column mapping child window
    //ImGui::EndChild();
    ImGui::SameLine();
    s_clipper.Begin(s_columns.size());
    //ImGui::BeginChild("SourceColumnMapping", ImVec2(0,0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize);
    if (ImGui::BeginTable("SourceMappingTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, mapping_tables_outer_size))
    {
        ImGui::TableSetupColumn(" Source Columns ", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn(" Sample data", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        while (s_clipper.Step())
        {
            for (int i = s_clipper.DisplayStart; i < s_clipper.DisplayEnd; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (editable)
                {
                    ImGui::PushID(i);
                    ImGui::Button((s_columns_buf[i]).c_str(), button_style);
                    ImGui::PopID();
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                    {
                        ImGui::SetDragDropPayload("DND_Column", &i, sizeof(int));
                        // Display prevew
                        ImGui::Text("Mapping column %s", s_columns_buf[i]);
                        ImGui::EndDragDropSource();
                    }
                }
                else
                {
                    ImGui::Text("%s", s_columns_buf[i].c_str());
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", /*s_columns[i].c_str(),*/ values[i].c_str());
            }
        } s_clipper.End();

        // End source column mapping table
        ImGui::EndTable();
    }

    // End Source column mapping
    //ImGui::EndChild();
}