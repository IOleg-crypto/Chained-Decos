#include "PlayerModel.h"

PlayerModel::PlayerModel() : m_rotationModelY(0) {
}

void PlayerModel::SetModel(Model *model) { m_model = model; }

void PlayerModel::ToggleModelRendering(const bool useModel) { m_useModel = useModel; }

ModelLoader &PlayerModel::GetModelManager() { return m_modelManager; }

bool PlayerModel::IsModelUsed() const { return m_useModel; }

Model *PlayerModel::GetModel() const { return m_model; }
