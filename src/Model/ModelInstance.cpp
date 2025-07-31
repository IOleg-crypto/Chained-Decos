//
// Created by I#Oleg
//

#include "ModelInstance.h"

ModelInstance::ModelInstance(const Vector3 pos, Model *pMdl, const float scl, const std::string &name, const Color color, const std::string &modelTexture,
    const Texture2D &texture) : position(pos), pModel(pMdl), scale(scl), modelName(name), texture(texture),
                                modelTexture(modelTexture), color(color) {
}

// Default init
ModelInstance::ModelInstance(const Vector3 pos , Model *pMdl , const float scl , const std::string &name , const Color color)
    : position(pos), pModel(pMdl), scale(scl), modelName(name), texture(), color(color) {
    // Nothing
}

// If color don`t exist - in json
ModelInstance::ModelInstance(const Vector3 pos , Model *pMdl , const float scl , const std::string &name)
    : position(pos), pModel(pMdl), scale(scl), modelName(name), texture(), color(WHITE) {
    // Nothing
}

std::string ModelInstance::getModelName() const {
    return modelName;
}

Color ModelInstance::getColor() const {
    return color;
}

float ModelInstance::getScale() const {
    return scale;
}

Model * ModelInstance::getModel() const {
    return pModel;
}

Vector3 ModelInstance::getModelPosition() const {
    return position;
}

Texture2D ModelInstance::getTexture() const {
    return this->texture;
}

std::string ModelInstance::getTexturePath() const {
    return modelTexture;
}