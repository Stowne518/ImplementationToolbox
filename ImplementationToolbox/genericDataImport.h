#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "imgui.h"

// Class declartion
class Sql;
struct AppLog;
struct DisplaySettings
{
private:
	bool buttonStyle = false;		// False for expanded, true for compact
	bool column_window = true;
	bool data_window = true;
	bool insert_window = true;
	bool mapoverview = true;

	// Button style specs - compact
	const float
		BUTTON_WIDTH_COMPACT = 120,
		BUTTON_HEIGHT_COMPACT = ImGui::GetTextLineHeight() + 10;

	// Button style specs - expanded
	const float
		BUTTON_WIDTH_EXPANDED = 120,
		BUTTON_HEIGHT_EXPANDED = 40;

public:
	void setButtonStyle(bool value) { buttonStyle = value; }			// Set value if buttons should be expanded or compact
	ImVec2 getButtonStyle()
	{ 
		const ImVec2 BUTTON_STYLE_COMPACT = ImVec2(BUTTON_WIDTH_COMPACT, BUTTON_HEIGHT_COMPACT);
		const ImVec2 BUTTON_STYLE_EXPANDED = ImVec2(BUTTON_WIDTH_EXPANDED, BUTTON_HEIGHT_EXPANDED);

		if (buttonStyle)
		{
			return BUTTON_STYLE_COMPACT;
		}
		else
		{
			return BUTTON_STYLE_EXPANDED;
		}
	}

	void setColumnWindow(bool value) { column_window = value; }
	bool getColumnWindow() { return column_window; }

	void setDataWindow(bool value) { data_window = value; }
	bool getDataWindow() { return data_window; }

	void setInsertWindow(bool value) { insert_window = value; }
	bool getInsertWindow() { return insert_window; }

	void setMappingOverview(bool value) { mapoverview = value; }
	bool getMappingOverview() { return mapoverview; }
};

void genericDataImport(bool* p_open, Sql, AppLog&, std::string);

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns);

void getRows(std::filesystem::path& dir, std::vector<std::string>& rows);

std::string displayDataFiles(std::string dir);

std::string displayTableNames(Sql& sql);

void displayMappingTable(AppLog&, DisplaySettings& ds, std::vector<std::string>& s_columns, std::vector<std::string>& d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_columns_index, std::vector<int>&, std::vector<int>&, int&, bool nulls);

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<int>& d_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, std::vector<int>& rows_index, AppLog& log);

void displayDataTable(AppLog&, std::vector<std::string>& d_columns, std::vector<int>& d_columns_index, std::vector<std::string>& rows, std::vector<int>& rows_index, int&);
bool insertMappedData(Sql& sql, std::string query);

void clearMappings(AppLog& log, std::vector<std::string>& source_columns, std::vector<int>& source_columns_index, std::vector<std::string>& destination_columns, std::vector<int>& destination_columns_index, std::vector<std::string>& buffer_columns, std::vector<int>& buffer_columns_index, std::vector<std::string>& data_rows, std::vector<int>& data_rows_index, std::vector<std::string>& insert_rows, std::string& table_name, bool& loaded_csv, bool& load_tables, bool& load_columns, bool& confirm_mapping, std::string& filepath, bool* nulls);
