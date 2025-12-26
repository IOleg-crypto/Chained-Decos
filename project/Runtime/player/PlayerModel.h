
#ifndef PLAYER_MODEL_H
#define PLAYER_MODEL_H

#include <raylib.h>
#include <scene/resources/model/Model.h>


// PlayerModel: handles the 3D model of the player
class PlayerModel
{
public:
    PlayerModel();
    ~PlayerModel();

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

    // Create a simple fallback model using basic shapes
    void CreateFallbackModel();

    // Render the player model (handles both GLB and fallback models)
    void Render(const Vector3 &position, float rotationY, const Color &color) const;

private:
    Model *m_model = nullptr;
    bool m_useModel = false;
    ModelLoader m_modelManager;
    float m_rotationModelY;

    // Fallback model data
    Model m_fallbackModel = {};
    bool m_usingFallback = false;
    Mesh m_fallbackMeshes[3] = {}; // Body, head, limbs
};

#endif // PLAYER_MODEL_H
