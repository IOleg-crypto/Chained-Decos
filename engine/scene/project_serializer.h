#ifndef CH_PROJECT_SERIALIZER_H
#define CH_PROJECT_SERIALIZER_H

#include "project.h"
#include <filesystem>

namespace CHEngine
{
    class ProjectSerializer
    {
    public:
        ProjectSerializer(std::shared_ptr<Project> project);

        bool Serialize(const std::filesystem::path &filepath);
        bool Deserialize(const std::filesystem::path &filepath);

    private:
        std::shared_ptr<Project> m_Project;
    };
} // namespace CHEngine

#endif // CH_PROJECT_SERIALIZER_H
