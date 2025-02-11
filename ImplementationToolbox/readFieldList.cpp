#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "readFieldList.h"

std::vector<std::string> readFieldList()
{
    std::vector<std::string> fn;
    std::string field;
    std::ifstream input("genericfieldlist.txt");

    if (input)
    {
        while (input >> field)
        {
            fn.push_back(field);
        }
        input.close();
        return fn;
    }
    return fn;
}
