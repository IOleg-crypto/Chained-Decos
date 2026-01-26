#ifndef CH_ASSET_ARCHIVE_H
#define CH_ASSET_ARCHIVE_H

#include "asset.h"
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>


namespace CHEngine
{
class AssetArchive
{
public:
    static void Add(const std::string &path, std::shared_ptr<Asset> asset);
    static std::shared_ptr<Asset> Get(const std::string &path);
    static void Remove(const std::string &path);
    static void Clear();
    static bool Exists(const std::string &path);

private:
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }
    };

    static std::unordered_map<std::string, std::shared_ptr<Asset>, StringHash, std::equal_to<>>
        s_Cache;
    static std::shared_mutex s_Mutex;
};
} // namespace CHEngine

#endif // CH_ASSET_ARCHIVE_H
