#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Class declartion
class Sql;

void genericDataImport(Sql, std::string);

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns);

void getRows(std::filesystem::path& dir, std::vector<std::string>& rows);

std::string displayDataFiles(std::string dir);

std::string displayTableNames(Sql& sql);

void displayMappingTable(std::vector<std::string>& s_columns, std::vector<std::string>& d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows);
