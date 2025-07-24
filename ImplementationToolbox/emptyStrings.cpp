#include "genericDataImport.h"

// Function to check if a vector of strings is empty
bool emptyStrings(std::vector<std::string> vec)
{
    bool vectorEmpty = true;                        // Set vectorEmpty to true by default to make the function prove its false
    for (const auto& str : vec)                     // Loop through each string within the vector
    {
        if (!str.empty())
        {
            return vectorEmpty = false;             // As soon as we find a string that isn't empty we can return since we know the vector isn't empty
        }
    }

    return vectorEmpty;                             // Otherwise we return true to show the vector was empty
}