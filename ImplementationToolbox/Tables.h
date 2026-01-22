#pragma once
#include <vector>
#include <string>
#include <filesystem>

class Sql;

using namespace std;

class ITable
{
public:
	virtual void get_columns() = 0;
	virtual ~ITable() = default;
};

class SourceTable : ITable
{
private:
	char* source_type; // SQL for SQL Table, CSV for CSV file
	vector<string> source_columns;
	vector<string> data_rows;
	filesystem::path source_path;
public:
	SourceTable(filesystem::path = "");
	void get_columns() override;
};

//class Column
//{
//private:
//	string name;
//	string type;
//	string max;
//	string null;
//public:
//	Column();
//	Column(string name, string type, string max, string null);
//};

//class DestinationTable : ITable
//{
//private:
//	vector<Column> columns;
//	bool adduser;
//	bool addtime;
//	Sql& sql;
//public:
//	DestinationTable();
//	void get_columns() override;
//	void add_column(string name, string type, string max, string null);
//	bool get_adduser();
//	bool get_addtime();
//	void set_adduser(bool);
//	void set_addtime(bool);
//};