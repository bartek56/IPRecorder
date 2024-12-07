#include "Utils.hpp"

namespace utils
{
    std::vector<std::string> split(const std::string &s, const std::string &delimiter)
    {
        std::vector<std::string> vec;
        size_t pos = 0;
        size_t newPos = 0;
        std::string token;
        while((pos = s.find(delimiter, newPos)) != std::string::npos)
        {
            token = s.substr(newPos, pos - newPos);
            vec.push_back(token);
            newPos = pos + delimiter.length();
        }
        vec.push_back(s.substr(newPos, pos));
        return vec;
    }

    int charToInt(char c)
    {
        auto temp = static_cast<unsigned char>(c);
        return static_cast<int>(temp);
    }

}// namespace utils
