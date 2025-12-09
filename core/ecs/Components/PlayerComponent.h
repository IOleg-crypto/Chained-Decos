#ifndef PLAYER_COMPONENT_H
#define PLAYER_COMPONENT_H

struct PlayerComponent
{
    float moveSpeed = 5.0f;
    float jumpForce = 10.0f;
    float mouseSensitivity = 0.1f;

    // Player state
    bool isGrounded = false;
    bool canDoubleJump = false;
    int jumpsRemaining = 2;
    bool isFallingSoundPlaying = false;
    float cameraPitch = 0.0f;
    float cameraDistance = 5.0f; // Zoom distance
    float cameraYaw = 0.0f;      // Independent camera rotation

    // UI/Stats
    float maxHeight = 0.0f;
    float runTimer = 0.0f;
};

#endif // PLAYER_COMPONENT_H
