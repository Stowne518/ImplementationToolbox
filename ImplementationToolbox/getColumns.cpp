#include "genericDataImport.h"

std::vector<std::string> getColumns(const std::filesystem::path& dir)
{
    std::ifstream dataImport;
    std::vector<std::string> columns;

    dataImport.open(dir);
    if (!dataImport.is_open())
        throw std::runtime_error("Failed to open file: " + dir.string());

    std::string line;
    if (std::getline(dataImport, line))
    {
        std::stringstream ss(line);
        std::string token;
        // Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
        while (std::getline(ss, token, ','))
        {
            columns.push_back(token);
        }
    }
    dataImport.close();     // close the file
    return columns;
}