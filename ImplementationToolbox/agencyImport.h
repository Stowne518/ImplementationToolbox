#pragma once
#include <vector>
#include <string>
#include <filesystem>

class Sql;			// Declare SQL class for later
class Agencies;

int agencyImport(Sql&, Agencies&, std::string);
std::string displayAgencyFiles(std::string dir);
int insertAgencySql(Sql&, std::vector<AgencyUnit>, std::vector<std::string>*);
void readAgencyRows(Agencies&, std::string);
std::vector<AgencyUnit> buildAgencies(Agencies&);