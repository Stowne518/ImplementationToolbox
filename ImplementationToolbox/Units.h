#pragma once
#include <string>
#include <vector>
class Units
{
private:
	// Unit class variables
	std::string directory;						// Path to unit_csv folder
	std::string fileName;						// File being used to import. Will be pull path with name on end
	std::vector<std::string> columns;			// Container for all column names
	std::vector<std::string> rows;				// Container for full row of commas separated values from file
public:
	void setDir(std::string d) {
		directory = d;
	}
	void setFileName(std::string s) {
		fileName = getDir() + s;
	}
	void setColumns(std::string s) {
		columns.push_back(s);
	}
	void setRows(std::string s) {
		rows.push_back(s);
	}
	// Send row index you want to erase
	void eraseRow(int i) {
		std::string s = getRows(i);
		auto it = std::find(rows.begin(), rows.end(), s);
		rows.erase(it);
	}
	std::string getDir() {
		return directory;
	}
	std::string getFileName() {
		return fileName;
	}
	// Return all columns
	std::vector<std::string> getCols() {
		return columns;
	}
	// Overloaded: return a single column at index i
	std::string getCols(int i){
		return columns[i];
	}
	// Return all rows
	std::vector<std::string> getRows() {
		return rows;
	}
	// Overloaded: Return a single row at index i
	std::string getRows(int i) {
		return rows[i];
	}
};

