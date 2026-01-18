#ifndef CH_COMPONENT_UI_H
#define CH_COMPONENT_UI_H

#include "engine/scene/entity.h"

namespace CHEngine
{
namespace ComponentUI
{
void DrawTag(Entity entity);
void DrawTransform(Entity entity);
void DrawModel(Entity entity);
void DrawMaterial(Entity entity, int hitMeshIndex = -1);
void DrawCollider(Entity entity);
void DrawRigidBody(Entity entity);
void DrawSpawn(Entity entity);
void DrawPlayer(Entity entity);
void DrawPointLight(Entity entity);
void DrawAudio(Entity entity);
void DrawHierarchy(Entity entity);
void DrawNativeScript(Entity entity);
void DrawAnimation(Entity entity);

void DrawAddComponentPopup(Entity entity);
} // namespace ComponentUI
} // namespace CHEngine

#endif // CH_COMPONENT_UI_H
