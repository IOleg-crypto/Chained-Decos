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
};

#endif // PLAYER_COMPONENT_H
