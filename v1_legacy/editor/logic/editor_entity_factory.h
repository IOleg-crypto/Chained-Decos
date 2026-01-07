#ifndef CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H
#define CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H

#include "core/Base.h"
#include "engine/scene/core/scene.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace CHEngine
{
class CommandHistory;
class SelectionManager;
class EditorSceneManager;

class EditorEntityFactory
{
public:
    EditorEntityFactory();
    ~EditorEntityFactory() = default;

    entt::entity CreateEntity(const std::string &name = "Empty Entity");
    void DeleteEntity(entt::entity entity);

    void AddModel();
    void AddUIButton(const Vector2 &position);
    void AddUIText(const Vector2 &position);

    void OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition);
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H
