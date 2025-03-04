#pragma once
#include "Units.h"
class Unit :
    public Units
{
private:
    char unitcode[7];
    char type[5];
    char agency[5];

public:
    // Constructor
    Unit(char* unitcode, char* type, char* agency){
        strncpy_s(this->unitcode, unitcode, 6);
        strncpy_s(this->type, type, 4);
        strncpy_s(this->agency, agency, 4);
    }

    // Getters and Setters
    const char* getUnitCode() const { return unitcode; }
    void setUnitCode(const char* unitcode) { strncpy_s(this->unitcode, unitcode, 6); }

    const char* getType() const { return type; }
    void setType(const char* type) { strncpy_s(this->type, type, 4); }

    const char* getAgency() const { return agency; }
    void setAgency(const char* agency) { strncpy_s(this->agency, agency, 4); }
};

