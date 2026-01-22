#include "Tables.h"
#include "Sql.h"
#include <fstream>
#include <sstream>

//void DestinationTable::get_columns()
//{
//  
//}

SourceTable::SourceTable(filesystem::path)
{
}

void SourceTable::get_columns()
{
  ifstream dataImport;

  dataImport.open(source_path);
  if (!dataImport.is_open())
    throw runtime_error("Failed to open file: " + source_path.string());

  string line;
  if (getline(dataImport, line))
  {
    stringstream ss(line);
    string token;
    // Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
    while (getline(ss, token, ','))
    {
      source_columns.push_back(token);
    }
  }
  dataImport.close();     // close the file
}