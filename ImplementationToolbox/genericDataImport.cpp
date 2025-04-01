#include "genericDataImport.h"
#include "Sql.h"

bool genericDataImport(std::filesystem::path& dir, Sql sql)
{
	std::vector<std::string> columns;
	// readColumns(dir, columns);

	// Default return
	return false;
}

void getColumns(std::filesystem::path& dir, std::vector<std::string>& columns) {
	std::ifstream dataImport;

	dataImport.open(dir);
	std::string line;
	std::getline(dataImport, line);
	std::stringstream ss(line);
	std::string token;
	// Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
	while (std::getline(ss, token, ',')) {
		columns.push_back(token);
	}
}