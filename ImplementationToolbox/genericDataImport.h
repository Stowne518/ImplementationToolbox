#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>

// Class declartion
class Sql;

bool genericDatImport(std::filesystem::path&, Sql);

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns);
