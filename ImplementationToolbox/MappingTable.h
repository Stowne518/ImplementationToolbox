#pragma once
#include <vector>
#include <string>
#include <sstream>
class MappingTable
{
private:
    // Probably going to add a SourceTable and DestinationTable class and replace these
    std::vector<std::string> SourceColumns;
    std::vector<std::string> DestinationColumnNames;
    std::vector<std::string> DestinationColumnTypes;
    std::vector<std::string> DestinationColumnMax;
    std::vector<std::string> DestinationColumnNulls;
    std::vector<std::string> BufferColumns;
    std::vector<std::string> Rows;
    std::vector<int>& BufferColumnIndex;
    bool* nulls;
    bool* duplicate;
    bool* auto_map;
    bool editable;
public:
    MappingTable();
    ~MappingTable();

    // Getters
    std::vector<std::string> getSourceColumns() const { return SourceColumns; }
    std::vector<std::string> getDestinaationColumnNames() const { return DestinationColumnNames; }
    std::vector<std::string> getDestinationColumnTypes() const { return DestinationColumnTypes; }
    std::vector<std::string> getDestinationColumnMax() const { return DestinationColumnMax; }
    std::vector<std::string> getDestinationColumnNulls() const { return DestinationColumnNulls; }
    std::vector<std::string> getBufferColumns() const { return BufferColumns; }
    std::vector<std::string> getRows() const { return Rows; }
    bool* getNulls() const { return nulls; }
    bool* getDuplicate() const { return duplicate; }
    bool* getAutoMap() const { return auto_map; }
    auto getEditable() const { return editable; }
};

