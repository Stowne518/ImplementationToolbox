#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "AgencyUnit.h"
#include "Agencies.h"
#include "unitbuilder.h"
#include "agencyImport.h"
#include "Sql.h"
#include "imgui.h"

int agencyImport(Sql& sql, Agencies& agencies, std::string dir)
{
    // Vector to hold each agency with their data
    static std::vector<AgencyUnit> agency;
    static std::vector<std::string> rows;

    // buffers for reading agency data into
    static char agencycode[30][5];
    static char agencytype[30][5];
    static char agencyname[30][50];

    // return value for successful entries
    static int recordCount = 0;

    try
    {
        ImGui::Text("Select a file to import: "); ImGui::SameLine();
        std::string fileName = displayFiles(dir + "Agencies");
        agencies.setFileName(dir + "Agencies\\" + fileName);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    static bool agency_imported = false;
    ImGui::SameLine();
    if (!agency_imported)
    {
        if (ImGui::Button("ImportCSV##agency"))
            agency_imported = true;
    }
    if (agency_imported)
    {
        ImGui::BeginDisabled();
        ImGui::Button("ImportCSV");
        ImGui::EndDisabled();
    }
    if (agency_imported)
    {
        static bool read = false;
        while (!read)
        {
            readAgencyRows(agencies, agencies.getFileName());

            rows = agencies.getRows();
            agency = buildAgencies(agencies);
            for (int i = 0; i < agency.size(); i++)
            {
                strncpy_s(agencycode[i], agency[i].getCode(), sizeof(agencycode[i]));
                strncpy_s(agencytype[i], agency[i].getType(), sizeof(agencytype[i]));
                strncpy_s(agencyname[i], agency[i].getName(), sizeof(agencyname[i]));
            }
            // Break while loop after the CSV read is finished
            read = true;
        }

        // Build table for each element we import and allow edits
        if (ImGui::BeginTable("Imported Agencies", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame))
        {
            // Create header row
            ImGui::TableSetupColumn("Code");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Name");
            ImGui::TableHeadersRow();
            for (int i = 0; i < agency.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::InputText(("##agencycode" + std::to_string(i)).c_str(), agencycode[i], IM_ARRAYSIZE(agencycode[i]));
                ImGui::TableSetColumnIndex(1);
                ImGui::InputText(("##agencytype" + std::to_string(i)).c_str(), agencytype[i], IM_ARRAYSIZE(agencytype[i]));
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(300);
                ImGui::InputText(("##agencyname" + std::to_string(i)).c_str(), agencyname[i], IM_ARRAYSIZE(agencyname[i]));

                // Track when updates happen
                agency[i].setCode(agencycode[i]);
                agency[i].setType(agencytype[i]);
                agency[i].setName(agencyname[i]);
            }
            // End Agency table
            ImGui::EndTable();
        }

        bool sqlconnetion = sql._GetConnected();            // Continually make sure we're connected to SQL
        static std::vector<std::string> results;
        if(sqlconnetion && sql._GetDatabase() == "cad")
        {
            if (ImGui::Button("SQL Import##Agency"))
            {
                recordCount = insertAgencySql(sql, agency, &results);
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
int insertAgencySql(Sql& sql, std::vector<AgencyUnit> agencies, std::vector<std::string>* result) {
    int recordCount = 0;
    // Write query into buffer var since we store it with C style strings then convert to string and pass along to SQL        
    for (int i = 0; i < agencies.size(); i++) {
        char buffer[1000];
        std::string query;
        sprintf_s(buffer, "INSERT INTO %s..agency (agencycode, agencytype, agencyname) VALUES ('%s','%s','%s')", sql._GetDatabase().c_str(), agencies[i].getCode(), agencies[i].getType(), agencies[i].getName());
        query = buffer;
        // Check if there is already a unit with the same unit code in the system
        int agencyCount = sql.returnRecordCount("agency", "agencycode", agencies[i].getCode());
        if (agencyCount > 0) {
            ImGui::Text("Query Failed: %s", query.c_str());
            result->push_back("Agency " + (std::string)agencies[i].getCode() + " not added - detected duplicate!");
        }
        else {
            sql.executeQuery(query);
            result->push_back("Agency " + (std::string)agencies[i].getCode() + " added successfully!");
            recordCount++;
        }
    }
    return recordCount;
}

void readAgencyRows(Agencies& agencies, std::string dir)
{
    std::ifstream agencyData;
    agencyData.open(dir);

    if (!agencyData)
        return;
    agencyData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(agencyData, line))
    {
        agencies.setRows(line);
    }
}

std::vector<AgencyUnit> buildAgencies(Agencies& agencies)
{
    std::vector<AgencyUnit> agency;
    std::vector<std::string> rows = agencies.getRows();
    std::string value;

    // Buffers
    char 
        code[5],
        type[5],
        name[50];

    for (int i = 0; i < rows.size(); i++)
    {
        std::vector<std::string> values;
        std::stringstream ss(rows[i]);
        while (std::getline(ss, value, ','))
        {
            values.push_back(value);
        }
        strncpy_s(code, values[0].c_str(), IM_ARRAYSIZE(code));
        strncpy_s(type, values[1].c_str(), IM_ARRAYSIZE(type));
        strncpy_s(name, values[2].c_str(), IM_ARRAYSIZE(name));
        agency.push_back(AgencyUnit(code, type, name));
    }
    return agency;
}