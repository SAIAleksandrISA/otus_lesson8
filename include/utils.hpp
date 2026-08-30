#pragma once
#include <string>

namespace utils
{
    bool wildcard_match(const std::string& str, const std::string& pattern);
    std::string to_lower(std::string s);
}