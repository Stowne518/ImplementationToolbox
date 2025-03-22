#include "groupImport.h"
#include "unitbuilder.h"
#include "Group.h"
#include "Groups.h"
#include "Sql.h"

int groupImport(Sql& sql, Groups& groups, std::string dir)
{
	static std::vector<Group> group;
	static std::vector<std::string> rows;

	// buffers for reading data
	static char groupcode[300][7];
	static char groupdesc[300][50];
	static char groupdeftype[300][5];

    // Value to return based on number of recrods entered
    static int recordCount = 0;

	try
	{
		ImGui::Text("Select a file to import: "); ImGui::SameLine();
		std::string fileName = displayFiles(dir + "Groups");
		groups.setFileName(dir + "Groups\\" + fileName);
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << std::endl;
	}
	static bool group_imported = false;
    ImGui::SameLine();
    if (!group_imported)
    {
        if (ImGui::Button("ImportCSV##groups"))
            group_imported = true;
    }
    if (group_imported)
    {
        ImGui::BeginDisabled();
        ImGui::Button("ImportCSV");
        ImGui::EndDisabled();
    }
    if (group_imported)
    {
        static bool read = false;
        while (!read)
        {
            readGroupRows(groups, groups.getFileName());

            rows = groups.getRows();
            group = buildGroup(groups);
            for (int i = 0; i < group.size(); i++)
            {
                strncpy_s(groupcode[i], group[i].getGroupCode(), sizeof(groupcode[i]));
                strncpy_s(groupdesc[i], group[i].getDescription(), sizeof(groupdesc[i]));
                strncpy_s(groupdeftype[i], group[i].getDeftype(), sizeof(groupdeftype[i]));
            }
            // Break while loop after the CSV read is finished
            read = true;
        }

        // Build table for each element we import and allow edits
        if (ImGui::BeginTable("Imported Groups", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame))
        {
            // Create header row
            ImGui::TableSetupColumn("Group Code");
            ImGui::TableSetupColumn("Description");
            ImGui::TableSetupColumn("Default Type");
            ImGui::TableHeadersRow();
            for (int i = 0; i < group.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::InputText(("##groupcode" + std::to_string(i)).c_str(), groupcode[i], IM_ARRAYSIZE(groupcode[i]));
                ImGui::TableSetColumnIndex(1);
                ImGui::InputText(("##groupdesc" + std::to_string(i)).c_str(), groupdesc[i], IM_ARRAYSIZE(groupdesc[i]));
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(300);
                ImGui::InputText(("##groupdeftype" + std::to_string(i)).c_str(), groupdeftype[i], IM_ARRAYSIZE(groupdeftype[i]));

                // Track when updates happen
                group[i].setGroupCode(groupcode[i]);
                group[i].setDescription(groupdesc[i]);
                group[i].setDeftype(groupdeftype[i]);
            }
            // End Group table
            ImGui::EndTable();
        }

        bool sqlconnetion = sql._GetConnected();            // Continually make sure we're connected to SQL
        static std::vector<std::string> results;
        if (sqlconnetion && sql._GetDatabase() == "cad")
        {
            if (ImGui::Button("SQL Import##Group"))
            {
                recordCount = insertGroupsSql(sql, group, &results);
            }
        }
        else if (sqlconnetion && sql._GetDatabase() != "cad")
        {
            ImGui::BeginDisabled();
            ImGui::Button("SQL Import");
            ImGui::SetItemTooltip("Connect to cad database");
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::Button("SQL Import");
            ImGui::SetItemTooltip("Connect to SQL");
            ImGui::EndDisabled();
        }

        if (ImGui::BeginTable("SQL Results", 1, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Results");
            ImGui::TableHeadersRow();
            for (int i = 0; i < results.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", results[i].c_str());
            }
            // End results table
            ImGui::EndTable();
        }
        ImGui::Dummy(ImVec2(0, 25));
    }
    return recordCount;
}

/// <summary>
/// utilize SQL connetion to send insert statments for each unit added to the agency csv unit file
/// </summary>
/// <param name="sql">is a SQL Class with conneciton string info and query function</param>
/// <param name="units">is a vector container of all units built in our unitImport functions with all their information</param>
int insertGroupsSql(Sql& sql, std::vector<Group> group, std::vector<std::string>* result) {
    int successfulRecords = 0;
    // Write query into buffer var since we store it with C style strings then convert to string and pass along to SQL        
    for (int i = 0; i < group.size(); i++) {
        char buffer[1000];
        std::string query;
        sprintf_s(buffer, "INSERT INTO %s..grmain (grpcode, descriptn, deftype) VALUES ('%s','%s','%s')", sql._GetDatabase().c_str(), group[i].getGroupCode(), group[i].getDescription(), group[i].getDeftype());
        query = buffer;
        // Check if there is already a unit with the same unit code in the system
        int groupCount = sql.returnRecordCount("grmain", "grpcode", group[i].getGroupCode());
        if (groupCount > 0) {
            ImGui::Text("Query Failed: %s", query.c_str());
            result->push_back("Group " + (std::string)group[i].getGroupCode() + " not added - detected duplicate!");
        }
        else {
            sql.executeQuery(query);
            result->push_back("Group " + (std::string)group[i].getGroupCode() + " added successfully!");
            successfulRecords++;
        }
    }
    return successfulRecords;
}

void readGroupRows(Groups& groups, std::string dir)
{
    std::ifstream groupData;
    groupData.open(dir);

    if (!groupData)
        return;
    groupData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(groupData, line))
    {
        groups.setRows(line);
    }
}

std::vector<Group> buildGroup(Groups& groups)
{
    std::vector<Group> group;
    std::vector<std::string> rows = groups.getRows();
    std::string value;

    // Buffers
    char
        groupcode[5],
        deftype[5],
        desc[50];

    for (int i = 0; i < rows.size(); i++)
    {
        std::vector<std::string> values;
        std::stringstream ss(rows[i]);
        while (std::getline(ss, value, ','))
        {
            values.push_back(value);
        }
        strncpy_s(groupcode, values[0].c_str(), IM_ARRAYSIZE(groupcode));
        strncpy_s(desc, values[1].c_str(), IM_ARRAYSIZE(desc));
        strncpy_s(deftype, values[2].c_str(), IM_ARRAYSIZE(deftype));
        group.push_back(Group(groupcode, desc, deftype));
    }
    return group;
}