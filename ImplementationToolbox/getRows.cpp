#include "genericDataImport.h"

std::vector<std::string> getRows(const std::filesystem::path& dir)
{
    std::ifstream rowData;
    std::vector<std::string> rowTmp;
    rowData.open(dir);

    if (!rowData)
        return rowTmp;
    // Ignore first row in CSV, this is assumed to be the column headers
    rowData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(rowData, line))
    {
        rowTmp.push_back(line);
    }
    rowData.close();        // Close the file
    return rowTmp;
}