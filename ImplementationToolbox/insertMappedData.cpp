#include "genericDataImport.h"
#include "Sql.h"

bool insertMappedData(Sql& sql, std::string query, std::string& message)
{
	// Execute the query to insert the data
	bool success = sql.executeQuery(query, message);
	return success;
}