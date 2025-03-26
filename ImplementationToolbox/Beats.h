#pragma once
#include <string>
#include <vector>
class Beats
{
private:
	std::vector<std::string> rows;
	std::vector<std::string> cols;
	std::string fileName;
public:
	void setCols(std::string s) { cols.push_back(s); }
	std::vector<std::string> getCols() { return rows; }
	std::string getCols(int i) { return cols[i]; }
	int getColsCount() { return cols.size(); }

	void setRows(std::string s) { rows.push_back(s); }
	std::vector<std::string> getRows() { return rows; }
	std::string getRows(int i) { return rows[i]; }
	
	void setFileName(std::string f) { fileName = f; }
	std::string getFileName() { return fileName; }
};

