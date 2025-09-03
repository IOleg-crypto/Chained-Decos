
#ifndef PLAYER_MODEL_H
#define PLAYER_MODEL_H

#include <Model/Model.h>
#include <raylib.h>

// PlayerModel: handles the 3D model of the player
class PlayerModel
{
public:
    PlayerModel();
    ~PlayerModel() = default;

    // Set the 3D model
    void SetModel(Model *model);

    // Toggle model rendering
    void ToggleModelRendering(bool useModel);

    // Get model manager
    ModelLoader &GetModelManager();

    // Check if model is being used
    bool IsModelUsed() const;

    // Get the model pointer
    Model *GetModel() const;

private:
    Model *m_model = nullptr;
    bool m_useModel = false;
    ModelLoader m_modelManager;
    float m_rotationModelY;
};

#endif // PLAYER_MODEL_H
