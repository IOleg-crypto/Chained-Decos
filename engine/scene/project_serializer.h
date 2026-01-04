#ifndef CH_PROJECT_SERIALIZER_H
#define CH_PROJECT_SERIALIZER_H

#include "project.h"

namespace CH
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
} // namespace CH

#endif // CH_PROJECT_SERIALIZER_H
