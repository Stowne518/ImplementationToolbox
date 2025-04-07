#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Class declartion
class Sql;
struct AppLog;

void genericDataImport(Sql, AppLog&, std::string);

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns);

void getRows(std::filesystem::path& dir, std::vector<std::string>& rows);

std::string displayDataFiles(std::string dir);

std::string displayTableNames(Sql& sql);

void displayMappingTable(std::vector<std::string>& s_columns, std::vector<std::string>& d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>&, std::vector<int>&, std::vector<int>&);

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<int>& b_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, AppLog&);

bool insertMappedData(Sql& sql, std::string query);
