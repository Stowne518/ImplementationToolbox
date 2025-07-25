#include "genericDataImport.h"
#include "AppLog.h"

// Function to process data after dependencies are completed
void processData(std::vector<std::string>& data_rows,
    std::vector<std::vector<std::string>>& data_parsed_final,
    std::vector<int>& buffer_columns_index,
    std::vector<int>& destination_columns_index,
    std::vector<int>& data_rows_index,
    AppLog& log,
    bool& cleanup)
{
    // Map out buffer index values from source column index positions
    for (int i = 0; i < buffer_columns_index.size(); i++)
    {
        if (buffer_columns_index[i] != -1)                         // We use -1 to indicate that the columns was not mapped and can be ignored
        {
            destination_columns_index.push_back(i);                 // Map SQL table columns in order they were mapped
            data_rows_index.push_back(buffer_columns_index[i]);     // Get index position of data that matches columns to the SQL tables they were mapped to
        }
    }

    // Log index values for debugging
    log.AddLog("[DEBUG] destination_column_index positions =  ");
    for (int i = 0; i < destination_columns_index.size(); i++)
    {
        if (i == destination_columns_index.size() - 1)
            log.AddLog("%i", destination_columns_index[i]);
        else
            log.AddLog("%i, ", destination_columns_index[i]);
    }
    log.AddLog("\n");

    log.AddLog("[DEBUG] data_rows_index =  ");
    for (int i = 0; i < data_rows_index.size(); i++)
    {
        if (i == data_rows_index.size() - 1)
            log.AddLog("%i", data_rows_index[i]);
        else
            log.AddLog("%i, ", data_rows_index[i]);
    }
    log.AddLog("\n");

    // Remove values from data_rows that aren't mapped to a column in SQL
    for (int i = 0; i < data_rows.size(); i++)
    {
		std::vector<std::string> data_tmp;                              // Create a temporary vector to hold the parsed out strings from the CSV line
        std::vector<std::string> values = parseCSVLine(data_rows[i]);   // Parse out the CSV line into a vector of strings
        if (!emptyStrings(values))                                      // Detect if the previously parsed line was empty, if so don't add to data_parsed_final -- #22
        {
            for (int j = 0; j < destination_columns_index.size(); j++)  // Loop over destination columns index size and use the data_rows_index to determine to which columns we're keeping
            {
                // If we get an empty cell we push back an empty string
                if (values[data_rows_index[j]] == "")
                {
                    data_tmp.push_back("");
                }
                else
                {
                    data_tmp.push_back(values[data_rows_index[j]]);
                }
            }
            data_parsed_final.push_back(data_tmp); // Push vector of parsed out strings into data_parsed_final to use each data piece of only selected data
        }
        else
        {
            log.AddLog("[WARN] Detected empty line in row %i of the CSV file. Skipping...\n", i);
        }
    }
    cleanup = true; // Tell application cleanup has been completed and we won't revisit this section
}