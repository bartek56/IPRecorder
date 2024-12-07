#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

namespace utils {
std::vector<std::string> split(const std::string& s, const std::string &delimiter);
int charToInt(char c);
}

#endif // UTILS_HPP
