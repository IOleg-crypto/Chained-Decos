#include <Player/PlayerModel.h>

PlayerModel::PlayerModel() : m_model(nullptr), m_useModel(false)
{
}

void PlayerModel::SetModel(Model* model)
{
    m_model = model;
}

void PlayerModel::ToggleModelRendering(bool useModel)
{
    m_useModel = useModel;
}

Models& PlayerModel::GetModelManager()
{
    return m_modelManager;
}

bool PlayerModel::IsModelUsed() const
{
    return m_useModel;
}

Model* PlayerModel::GetModel() const
{
    return m_model;
}