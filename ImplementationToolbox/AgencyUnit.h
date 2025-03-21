#pragma once
#include <string>
#include <vector>

class AgencyUnit
{
private:
	char code[5];
	char type[5];
	char name[50];

	
public:
	// Default constructor
	AgencyUnit() = default;

	// Constructor
	AgencyUnit(char* code, char* type, char* name)
	{
		strncpy_s(this->code, code, 4);
		strncpy_s(this->type, type, 4);
		strncpy_s(this->name, name, 50);
	}

	const char* getCode() const { return code; }
	void setCode(const char* code) { strncpy_s(this->code, code, 4); }

	const char* getType() const { return type; }
	void setType(const char* type) { strncpy_s(this->type, type, 4); }
	
	const char* getName() const { return name; }
	void setName(const char* name) { strncpy_s(this->name, name, 50); }
};

