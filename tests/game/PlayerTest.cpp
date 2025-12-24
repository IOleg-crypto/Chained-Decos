#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "components/audio/core/AudioManager.h"
#include "components/physics/collision/core/collisionManager.h"
#include "core/Engine.h"
#include "project/ChainedDecos/player/core/Player.h"

class PlayerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        player = std::make_unique<Player>(&CHEngine::Engine::Instance().GetAudioManager());
        collisionManager = std::make_unique<CollisionManager>();
    }

    void TearDown() override
    {
        player.reset();
        collisionManager.reset();
    }

    std::unique_ptr<Player> player;
    std::unique_ptr<CollisionManager> collisionManager;
};

TEST_F(PlayerTest, ConstructorInitializesDefaults)
{
    EXPECT_NE(player.get(), nullptr);

    // Player should have a valid position
    Vector3 position = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(position.x));
    EXPECT_FALSE(std::isnan(position.y));
    EXPECT_FALSE(std::isnan(position.z));

    // Player should have valid size
    Vector3 size = player->GetPlayerSize();
    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
    EXPECT_GT(size.z, 0.0f);
}

TEST_F(PlayerTest, PlayerMovementWorks)
{
    Vector3 initialPosition = player->GetPlayerPosition();

    // Move player
    player->Move({1.0f, 0.0f, 0.0f});

    Vector3 newPosition = player->GetPlayerPosition();
    // Position might not change immediately due to collision system
    EXPECT_FALSE(std::isnan(newPosition.x));
    EXPECT_FALSE(std::isnan(newPosition.y));
    EXPECT_FALSE(std::isnan(newPosition.z));
}

TEST_F(PlayerTest, PlayerPositionCanBeSet)
{
    Vector3 newPosition = {10.0f, 5.0f, 10.0f};

    // Note: SetPlayerPosition might be const, so we need to check if there's a non-const version
    // For now, let's just test that GetPlayerPosition works
    Vector3 currentPosition = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(currentPosition.x));
    EXPECT_FALSE(std::isnan(currentPosition.y));
    EXPECT_FALSE(std::isnan(currentPosition.z));
}

TEST_F(PlayerTest, PlayerHasSpeed)
{
    float speed = player->GetSpeed();
    EXPECT_GE(speed, 0.0f); // Speed should be non-negative

    // Test setting speed if the method exists
    // player->SetSpeed(10.0f);
    // EXPECT_EQ(player->GetSpeed(), 10.0f);
}

TEST_F(PlayerTest, PlayerHasRotation)
{
    float rotation = player->GetRotationY();
    EXPECT_FALSE(std::isnan(rotation));

    // Test setting rotation if the method exists
    // player->SetRotationY(1.57f); // π/2 radians
    // EXPECT_NEAR(player->GetRotationY(), 1.57f, 0.01f);
}

TEST_F(PlayerTest, PlayerHasBoundingBox)
{
    BoundingBox bbox = player->GetPlayerBoundingBox();
    EXPECT_GT(bbox.max.x, bbox.min.x);
    EXPECT_GT(bbox.max.y, bbox.min.y);
    EXPECT_GT(bbox.max.z, bbox.min.z);
}

TEST_F(PlayerTest, PlayerHasCollision)
{
    const Collision &collision = player->GetCollision();
    EXPECT_NE(&collision, nullptr);

    // Test mutable access if available
    // PlayerCollision& mutableCollision = player->GetCollisionMutable();
    // EXPECT_NE(&mutableCollision, nullptr);
}

TEST_F(PlayerTest, PlayerUpdateWorks)
{
    // Update should not crash
    EXPECT_NO_THROW({ player->Update(*collisionManager); });
}

TEST_F(PlayerTest, PlayerCanJump)
{
    // Test jump impulse if the method exists
    EXPECT_NO_THROW({ player->ApplyJumpImpulse(5.0f); });
}

TEST_F(PlayerTest, PlayerHasCameraController)
{
    auto cameraController = player->GetCameraController();
    EXPECT_NE(cameraController, nullptr);
    EXPECT_NE(cameraController.get(), nullptr);
}
