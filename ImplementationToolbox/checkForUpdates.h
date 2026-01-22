#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include "AppLog.h"

struct AppLog;

bool checkForUpdates(const std::filesystem::path& source, const std::filesystem::path& destination, AppLog& log);