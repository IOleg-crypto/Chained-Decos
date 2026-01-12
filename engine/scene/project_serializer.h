#ifndef CH_PROJECT_SERIALIZER_H
#define CH_PROJECT_SERIALIZER_H

#include "project.h"

namespace CHEngine
{
class ProjectSerializer
{
public:
    ProjectSerializer(Ref<Project> project);

    void Serialize(const std::filesystem::path &filepath);
    bool Deserialize(const std::filesystem::path &filepath);

private:
    Ref<Project> m_Project;
};
} // namespace CHEngine

#endif // CH_PROJECT_SERIALIZER_H
