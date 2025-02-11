#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <algorithm>

struct TableItem {
    std::string fieldName;
    int order;
};

bool compareByOrder(const TableItem& a, const TableItem& b) {
    return a.order < b.order;
}

void showTable(std::vector<TableItem>& items) {
    static int draggingIndex = -1;
    static int targetIndex = -1;

    if (ImGui::BeginTable("fieldordertable", ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Field Name");
        ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableHeadersRow();

        // Sorting
        // May need to make 'sortSpecs->SpecsDirty = false' after sorting, doesn't seem like we're doing that
        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending) {
                std::sort(items.begin(), items.end(), compareByOrder);
            }
            else {
                std::sort(items.rbegin(), items.rend(), compareByOrder);
            }
        }

        // Display Rows
        for (int i = 0; i < items.size(); i++) {
            ImGui::TableNextRow();
            // Drag and drop rows
            ImGui::PushID(i);
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable("##row", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    draggingIndex = i;
                }
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", items[i].fieldName.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", items[i].order);

            

            // Handle drag/drop
            if (draggingIndex != -1 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                targetIndex = i;
                if (draggingIndex != targetIndex) {
                    TableItem temp = items[draggingIndex];
                    items.erase(items.begin() + draggingIndex);
                    items.insert(items.begin() + targetIndex, temp);
                    draggingIndex = -1;
                    targetIndex = -1;
                }
            }
        }
        // End ordering table
        ImGui::EndTable();
    }
}

