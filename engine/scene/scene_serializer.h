#ifndef CH_SCENE_SERIALIZER_H
#define CH_SCENE_SERIALIZER_H

#include "engine/scene/scene.h"
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

class SceneSerializer
{
public:
    SceneSerializer(Scene* scene);

    bool Serialize(const std::string& filepath);
    bool Deserialize(const std::string& filepath);

    std::string SerializeToString();
    bool DeserializeFromString(const std::string& yaml);

private:
    static void SerializeEntity(YAML::Emitter& out, Entity entity);

    Scene* m_Scene;
};
} // namespace CHEngine

#endif // CH_SCENE_SERIALIZER_H
