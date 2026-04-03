#ifndef CH_ASSIMP_IMPORTER_H
#define CH_ASSIMP_IMPORTER_H

#include "engine/graphics/loaders/model_loader.h"
#include <filesystem>

namespace CHEngine
{
    class AssimpImporter
    {
    public:
        static PendingModelData Import(const std::filesystem::path& path, int samplingFPS);
    };
}
#endif
