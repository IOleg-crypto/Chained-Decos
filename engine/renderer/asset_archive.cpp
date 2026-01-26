#include "asset_archive.h"
#include <mutex>

namespace CHEngine
{
std::unordered_map<std::string, std::shared_ptr<Asset>, AssetArchive::StringHash, std::equal_to<>>
    AssetArchive::s_Cache;
std::shared_mutex AssetArchive::s_Mutex;

void AssetArchive::Add(const std::string &path, std::shared_ptr<Asset> asset)
{
    std::unique_lock lock(s_Mutex);
    s_Cache[path] = asset;
}

std::shared_ptr<Asset> AssetArchive::Get(const std::string &path)
{
    std::shared_lock lock(s_Mutex);
    auto it = s_Cache.find(path);
    if (it != s_Cache.end())
        return it->second;
    return nullptr;
}

void AssetArchive::Remove(const std::string &path)
{
    std::unique_lock lock(s_Mutex);
    s_Cache.erase(path);
}

void AssetArchive::Clear()
{
    std::unique_lock lock(s_Mutex);
    s_Cache.clear();
}

bool AssetArchive::Exists(const std::string &path)
{
    std::shared_lock lock(s_Mutex);
    return s_Cache.find(path) != s_Cache.end();
}
} // namespace CHEngine
