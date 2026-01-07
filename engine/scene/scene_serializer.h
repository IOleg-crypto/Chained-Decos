#ifndef CH_SCENE_SERIALIZER_H
#define CH_SCENE_SERIALIZER_H

#include <string>

namespace CH
{
class Scene;

class SceneSerializer
{
public:
    SceneSerializer(Scene *scene);

    void Serialize(const std::string &filepath);
    bool Deserialize(const std::string &filepath);

private:
    Scene *m_Scene;
};
} // namespace CH

#endif // CH_SCENE_SERIALIZER_H
