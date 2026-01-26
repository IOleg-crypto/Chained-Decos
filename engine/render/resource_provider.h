#ifndef CH_RESOURCE_PROVIDER_H
#define CH_RESOURCE_PROVIDER_H

#include <filesystem>
#include <string>


namespace CHEngine
{
class ResourceProvider
{
public:
    static std::filesystem::path ResolvePath(const std::string &path);
    static bool Exists(const std::string &path);
};
} // namespace CHEngine

#endif // CH_RESOURCE_PROVIDER_H
