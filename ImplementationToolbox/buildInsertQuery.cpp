#include "genericDataImport.h"
#include "AppLog.h"

void buildInsertQuery(std::string table_name,
    std::vector<std::string>& insert_rows,
    std::vector<int>& d_columns_index,
    std::vector<std::string>& d_columns,
    std::vector<std::string>& rows,
    std::vector<int>& rows_index,
    std::vector<std::vector<std::string>>& data_parsed_final,
    bool adduser,
    bool addtime,
    bool* nulls,
    AppLog& log)
{
    // DONE: Add multi-threading for large files to prevent UI from freezing and allow each insert to print to log as it's written
    // DONE: Changing logic here to only grab mapped data
    // Added integer vectors for tracking index positions of columns mapped, and columns from the table.    
    try
    {
        do
        {
            for (int i = 0; i < data_parsed_final.size(); i++)
            {
                std::string value;
                std::vector<std::string> values;
                std::string query = "INSERT INTO " + table_name + " (";
                // This should be d_columns, not b_columns
                for (int i = 0; i < d_columns_index.size(); i++)
                {
                    query += d_columns[d_columns_index[i]];
                    if (i != d_columns_index.size() - 1)
                        query += ", ";
                }
                if (addtime && adduser)
                {
                    query += " , adduser, addtime"; // Add adduser and addtime at the end of the insert  statement -- will probably need to make this a parameter later
                }
                else if (adduser)
                {
                    query += " , adduser"; // Add adduser at the end of the insert statement
                }
                else if (addtime)
                {
                    query += " , addtime"; // Add addtime at the end of the insert statement
                }
                query += ") VALUES (";
                for (int j = 0; j < data_parsed_final[i].size(); j++)
                {
                    values.push_back(data_parsed_final[i][j]); // Push back the data from the parsed data vector
                }
                for (int i = 0; i < d_columns_index.size(); i++)
                {
                    // Convert value to uppercase for comparison
                    std::string upper_value = values[i];
                    std::transform(upper_value.begin(), upper_value.end(), upper_value.begin(), ::toupper);
                    if (upper_value == "NULL")                                  // Handle any NULL values passed with correct SQL syntax. Need to ignore case and catch all nulls
                        query += "NULL";
                    else if (nulls[d_columns_index[i]] && values[i] == "")      // If we marked a column as null requried while mapping and it has a blank fill in NULL instead
                        query += "NULL";
                    else
                        query += "'" + values[i] + "'";
                    if (i != d_columns_index.size() - 1)
                        query += ",";
                }
                if (addtime && adduser)
                {
                    query += ",'CSTDATAIMPORT', GETDATE()"; // Add adduser value and run SQL function GETDATE to insert current date/time
                }
                else if (adduser)
                {
                    query += ",'CSTDATAIMPORT'"; // Add adduser value
                }
                else if (addtime)
                {
                    query += ",GETDATE()"; // Run SQL function GETDATE to insert current date/time
                }
                query += ");";
                insert_rows.push_back(query);
                log.AddLog("[INFO] Insert query generated: %s\n", query.c_str());
            }
        } while (insert_rows.size() < data_parsed_final.size());
    }
    catch (const std::exception& e)
    {
        log.AddLog("[ERROR] Failed to build insert query: %s\n", e.what());
    }
}