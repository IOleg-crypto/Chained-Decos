#ifndef CH_PROJECT_SERIALIZER_H
#define CH_PROJECT_SERIALIZER_H

#include "engine/project/project.h"
#include <filesystem>
#include <memory>

namespace Chained
{
class EditorProjectSerializer
{
public:
    static bool Serialize(const std::shared_ptr<Project>& project, const std::filesystem::path& filepath);
    static bool Deserialize(const std::shared_ptr<Project>& project, const std::filesystem::path& filepath);
};
} // namespace Chained

#endif // CH_PROJECT_SERIALIZER_H
