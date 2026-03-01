#ifndef CH_ENVIRONMENT_IMPORTER_H
#define CH_ENVIRONMENT_IMPORTER_H

#include "engine/graphics/environment.h"
#include <memory>
#include <string>

namespace CHEngine
{
class EnvironmentImporter
{
public:
    static std::shared_ptr<EnvironmentAsset> ImportEnvironment(const std::string& path);
    static bool SaveEnvironment(const std::shared_ptr<EnvironmentAsset>& asset, const std::string& path);
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_IMPORTER_H
