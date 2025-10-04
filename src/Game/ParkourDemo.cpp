#include "Map/ParkourMapGenerator.h"
#include "raylib.h"
#include <memory>
#include <cmath>

// Simple Player Controller for Parkour Demo
class DemoPlayer
{
private:
    Vector3 position;
    Vector3 velocity;
    float yaw, pitch;
    bool isOnGround;
    bool canJump;
    float jumpCooldown;
    const float MOVE_SPEED = 8.0f;
    const float JUMP_FORCE = 12.0f;
    const float GRAVITY = -25.0f;
    const float MOUSE_SENSITIVITY = 0.003f;

public:
    DemoPlayer(Vector3 startPos) : position(startPos), velocity({0,0,0}), yaw(-3.14f), pitch(0), isOnGround(false), canJump(true), jumpCooldown(0)
    {}

    void Update(float deltaTime, const ParkourTestMap& map)
    {
        // Handle mouse look
        Vector2 mouseDelta = GetMouseDelta();
        yaw += mouseDelta.x * MOUSE_SENSITIVITY;
        pitch -= mouseDelta.y * MOUSE_SENSITIVITY;
        pitch = Clamp(pitch, -PI/2 + 0.1f, PI/2 - 0.1f);

        // Handle movement input
        Vector3 moveDir = {0, 0, 0};
        if (IsKeyDown(KEY_W)) moveDir.z -= 1;
        if (IsKeyDown(KEY_S)) moveDir.z += 1;
        if (IsKeyDown(KEY_A)) moveDir.x -= 1;
        if (IsKeyDown(KEY_D)) moveDir.x += 1;

        // Normalize and apply movement
        if (Vector3Length(moveDir) > 0)
        {
            moveDir = Vector3Normalize(moveDir);

            // Transform to camera space
            Vector3 forward = Vector3Normalize({sinf(yaw), 0, cosf(yaw)});
            Vector3 right = Vector3Normalize({cosf(yaw), 0, -sinf(yaw)});
            Vector3 finalMove = Vector3Add(Vector3Scale(forward, moveDir.z), Vector3Scale(right, moveDir.x));

            velocity.x = finalMove.x * MOVE_SPEED;
            velocity.z = finalMove.z * MOVE_SPEED;
        }
        else
        {
            velocity.x *= 0.8f; // Friction
            velocity.z *= 0.8f;
        }

        // Handle jumping
        jumpCooldown -= deltaTime;
        if (IsKeyPressed(KEY_SPACE) && isOnGround && canJump && jumpCooldown <= 0)
        {
            velocity.y = JUMP_FORCE;
            isOnGround = false;
            canJump = false;
            jumpCooldown = 0.2f; // Prevent jump spam
        }

        // Apply gravity
        velocity.y += GRAVITY * deltaTime;

        // Simple collision detection with parkour elements
        Vector3 newPosition = position;
        newPosition.x += velocity.x * deltaTime;
        newPosition.y += velocity.y * deltaTime;
        newPosition.z += velocity.z * deltaTime;

        // Check collision with platforms
        bool collision = false;
        for (const auto& element : map.elements)
        {
            if (element.isPlatform && CheckPlayerPlatformCollision(position, newPosition, element))
            {
                collision = true;
                break;
            }
        }

        if (!collision)
        {
            position = newPosition;
            isOnGround = false;
        }
        else
        {
            // Try to move horizontally only
            Vector3 horizontalMove = position;
            horizontalMove.x += velocity.x * deltaTime;
            horizontalMove.z += velocity.z * deltaTime;

            bool horizontalCollision = false;
            for (const auto& element : map.elements)
            {
                if (element.isPlatform && CheckPlayerPlatformCollision(position, horizontalMove, element))
                {
                    horizontalCollision = true;
                    break;
                }
            }

            if (!horizontalCollision)
            {
                position = horizontalMove;
            }

            velocity.x = 0;
            velocity.z = 0;

            // Check if we're on top of a platform
            if (velocity.y <= 0)
            {
                Vector3 groundCheck = {position.x, position.y - 0.1f, position.z};
                for (const auto& element : map.elements)
                {
                    if (element.isPlatform && CheckPointPlatformCollision(groundCheck, element))
                    {
                        position.y = element.position.y + element.size.y/2 + 1.0f;
                        velocity.y = 0;
                        isOnGround = true;
                        canJump = true;
                        break;
                    }
                }
            }
        }

        // Update camera
        Vector3 cameraPos = position;
        cameraPos.y += 1.8f; // Eye height

        Vector3 cameraTarget;
        cameraTarget.x = position.x + sinf(yaw);
        cameraTarget.y = position.y + 1.8f + sinf(pitch);
        cameraTarget.z = position.z + cosf(yaw);
    }

    Vector3 GetPosition() const { return position; }
    Vector3 GetCameraPosition() const { return {position.x, position.y + 1.8f, position.z}; }
    Vector3 GetCameraTarget() const
    {
        return {
            position.x + sinf(yaw),
            position.y + 1.8f + sinf(pitch),
            position.z + cosf(yaw)
        };
    }

private:
    bool CheckPlayerPlatformCollision(Vector3 oldPos, Vector3 newPos, const ParkourElement& platform)
    {
        // Simple AABB collision detection
        Vector3 playerSize = {0.5f, 1.8f, 0.5f}; // Player bounding box

        float playerMinX = newPos.x - playerSize.x/2;
        float playerMaxX = newPos.x + playerSize.x/2;
        float playerMinY = newPos.y - playerSize.y/2;
        float playerMaxY = newPos.y + playerSize.y/2;
        float playerMinZ = newPos.z - playerSize.z/2;
        float playerMaxZ = newPos.z + playerSize.z/2;

        float platformMinX = platform.position.x - platform.size.x/2;
        float platformMaxX = platform.position.x + platform.size.x/2;
        float platformMinY = platform.position.y - platform.size.y/2;
        float platformMaxY = platform.position.y + platform.size.y/2;
        float platformMinZ = platform.position.z - platform.size.z/2;
        float platformMaxZ = platform.position.z + platform.size.z/2;

        return !(playerMaxX <= platformMinX || playerMinX >= platformMaxX ||
                playerMaxY <= platformMinY || playerMinY >= platformMaxY ||
                playerMaxZ <= platformMinZ || playerMinZ >= platformMaxZ);
    }

    bool CheckPointPlatformCollision(Vector3 point, const ParkourElement& platform)
    {
        float platformMinX = platform.position.x - platform.size.x/2;
        float platformMaxX = platform.position.x + platform.size.x/2;
        float platformMinY = platform.position.y - platform.size.y/2;
        float platformMaxY = platform.position.y + platform.size.y/2;
        float platformMinZ = platform.position.z - platform.size.z/2;
        float platformMaxZ = platform.position.z + platform.size.z/2;

        return (point.x >= platformMinX && point.x <= platformMaxX &&
                point.y >= platformMinY && point.y <= platformMaxY &&
                point.z >= platformMinZ && point.z <= platformMaxZ);
    }
};

// Parkour Demo Application
class ParkourDemo
{
private:
    int screenWidth;
    int screenHeight;
    Camera3D camera;
    ParkourTestMap currentMap;
    int currentMapIndex;
    std::vector<ParkourTestMap> availableMaps;
    bool showMapSelection;
    DemoPlayer* player;

public:
    ParkourDemo(int width, int height) : screenWidth(width), screenHeight(height), currentMapIndex(0), showMapSelection(true)
    {
        // Load all available maps
        availableMaps = ParkourMapGenerator::GetAllParkourMaps();
        if (!availableMaps.empty())
        {
            currentMap = availableMaps[0];
            player = new DemoPlayer(currentMap.startPosition);
        }
        else
        {
            player = new DemoPlayer({0, 2, 0});
        }
    }

    void Init()
    {
        InitWindow(screenWidth, screenHeight, "Parkour Map Demo - Raylib Shapes");
        SetTargetFPS(60);
    }

    void Update()
    {
        float deltaTime = GetFrameTime();

        // Update player
        if (player)
        {
            player->Update(deltaTime, currentMap);
        }

        // Handle map selection
        if (IsKeyPressed(KEY_TAB))
        {
            showMapSelection = !showMapSelection;
        }

        if (showMapSelection && IsKeyPressed(KEY_ENTER) && !availableMaps.empty())
        {
            currentMapIndex = (currentMapIndex + 1) % availableMaps.size();
            currentMap = availableMaps[currentMapIndex];
            if (player)
            {
                delete player;
                player = new DemoPlayer(currentMap.startPosition);
            }
            showMapSelection = false;
        }

        if (showMapSelection && IsKeyPressed(KEY_BACKSPACE))
        {
            showMapSelection = false;
        }

        // Lock mouse cursor when playing
        if (!showMapSelection)
        {
            DisableCursor();
        }
        else
        {
            EnableCursor();
        }
    }

    void Render()
    {
        BeginDrawing();
        ClearBackground(currentMap.skyColor);

        // Update camera based on player position
        if (player)
        {
            camera.position = player->GetCameraPosition();
            camera.target = player->GetCameraTarget();
        }
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 75.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        BeginMode3D(camera);

        // Render the current parkour map
        ParkourMapGenerator::RenderParkourMap(currentMap, camera);

        // Draw start and end position indicators
        DrawSphere(currentMap.startPosition, 0.5f, GREEN);
        DrawSphere(currentMap.endPosition, 0.5f, YELLOW);

        // Draw player position indicator (for debugging)
        if (player)
        {
            Vector3 playerPos = player->GetPosition();
            DrawCube({playerPos.x, playerPos.y - 0.9f, playerPos.z}, 0.5f, 0.1f, 0.5f, RED);
        }

        EndMode3D();

        // Draw UI
        DrawText(TextFormat("Current Map: %s", currentMap.displayName.c_str()), 10, 10, 20, WHITE);
        DrawText(TextFormat("Difficulty: %.1f/5.0", currentMap.difficulty), 10, 40, 20, WHITE);
        DrawText(currentMap.description.c_str(), 10, 70, 15, LIGHTGRAY);

        // Controls help
        if (!showMapSelection)
        {
            DrawText("WASD: Move | SPACE: Jump | Mouse: Look | TAB: Map Selection", 10, screenHeight - 50, 15, GRAY);
        }

        // Map selection UI
        if (showMapSelection)
        {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("SELECT PARKOUR MAP", screenWidth/2 - 200, screenHeight/2 - 100, 30, WHITE);

            for (size_t i = 0; i < availableMaps.size(); i++)
            {
                Color color = (i == currentMapIndex) ? YELLOW : WHITE;
                DrawText(TextFormat("%zu. %s (%.1f)",
                       i + 1,
                       availableMaps[i].displayName.c_str(),
                       availableMaps[i].difficulty),
                       screenWidth/2 - 200, screenHeight/2 - 50 + (int)i * 30, 20, color);
            }

            DrawText("Press ENTER to select, TAB to close", screenWidth/2 - 150, screenHeight - 80, 20, GRAY);
        }
        else
        {
            DrawText("Press TAB for map selection", 10, screenHeight - 30, 15, GRAY);
        }

        DrawFPS(screenWidth - 80, 10);

        EndDrawing();
    }

    void Run()
    {
        while (!WindowShouldClose())
        {
            Update();
            Render();
        }
    }

    ~ParkourDemo()
    {
        if (player)
        {
            delete player;
        }
        CloseWindow();
    }
};

int main()
{
    ParkourDemo demo(1280, 720);
    demo.Init();
    demo.Run();
    return 0;
}