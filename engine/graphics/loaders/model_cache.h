#ifndef CH_MODEL_CACHE_H
#define CH_MODEL_CACHE_H

#include "engine/graphics/api/model_data.h"
#include <filesystem>

namespace CHEngine
{
    class ModelCache
    {
    public:
        static bool Save(const std::filesystem::path& cachePath, const PendingModelData& data);
        static bool Load(const std::filesystem::path& cachePath, PendingModelData& outData);

        static std::filesystem::path GetCachePath(const std::filesystem::path& modelPath);
        static bool IsCacheValid(const std::filesystem::path& modelPath);
    };
}

#endif // CH_MODEL_CACHE_H
