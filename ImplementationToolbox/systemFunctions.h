#pragma once
#pragma comment(lib, "D3dx9")

#include "imgui.h"
#include "readFieldList.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <D3dx9tex.h>
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

// See imgui wiki for function example: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool displayCentralSquareLogo(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height);

void showDisabledButton(static char label[], ImVec2 size);

std::vector<std::string> getListOfConnStrings();
