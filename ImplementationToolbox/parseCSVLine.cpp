#include "genericDataImport.h"

// Function to parse a single line of CSV data
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string token;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            // Toggle the insideQuotes flag when encountering a quote
            insideQuotes = !insideQuotes;
        }
        else if (c == ',' && !insideQuotes) {
            // If we encounter a comma outside quotes, finalize the current token
            token = trim(token); // Remove leading/trailing/spaced strings
            result.push_back(token);
            token.clear();
        }
        else {
            // Append the character to the current token
            token += c;
        }
    }

    // Add the last token to the result
    result.push_back(token);

    return result;
}