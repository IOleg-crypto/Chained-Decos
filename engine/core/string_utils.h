#ifndef CH_STRING_UTILS_H
#define CH_STRING_UTILS_H

#include <string>
#include <algorithm>

namespace CHEngine
{

class StringUtils
{
public:
    static std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))::tolower);
        return s;
    }

    static bool EndsWith(const std::string& str, const std::string& suffix)
    {
        if (str.length() >= suffix.length())
        {
            return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
        }
        return false;
    }
};

} // namespace CHEngine

#endif // CH_STRING_UTILS_H
