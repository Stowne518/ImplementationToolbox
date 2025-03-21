#pragma once
#include <vector>
#include <string>

class Sql;			// Declare SQL class for later
class Agencies;

void agencyImport(Sql&, Agencies&, std::string);
void insertAgencySql(Sql&, std::vector<AgencyUnit>, std::vector<std::string>*);
void readAgencyRows(Agencies&, std::string);
std::vector<AgencyUnit> buildAgencies(Agencies&);