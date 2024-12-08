#include "Utils.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace utils
{
    std::vector<std::string> split(const std::string &text, const std::string &delimiter)
    {
        std::vector<std::string> vec;
        size_t positionOfNextDelimiter = 0;
        size_t positionOfStartSplit = 0;
        std::string partOfString;
        while((positionOfNextDelimiter = text.find(delimiter, positionOfStartSplit)) != std::string::npos)
        {
            partOfString = text.substr(positionOfStartSplit, positionOfNextDelimiter - positionOfStartSplit);
            vec.push_back(partOfString);
            positionOfStartSplit = positionOfNextDelimiter + delimiter.length();
        }
        vec.push_back(text.substr(positionOfStartSplit, positionOfNextDelimiter));
        return vec;
    }

    int charToInt(char sign)
    {
        auto temp = static_cast<unsigned char>(sign);
        return static_cast<int>(temp);
    }

}// namespace utils
