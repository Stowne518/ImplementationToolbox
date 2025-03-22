#pragma once
#include <vector>
#include <string>
class Group
{
private:
	char grpcode[7];
	char description[50];
	char deftype[5];
public:
	// Default Constructor
	Group() = default;

	// Constructor
	Group(char* grpcode, char* description, char* deftype)
	{
		strncpy_s(this->grpcode, grpcode, 6);
		strncpy_s(this->description, description, 50);
		strncpy_s(this->deftype, deftype, 4);
	}

	const char* getGroupCode() const { return grpcode; }
	void setGroupCode(const char* code) { strncpy_s(this->grpcode, grpcode, 6); }

	const char* getDescription() const { return description; }
	void setDescription(const char* description) { strncpy_s(this->description, description, 50); }

	const char* getDeftype() const { return deftype; }
	void setDeftype(const char* deftype) { strncpy_s(this->deftype, deftype, 4); }
};

