#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "imgui.h"

class Sql;		// Declare SQL class for sql connections later
class Units;	// Declare Units class
class Unit;		// Declare Unit class

void unitInsert(bool *, Sql&, Units&);
std::string displayFiles(std::string);
void insertSql(Sql&, std::vector<Unit>, std::vector<std::string>*, std::string);
void readColumns(Units&);
void readRows(Units&);
std::vector<Unit> buildUnits(Units&);
void DrawRedXMark(ImDrawList* draw_list, ImVec2 pos, float size);
void DrawGreenCheckMark(ImDrawList*, ImVec2, float);