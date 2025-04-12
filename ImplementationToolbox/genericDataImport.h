#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Class declartion
class Sql;
struct AppLog;

void genericDataImport(bool* p_open, Sql, AppLog&, std::string);

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns);

void getRows(std::filesystem::path& dir, std::vector<std::string>& rows);

std::string displayDataFiles(std::string dir);

std::string displayTableNames(Sql& sql);

void displayMappingTable(AppLog&, std::vector<std::string>& s_columns, std::vector<std::string>& d_columns, std::vector<std::string>& b_columns, std::vector<std::string>& rows, std::vector<int>& b_columns_index, std::vector<int>&, std::vector<int>&, int&);

std::vector<std::string> buildInsertQuery(std::string table_name, std::vector<int>& d_columns_index, std::vector<std::string>& d_columns, std::vector<std::string>& rows, std::vector<int>& rows_index, AppLog& log);

void displayDataTable(AppLog&, std::vector<std::string>& d_columns, std::vector<int>& d_columns_index, std::vector<std::string>& rows, std::vector<int>& rows_index, int&);
bool insertMappedData(Sql& sql, std::string query);

void clearMappings(AppLog& log, std::vector<std::string>& source_columns, std::vector<int>& source_columns_index, std::vector<std::string>& destination_columns, std::vector<int>& destination_columns_index, std::vector<std::string>& buffer_columns, std::vector<int>& buffer_columns_index, std::vector<std::string>& data_rows, std::vector<int>& data_rows_index, std::vector<std::string>& insert_rows, std::string& table_name, bool& loaded_csv, bool& load_tables, bool& load_columns, bool& confirm_mapping, std::string& filepath);
