#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H

#include "Scene.h"
#include <memory>
#include <string>

namespace CHEngine
{

class ECSSceneSerializer
{
public:
    ECSSceneSerializer(const std::shared_ptr<Scene> &scene);
    ~ECSSceneSerializer() = default;

    /**
     * @brief Serialize the scene to a YAML file.
     * @param filepath Path to the output .chscene file.
     */
    void Serialize(const std::string &filepath);

    /**
     * @brief Deserialize the scene from a YAML file.
     * @param filepath Path to the input .chscene file.
     * @return true if successful, false otherwise.
     */
    bool Deserialize(const std::string &filepath);

private:
    std::shared_ptr<Scene> m_Scene;
};

} // namespace CHEngine

#endif // SCENE_SERIALIZER_H
