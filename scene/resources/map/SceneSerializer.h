#ifndef SCENESERIALIZER_H
#define SCENESERIALIZER_H

#include "GameScene.h"
#include <string>

namespace CHEngine
{
class SceneSerializer
{
public:
    SceneSerializer(const std::shared_ptr<GameScene> &scene);
    ~SceneSerializer() = default;

    // Binary format (.chscene)
    bool SerializeBinary(const std::string &filepath);
    bool DeserializeBinary(const std::string &filepath);

    // JSON format (.json) - for backward compatibility/migration
    bool SerializeJson(const std::string &filepath);
    bool DeserializeJson(const std::string &filepath);

private:
    std::shared_ptr<GameScene> m_Scene;
};
} // namespace CHEngine

#endif // SCENESERIALIZER_H
