#include "genericDataImport.h"
#include "Sql.h"
#include "AppLog.h"
#include <thread>

void checkForDuplicates_Thread(
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
    std::thread(
        checkForDuplicates,
        std::ref(sql),
        std::ref(log),
        std::move(table_name),
        std::ref(insert_rows),
        std::ref(data_parsed_final),
        std::ref(destination_columns_index),
        std::ref(destination_columns),
        restrict_duplicates,
        std::ref(dup_rows),
        std::ref(inserted),
        std::ref(running)
    ).detach();
}