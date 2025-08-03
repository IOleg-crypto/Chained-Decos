//
// Created by I#Oleg
//

#include "ModelInstance.h"

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl, const std::string &name, const Color color, const std::string &modelTexture,
    const Texture2D &texture) : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(texture),
                                m_modelTexture(modelTexture), m_color(color) {
}

// Default init
ModelInstance::ModelInstance(const Vector3 pos , Model *pMdl , const float scl , const std::string &name , const Color color)
    : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(color) {
    // Nothing
}

// If color don`t exist - in json
ModelInstance::ModelInstance(const Vector3 pos , Model *pMdl , const float scl , const std::string &name)
    : m_position(pos), m_pModel(pMdl), m_scale(scl), m_modelName(name), m_texture(), m_color(WHITE) {
    // Nothing
}

std::string ModelInstance::GetModelName() const {
    return m_modelName;
}

Color ModelInstance::GetColor() const {
    return m_color;
}

float ModelInstance::GetScale() const {
    return m_scale;
}

Model * ModelInstance::GetModel() const {
    return m_pModel;
}

Vector3 ModelInstance::GetModelPosition() const {
    return m_position;
}

Texture2D ModelInstance::GetTexture() const {
    return this->m_texture;
}

std::string ModelInstance::GetTexturePath() const {
    return m_modelTexture;
}