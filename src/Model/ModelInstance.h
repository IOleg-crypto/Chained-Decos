//
// Created by I#Oleg
//

#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <Collision/CollisionSystem.h>
#include <raylib.h>
#include <string>

class ModelInstance
{
  private:
    Vector3 m_position;                      // Takes position(x , y , z)
    Model *m_pModel;                         //  Take model
    float m_scale;                           // Scale model;
    std::string m_modelName;                 // Name of model
    Texture2D m_texture;                     // Texture of model (.obj or gltf)
    std::string m_modelTexture;              // Model texture)
    Color m_color;                           // Color of model;
    CollisionComponent m_collisionComponent; // Collision bounds for this model instance
  public:
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, Color color,
                  const std::string &modelTexture, const Texture2D &texture);
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, Color color);
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name);

  public:
    // Take properties of model
    [[nodiscard]] std::string GetModelName() const;
    [[nodiscard]] Color GetColor() const;
    [[nodiscard]] float GetScale() const;
    [[nodiscard]] Model *GetModel() const;
    [[nodiscard]] Vector3 GetModelPosition() const;
    [[nodiscard]] Texture2D GetTexture() const;
    [[nodiscard]] std::string GetTexturePath() const;

    // Collision-related methods
    [[nodiscard]] CollisionComponent &GetCollisionComponent();
    [[nodiscard]] const CollisionComponent &GetCollisionComponent() const;
    void UpdateCollisionBounds();
    void SetCollisionType(CollisionComponent::CollisionType type);
    void SetCollisionRadius(float radius);
    void SetCollisionSize(Vector3 size);
};

#endif // MODELINSTANCE_H