#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

class AppLog;

void getInterfaceFiles(bool* open, AppLog& log);
bool testDirectory(const std::string& rootPath);
std::string getProductLine(AppLog& log);
std::filesystem::path getFilePath(AppLog& log, const std::string& productName, const std::string& rootSourcePath);
std::vector<std::string> getInterfaceNames(AppLog& log, std::filesystem::path& pathToInterfaces);
void displayListOfInterfaces(AppLog& log, const std::vector<std::string>& interfaces, std::vector<std::string>& selectedInterfaces, const std::filesystem::path& pathToInterfaces);
bool checkLocalDirs(AppLog& log, const std::filesystem::path& path);
bool createLocalDirs(AppLog& log, const std::filesystem::path& localDir);
bool moveFiles(AppLog& log, const std::filesystem::path& sourceDir, const std::filesystem::path& destinationDir);