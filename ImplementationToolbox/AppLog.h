#pragma once
#include "imgui.h"
#include <ctype.h>
#include <stdio.h>
#include <map>
#include <string>
#include <chrono>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <ctime>
struct AppLog
{
	ImGuiTextBuffer		Buf;
	ImGuiTextFilter		Filter;
	ImVector<int>		LineOffsets; // Index to lines offset	
	bool				AutoScroll;

	// Map to store previous states of boolean variables
	std::map<std::string, bool> previousStates;

	AppLog();

	void Clear();
	
	void AddLog(const char* fmt, ...) IM_FMTARGS(2);

	void Draw(const char*, bool*);

	// Function to log message only once when boolean state changes
	void logStateChange(const std::string& varName, bool currentState);
};

static void ShowAppLog(bool* p_open);

