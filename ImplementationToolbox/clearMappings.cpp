#include "genericDataImport.h"
#include "AppLog.h"

void clearMappings(AppLog& log,
    std::vector<std::string>& source_columns,
    std::vector<int>& source_columns_index,
    std::vector<std::vector<std::string>>& destination_columns,
    std::vector<int>& destination_columns_index,
    std::vector<std::string>& buffer_columns,
    std::vector<int>& buffer_columns_index,
    std::vector<std::string>& data_rows,
    std::vector<int>& data_rows_index,
    std::vector<std::string>& insert_rows,
    std::string& table_name,
    bool& loaded_csv,
    bool& load_tables,
    bool& load_columns,
    bool& confirm_mapping,
    bool& data_processed,
    std::string& filepath,
    bool* nulls,
    bool* dups,
    bool& confirm_data,
    std::vector<std::string>& d_col_name,
    std::vector<std::string>& d_col_type,
    std::vector<std::string>& d_col_max,
    std::vector<std::string>& d_col_null,
    std::vector<std::string>& query_results,
    std::vector<std::string>& dup_rows,
    std::vector<bool>& inserted,
    bool& dup_check,
    bool& adduser,
    bool& addtime,
    std::vector<std::string>& sql_message,
    std::vector<std::vector<std::string>>& data_parsed_final)
{
    // Clear null flags
    for (int i = 0; i < 1000; i++)
    {
        if (nulls[i])
            nulls[i] = false;
        if (dups[i])
            dups[i] = false;
    }
    // Clear all vectors
    source_columns.clear();
    destination_columns.clear();
    buffer_columns.clear();
    table_name.clear();
    data_rows.clear();
    insert_rows.clear();
    source_columns_index.clear();
    destination_columns_index.clear();
    data_rows_index.clear();
    buffer_columns_index.clear();
    d_col_name.clear();
    d_col_max.clear();
    d_col_null.clear();
    d_col_type.clear();
    query_results.clear();
    dup_rows.clear();
    inserted.clear();
    sql_message.clear();
    data_parsed_final.clear();

    // Reset all flags
    loaded_csv = false;
    load_tables = false;
    load_columns = false;
    confirm_mapping = false;
    confirm_data = false;
    data_processed = false;
    dup_check = false;
    adduser = false;
    addtime = false;
    filepath.clear();

    // Log the clearing action
    log.AddLog("[INFO] All vectors and flags have been cleared.\n");
}