#pragma once
#include <string>
class Sql;			// Declare SQL class for sql connections later
class Units;		// Declare Units class
class Unit;			// Declare Unit class
class AgencyUnit;	// Declare AgencyUnit class
class Agencies;

void unitBuilder(bool* p_open, Sql& sql, std::string dir);
std::string displayFiles(std::string);