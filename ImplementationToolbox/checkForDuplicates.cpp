#include "genericDataImport.h"
#include "AppLog.h"
#include "Sql.h"

//std::mutex sqlMutex

void checkForDuplicates(
    Sql& sql,
    AppLog& log,
    std::string table_name,
    std::vector<std::string>& insert_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& destination_columns_index,
    std::vector<std::vector<std::string>>& destination_columns,
    bool* restrict_duplicates,
    std::vector<std::string>& dup_rows,
    std::vector<bool>& inserted,
    std::atomic<bool>& running
)
{
    // Initialize the vector to store valid rows
    std::vector<std::string> validRows;
    //std::lock_guard<std::mutex> lock(sqlMutex);
    running = true;
    // Loop through each row to insert data
    for (int i = 0; i < inserted.size(); i++)
    {
        int duplicate_count = 0;

        // Loop over each column to check if we need to be concerned with duplicates
        for (int j = 0; j < destination_columns_index.size(); j++)
        {
            // Check for duplicate restricted columns
            if (restrict_duplicates[destination_columns_index[j]])
            {
                // TODO optimize SQL query for checking for duplicates by sending all columns at once and checking for only 1 records in the DB instead of all of them
                if (sql.checkDuplicate(table_name, destination_columns[0][destination_columns_index[j]], data_parsed_final[i][j]) > 0)
                {
                    // Add row to new vector for rows that contain illegal duplicate data
                    dup_rows.push_back(insert_rows[i]);
                    duplicate_count++;
                    break;  // Break if we find a duplicate on this column since the whole row is thrown out we can stop checking
                }
            }
        }
        if (duplicate_count == 0)
        {
            validRows.push_back(insert_rows[i]);    // Only keep rows that pass the duplicate check
        }
    }
    running = false;
    insert_rows = validRows;    // Update the insert_rows vector to only contain valid rows
    return;                     // End function after setting new insert_rows value
}