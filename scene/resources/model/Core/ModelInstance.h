//
// Created by I#Oleg
//

#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include "../Animation/Animation.h"

#include <raylib.h>
#include <string>

//
// ModelInstance
// Represents a single instance of a 3D model in the game world.
// Stores:
//   - Position
//   - Scale
//   - Model reference
//   - Optional texture and color
// Used for rendering and collision purposes.
//
class ModelInstance
{
public:
    // ----------------------------------------------------
    // Constructors
    // ----------------------------------------------------

    // Full constructor with texture
    // pos          - Position of the instance in world space
    // pMdl         - Pointer to the base Model
    // scl          - Scale factor
    // name         - Model name
    // color        - Model color
    // modelTexture - Path to texture file
    // texture      - Texture2D object
    // animation    - Animation data
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, Color color,
                  const std::string &modelTexture, const Texture2D &texture);

    // Constructor without texture path
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, Color color);

    // Minimal constructor (only position, model, scale, and name)
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name);
    // Model have all
    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, const Color color,
                  const std::string &modelTexture, const Texture2D &texture,
                  const Animation &animation);

    ModelInstance(Vector3 pos, Model *pMdl, float scl, const std::string &name, const Color color,
                  const Animation &animation);

    // ----------------------------------------------------
    // Getters
    // ----------------------------------------------------

    // Get the model's registered name
    [[nodiscard]] std::string GetModelName() const;

    // Get the model's color
    [[nodiscard]] Color GetColor() const;

    // Get the model's scale factor
    [[nodiscard]] float GetScale() const;

    // Get a pointer to the base Model object
    [[nodiscard]] Model *GetModel() const;

    // Get the instance's position in the world
    [[nodiscard]] Vector3 GetModelPosition() const;

    // Get rotation in degrees (Euler XYZ)
    [[nodiscard]] Vector3 GetRotationDegrees() const;

    // Get the model's texture (if assigned)
    [[nodiscard]] Texture2D GetTexture() const;

    // Get the path to the model's texture file (if available)
    [[nodiscard]] std::string GetTexturePath() const;

    [[nodiscard]] Animation GetAnimation() const;

    // Set rotation in degrees
    void SetRotationDegrees(const Vector3 &rotationDeg);

private:
    Vector3 m_position;         // Position in world space (x, y, z)
    Model *m_model;             // Pointer to the base model
    float m_scale;              // Model scale factor
    std::string m_modelName;    // Name identifier of the model
    Texture2D m_texture;        // Texture object
    std::string m_modelTexture; // Path to texture file
    Color m_color;              // Model color
    Animation m_animation;      // Animation data for the model instance
    Vector3 m_rotationDeg = {0.0f, 0.0f, 0.0f}; // Rotation (degrees)
};

#endif // MODELINSTANCE_H
