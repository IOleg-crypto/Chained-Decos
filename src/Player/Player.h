// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/CameraController.h>
#include <Collision/CollisionSystem.h>
#include <Model/Model.h>
#include <Player/PositionData.h>
#include <World/PhysicsData.h>

// # ----------------------------------------------------------------------------
// # Player class handles the camera used to represent the player's point of view
// # ----------------------------------------------------------------------------
class Player
{
  private:
    float m_walkSpeed = 3.0f; // Speed for character
    float m_runSpeed = 15.0f;
    float m_jumpStrength = 8.0f; // Adjust as needed
    float m_jumpOffsetY = 0.0f;
    float m_baseCameraY = 4.5f;
    Vector3 m_baseTarget = {0, 2.0f, 0};
    Vector3 m_originalCameraTarget = {0, 2.0f, 0}; // Store original target for restoration
    bool m_isJumping = false;                      // Track jump state
    float m_cameraSmoothingFactor = 4.0f;          // Camera smoothing speed
    bool m_isPlayerMoving;                         // Track player moving
    PositionData m_posData;
    PhysicsData m_physData;
    Vector3 m_playerPosition;
    Vector3 m_playerSize;
    BoundingBox m_playerBoundingBox;
    Color m_playerColor;
    Model* m_playerModel;
    bool m_useModel;
    float m_cameraYaw = 0.0f;
float m_cameraPitch = 0.0f;

  private:
    Models m_modelPlayer;
    std::shared_ptr<CameraController> m_cameraController;

  public:
    Player(); // Constructor to initialize the camera and all stuff
    ~Player();
    Player(const Player &other) = delete;
    Player(Player &&other) = delete;

  public:
    void Update(); // Updates the camera each frame (e.g., handles input and movement)
    [[nodiscard]] float GetSpeed(); // Get character speed
    void SetSpeed(float speed);
    // Move player (camera) in 3D
    void Move(const Vector3& moveVector);
    // Allow player jumps
    void Jump();
    // Take history of player position(needed for player jump)
    void UpdatePositionHistory();
    // Allows W,A,S,D - movement
    void ApplyInput();
    // Get player rotation
    Matrix GetPlayerRotation();
    // Get camera
    [[nodiscard]] std::shared_ptr<CameraController> GetCameraController() const;
    // Get model manager
    Models GetModelManager();
    // Get player position data
    PositionData GetPlayerData() const;
    // Camera connects with jump
    void ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget, float jumpOffsetY);
    void UpdatePlayerBox();
    void DrawPlayer();
    void SetPlayerModel(Model* model);
    void ToggleModelRendering(bool useModel);
    Vector3 GetPlayerPosition() const { return m_playerPosition; }
    BoundingBox GetPlayerBoundingBox() const { return m_playerBoundingBox; }
    void UpdateCameraRotation(); // Update camera rotation based on mouse input
};

#endif // PLAYER_H
