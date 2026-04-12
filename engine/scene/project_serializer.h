#ifndef CH_PROJECT_SERIALIZER_H
#define CH_PROJECT_SERIALIZER_H

#include "project.h"
#include <filesystem>

namespace CHEngine
{
// Serializes and deserializes a project configuration; the project is not owned.
class ProjectSerializer
{
public:
    // Binds the serializer to a project instance.
    ProjectSerializer(std::shared_ptr<Project> project);

public:
    // Serializes the bound project to disk.
    bool Serialize(const std::filesystem::path& filepath);
    // Deserializes the project from disk.
    bool Deserialize(const std::filesystem::path& filepath);

private:
    std::shared_ptr<Project> m_Project;
};
} // namespace CHEngine

#endif // CH_PROJECT_SERIALIZER_H
