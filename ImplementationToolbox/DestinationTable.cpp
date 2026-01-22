#include "DestinationTable.h"

Column::Column() {}
Column::Column(string name, string type, string max, string null)
{
	this->name = name;
	this->type = type;
	this->max = max;
	this->null = null;
}

string Column::Name()
{
	return name;
}

void Column::Name(const string n)
{
	name = n;
}

string Column::Type()
{
	return type;
}

void Column::Type(const string t)
{
	type = t;
}

string Column::Max()
{
	return max;
}

void Column::Max(const string m)
{
	max = m;
}

string Column::Null()
{
	return null;
}

void Column::Null(const string n)
{
	null = n;
}

DestinationTable::DestinationTable()
{
}

void DestinationTable::add_column(string name, string type, string max, string null)
{
	columns.push_back(Column(name, type, max, null));
}

bool DestinationTable::get_adduser()
{
	return adduser;
}

bool DestinationTable::get_addtime()
{
	return addtime;
}

void DestinationTable::set_adduser(bool b)
{
	adduser = b;
}

void DestinationTable::set_addtime(bool b)
{
	addtime = b;
}

vector<Column> DestinationTable::get_columns()
{
	return columns;
}

void DestinationTable::reset()
{
	columns.clear();
	adduser = false;
	addtime = false;
}