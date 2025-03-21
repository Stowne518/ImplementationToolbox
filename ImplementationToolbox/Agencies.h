#pragma once
#include <vector>
#include <string>

class Agencies
{
private:
	std::vector<std::string> rows;
	std::string fileName;
public:
	void setRows(std::string s) { rows.push_back(s); }
	std::vector<std::string> getRows() { return rows; }
	std::string getRows(int i) { return rows[i]; }

	void setFileName(std::string f) { fileName = f; }
	std::string getFileName() { return fileName; }
};

