#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
class Sql;
class Group;
class Groups;

int groupImport(Sql& sql, Groups& groups, std::string dir);

int insertGroupsSql(Sql& sql, std::vector<Group> group, std::vector<std::string>* result);

void readGroupRows(Groups& groups, std::string dir);

std::vector<Group> buildGroup(Groups& groups);
