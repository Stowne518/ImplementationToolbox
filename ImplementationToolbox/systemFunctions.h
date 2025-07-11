#pragma once

#include "imgui.h"
#include "readFieldList.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

struct AppLog;

/// <summary>
/// Function used to check the main directory path exists for us to save various files into it. Returns true if it is able to create the directory or already exists. Returns false if it can't create it and doesn't exist
/// </summary>
/// <param name="path">= the directory path that we check to save profiles to</param>
/// <returns></returns>
bool createDirectory(std::string& path, AppLog& log);

// Function to check in the list of generic export fields were correctly read in.
bool genericFieldCheck();

// Display text as either green or red by passing true/false respectively
void DisplayColoredText(const char* text, bool isGreen);

void displayUpdates();

// Function to display a color picker and change the background color - DEPRECATED
// void colorPickerWithBackgroundChange(ImVec4& bgColor);



void showDisabledButton(static char label[], ImVec2 size);

std::vector<std::string> getListOfConnStrings();

bool addButton(const char* label);
bool addButton(const char* label, const ImVec2 size);
bool addButton(const char* label, const ImVec2 size, bool enabled);
bool addButtonTrigger(const char* label, bool* trigger);
bool addButtonTrigger(const char* label, bool* trigger, const ImVec2 size);
bool addButtonTrigger(const char* label, bool* trigger, const ImVec2 size, bool enabled);
