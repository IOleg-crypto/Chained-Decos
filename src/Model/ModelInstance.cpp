//
// Created by I#Oleg
//

#include "ModelInstance.h"

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color,
                             const std::string &modelTexture, const Texture2D &texture)
    : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(texture),
      m_modelTexture(modelTexture), m_color(color)
{
    // Initialize collision component
    m_collisionComponent.type = CollisionComponent::AABB;
    m_collisionComponent.isStatic = true; // Models are typically static
    m_collisionComponent.isActive = true;
    UpdateCollisionBounds();
}

// Default init
ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color)
    : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(color)
{
    // Initialize collision component
    m_collisionComponent.type = CollisionComponent::AABB;
    m_collisionComponent.isStatic = true; // Models are typically static
    m_collisionComponent.isActive = true;
    UpdateCollisionBounds();
}

// If color don`t exist - in json
ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name)
    : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(WHITE)
{
    // Initialize collision component
    m_collisionComponent.type = CollisionComponent::AABB;
    m_collisionComponent.isStatic = true; // Models are typically static
    m_collisionComponent.isActive = true;
    UpdateCollisionBounds();
}

std::string ModelInstance::GetModelName() const { return m_modelName; }

Color ModelInstance::GetColor() const { return m_color; }

float ModelInstance::GetScale() const { return m_scale; }

Model *ModelInstance::GetModel() const { return m_pModel; }

Vector3 ModelInstance::GetModelPosition() const { return m_position; }

Texture2D ModelInstance::GetTexture() const { return this->m_texture; }

std::string ModelInstance::GetTexturePath() const { return m_modelTexture; }

CollisionComponent &ModelInstance::GetCollisionComponent() { return m_collisionComponent; }

const CollisionComponent &ModelInstance::GetCollisionComponent() const
{
    return m_collisionComponent;
}

void ModelInstance::UpdateCollisionBounds()
{
    if (m_pModel != nullptr)
    {
        // Get model bounding box from Raylib
        BoundingBox modelBounds = GetMeshBoundingBox(m_pModel->meshes[0]);

        // Scale the bounding box by the model's scale
        Vector3 size = {(modelBounds.max.x - modelBounds.min.x) * m_scale,
                        (modelBounds.max.y - modelBounds.min.y) * m_scale,
                        (modelBounds.max.z - modelBounds.min.z) * m_scale};

        // Update collision bounds based on type
        if (m_collisionComponent.type == CollisionComponent::SPHERE ||
            m_collisionComponent.type == CollisionComponent::BOTH)
        {
            m_collisionComponent.sphere.center = m_position;
            // Use the largest dimension as radius
            float maxDimension = fmaxf(fmaxf(size.x, size.y), size.z);
            m_collisionComponent.sphere.radius = maxDimension * 0.5f;
        }

        if (m_collisionComponent.type == CollisionComponent::AABB ||
            m_collisionComponent.type == CollisionComponent::BOTH)
        {
            Vector3 halfSize = Vector3Scale(size, 0.5f);
            m_collisionComponent.box.min = Vector3Subtract(m_position, halfSize);
            m_collisionComponent.box.max = Vector3Add(m_position, halfSize);
        }
    }
    else
    {
        // Fallback: use default size if model is not available
        Vector3 defaultSize = {2.0f * m_scale, 2.0f * m_scale, 2.0f * m_scale};

        if (m_collisionComponent.type == CollisionComponent::SPHERE ||
            m_collisionComponent.type == CollisionComponent::BOTH)
        {
            m_collisionComponent.sphere.center = m_position;
            m_collisionComponent.sphere.radius = m_scale;
        }

        if (m_collisionComponent.type == CollisionComponent::AABB ||
            m_collisionComponent.type == CollisionComponent::BOTH)
        {
            Vector3 halfSize = Vector3Scale(defaultSize, 0.5f);
            m_collisionComponent.box.min = Vector3Subtract(m_position, halfSize);
            m_collisionComponent.box.max = Vector3Add(m_position, halfSize);
        }
    }
}

void ModelInstance::SetCollisionType(CollisionComponent::CollisionType type)
{
    m_collisionComponent.type = type;
    UpdateCollisionBounds();
}

void ModelInstance::SetCollisionRadius(float radius)
{
    m_collisionComponent.sphere.radius = radius;
}

void ModelInstance::SetCollisionSize(Vector3 size)
{
    Vector3 halfSize = Vector3Scale(size, 0.5f);
    m_collisionComponent.box.min = Vector3Subtract(m_position, halfSize);
    m_collisionComponent.box.max = Vector3Add(m_position, halfSize);
}