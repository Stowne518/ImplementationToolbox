#pragma once
#include <filesystem>
#include <string>
#include <iostream>
// Class def
class Beat;
class Beats;

void beatImport(std::string dir);

std::string displayBeatFiles(std::string dir);

void readBeatColumns(Beats& beats);

void readBeatRows(Beats& beats);

std::vector<Beat> buildBeats(Beats& beats);
