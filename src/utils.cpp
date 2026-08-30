#include "utils.hpp"
#include <algorithm>
#include <cctype>

namespace utils
{
    bool wildcard_match(const std::string& str, const std::string& pattern)
    {
        std::string s = to_lower(str);
        std::string p = to_lower(pattern);
        const char* s_ptr = s.c_str();
        const char* p_ptr = p.c_str();
        const char* s_star = nullptr;
        const char* p_star = nullptr;

        while (*s_ptr)
        {
            if (*p_ptr == '*' || *p_ptr == '?')
            {
                if (*p_ptr == '*')
                {
                    s_star = s_ptr;
                    p_star = p_ptr;
                    p_ptr++;
                }
                else
                {
                    s_ptr++;
                    p_ptr++;
                }
            }
            else if (*p_ptr == *s_ptr)
            {
                s_ptr++;
                p_ptr++;
            }
            else if (s_star)
            {
                s_ptr = s_star + 1;
                p_ptr = p_star;
                s_star = nullptr;
            }
            else
            {
                return false;
            }
        }
        while (*p_ptr == '*') p_ptr++;
        return (*p_ptr == '\0');
    }

    std::string to_lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}