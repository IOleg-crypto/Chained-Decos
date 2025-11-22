//
// Created by I#Oleg
//

#include "ModelInstance.h"

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color,
                             const std::string &modelTexture, const Texture2D &texture)
    : m_position(pos), m_model(pMdl), m_scale(scl), m_modelName(name), m_texture(texture),
      m_modelTexture(modelTexture), m_color(color)
{
}

// Default init
ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color)
    : m_position(pos), m_model(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(color)
{
}

// If color don`t exist - in json
ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name)
    : m_position(pos), m_model(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(WHITE)
{
}

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color,
                             const std::string &modelTexture, const Texture2D &texture,
                             const Animation &animation)
    : m_position(pos), m_model(pMdl), m_scale(scl), m_modelName(name), m_texture(texture),
      m_modelTexture(modelTexture), m_color(color), m_animation(animation)
{
}

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl,
                             const std::string &name, const Color color, const Animation &animation)
    : m_position(pos), m_model(pMdl), m_scale(scl), m_modelName(name), m_color(color),
      m_animation(animation)
{
}

std::string ModelInstance::GetModelName() const { return m_modelName; }

Color ModelInstance::GetColor() const { return m_color; }

float ModelInstance::GetScale() const { return m_scale; }

Model *ModelInstance::GetModel() const { return m_model; }

Vector3 ModelInstance::GetModelPosition() const { return m_position; }

Texture2D ModelInstance::GetTexture() const { return this->m_texture; }

std::string ModelInstance::GetTexturePath() const { return m_modelTexture; }

Animation ModelInstance::GetAnimation() const { return m_animation; }

Vector3 ModelInstance::GetRotationDegrees() const { return m_rotationDeg; }

void ModelInstance::SetRotationDegrees(const Vector3 &rotationDeg)
{
    m_rotationDeg = rotationDeg;
}