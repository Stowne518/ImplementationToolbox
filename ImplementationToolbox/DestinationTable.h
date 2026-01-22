#pragma once
#include <vector>
#include <string>
#include "Sql.h"

using namespace std;

class Column 
{
private:
	string name;
	string type;
	string max;
	string null;
public:
	Column();
	Column(string name, string type, string max, string null);
	string Name();
	void Name(const string);
	string Type();
	void Type(const string);
	string Max();
	void Max(const string);
	string Null();
	void Null(const string);
};

class DestinationTable
{
private:
	vector<Column> columns;
	bool adduser;
	bool addtime;
public:
	DestinationTable();
	void add_column(string name, string type, string max, string null);
	vector<Column> get_columns();
	bool get_adduser();
	bool get_addtime();
	void set_adduser(bool);
	void set_addtime(bool);
	void reset();
};
