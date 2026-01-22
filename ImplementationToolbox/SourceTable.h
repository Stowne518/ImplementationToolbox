#pragma once
#include <vector>
#include <string>

using namespace std;

class SourceTable
{
private:
	char* source_type; // SQL for SQL Table, CSV for CSV file
	vector<string> source_columns;
	vector<string> data_rows;
};

