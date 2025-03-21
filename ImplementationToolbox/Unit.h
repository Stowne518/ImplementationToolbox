#pragma once
#include "Units.h"

//  Must complete sequentially
//  1. Build Agencies (table: agency) (fields: code, type, name)
//  2. Build Groups (table: grmain) (fields: code, description, default type)
//  3. Build Beats (table: geotab1) (Populated from maps, law beats code_key = 'LWBT', fire beats code_key = '', EMS beats code_key = ''
//  4. Build Stations (table: station) (fields: code, type, agency, description)

class Unit :
    public Units
{
private:
    char unitcode[7];
    char type[5];
    char agency[5];
    char groupcode[7];
    char station[5];
    char district[5];
    char beat[5];

    const char STATUS = 'A';
    // fields to add from unit maintenance in CAD
    // Service, group(unit.groupcode -> code lookup (grmain.grpcode, and grmain.descriptn), district(unit.district -> code lookup(geotab1.code_key = 'LWDS' for law district, code_key = '')), beat, station
public:
    // Constructor
    Unit(char* unitcode, char* type, char* agency, char* groupcode, char* station, char* district, char* beat){
        strncpy_s(this->unitcode, unitcode, 6);
        strncpy_s(this->type, type, 4);
        strncpy_s(this->agency, agency, 4);
        strncpy_s(this->groupcode, groupcode, 6);
        strncpy_s(this->station, station, 4);
        strncpy_s(this->district, district, 4);
        strncpy_s(this->beat, beat, 4);
    }

    // Getters and Setters
    const char* getUnitCode() const { return unitcode; }
    void setUnitCode(const char* unitcode) { strncpy_s(this->unitcode, unitcode, 6); }

    const char* getType() const { return type; }
    void setType(const char* type) { strncpy_s(this->type, type, 4); }

    const char* getAgency() const { return agency; }
    void setAgency(const char* agency) { strncpy_s(this->agency, agency, 4); }

    const char* getGroupcode() const { return groupcode; }
    void setGroupcode(const char* groupcode) { strncpy_s(this->groupcode, groupcode, 4); }

    const char* getStation() const { return agency; }
    void setStation(const char* station) { strncpy_s(this->station, station, 4); }

    const char* getDistrict() const { return district; }
    void setDistrict(const char* district) { strncpy_s(this->district, district, 4); }

    const char* getBeat() const { return beat; }
    void setBeat(const char* beat) { strncpy_s(this->beat, beat, 4); }

    const char getStatus() const { return STATUS; }
};

