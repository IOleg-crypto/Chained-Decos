#ifndef CH_STRING_UTILS_H
#define CH_STRING_UTILS_H

#include <string>
#include <algorithm>

namespace CHEngine::Utils
{
    inline std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }
}

#endif // CH_STRING_UTILS_H
