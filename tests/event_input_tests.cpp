#include "engine/core/events.h"
#include "engine/core/input.h"
#include "gtest/gtest.h"

using namespace CHEngine;

// ============================================================================
// Event System Tests
// ============================================================================

TEST(EventSystemTest, KeyPressedEventCreation)
{
    KeyPressedEvent event(KEY_W, false);
    EXPECT_EQ(event.GetKeyCode(), KEY_W);
    EXPECT_FALSE(event.IsRepeat());
    EXPECT_EQ(event.GetEventType(), EventType::KeyPressed);
}

TEST(EventSystemTest, KeyReleasedEventCreation)
{
    KeyReleasedEvent event(KEY_SPACE);
    EXPECT_EQ(event.GetKeyCode(), KEY_SPACE);
    EXPECT_EQ(event.GetEventType(), EventType::KeyReleased);
}

TEST(EventSystemTest, MouseButtonEventCreation)
{
    MouseButtonPressedEvent pressEvent(MOUSE_BUTTON_LEFT);
    EXPECT_EQ(pressEvent.GetMouseButton(), MOUSE_BUTTON_LEFT);
    EXPECT_EQ(pressEvent.GetAction(), MouseButtonEvent::Action::Pressed);

    MouseButtonReleasedEvent releaseEvent(MOUSE_BUTTON_RIGHT);
    EXPECT_EQ(releaseEvent.GetMouseButton(), MOUSE_BUTTON_RIGHT);
    EXPECT_EQ(releaseEvent.GetAction(), MouseButtonEvent::Action::Released);
}

TEST(EventSystemTest, EventDispatcherKeyPressed)
{
    KeyPressedEvent event(KEY_W, false);
    bool handlerCalled = false;
    int receivedKey = -1;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(
        [&](KeyPressedEvent &e)
        {
            handlerCalled = true;
            receivedKey = e.GetKeyCode();
            return true;
        });

    EXPECT_TRUE(handlerCalled);
    EXPECT_EQ(receivedKey, KEY_W);
    EXPECT_TRUE(event.Handled);
}

TEST(EventSystemTest, EventDispatcherWrongType)
{
    KeyPressedEvent event(KEY_W, false);
    bool handlerCalled = false;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        [&](MouseButtonPressedEvent &e)
        {
            handlerCalled = true;
            return true;
        });

    EXPECT_FALSE(handlerCalled);
    EXPECT_FALSE(event.Handled);
}

TEST(EventSystemTest, EventDispatcherMultipleHandlers)
{
    KeyPressedEvent event(KEY_SPACE, false);
    int handlerCount = 0;

    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<KeyPressedEvent>(
        [&](KeyPressedEvent &e)
        {
            handlerCount++;
            return false; // Don't mark as handled
        });

    dispatcher.Dispatch<KeyPressedEvent>(
        [&](KeyPressedEvent &e)
        {
            handlerCount++;
            return true; // Mark as handled
        });

    EXPECT_EQ(handlerCount, 2);
    EXPECT_TRUE(event.Handled);
}

// ============================================================================
// Input System Tests
// ============================================================================

class InputSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset input state before each test
        Input::UpdateState();
    }
};

TEST_F(InputSystemTest, KeyPressedThisFrame)
{
    // Simulate key press
    {
        KeyPressedEvent e(KEY_W, false);
        Input::OnEvent(e);
    }
    EXPECT_TRUE(Input::IsKeyPressed(KEY_W));
    EXPECT_TRUE(Input::IsKeyDown(KEY_W));
    EXPECT_FALSE(Input::IsKeyReleased(KEY_W));
    EXPECT_FALSE(Input::IsKeyUp(KEY_W));
}

TEST_F(InputSystemTest, KeyReleasedThisFrame)
{
    // Simulate press then release
    {
        KeyPressedEvent e(KEY_W, false);
        Input::OnEvent(e);
    }
    Input::UpdateState(); // Clear per-frame flags
    {
        KeyReleasedEvent e(KEY_W);
        Input::OnEvent(e);
    }
    EXPECT_FALSE(Input::IsKeyPressed(KEY_W));
    EXPECT_FALSE(Input::IsKeyDown(KEY_W));
    EXPECT_TRUE(Input::IsKeyReleased(KEY_W));
    EXPECT_TRUE(Input::IsKeyUp(KEY_W));
}

TEST_F(InputSystemTest, KeyHeldAcrossFrames)
{
    // Frame 1: Press
    {
        KeyPressedEvent e(KEY_W, false);
        Input::OnEvent(e);
    }
    EXPECT_TRUE(Input::IsKeyPressed(KEY_W));
    EXPECT_TRUE(Input::IsKeyDown(KEY_W));

    // Frame 2: Still held
    Input::UpdateState();
    EXPECT_FALSE(Input::IsKeyPressed(KEY_W)); // No longer "just pressed"
    EXPECT_TRUE(Input::IsKeyDown(KEY_W));     // Still down
}

TEST_F(InputSystemTest, MouseButtonPressed)
{
    {
        MouseButtonPressedEvent e(MOUSE_BUTTON_LEFT);
        Input::OnEvent(e);
    }
    EXPECT_TRUE(Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(Input::IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
}

TEST_F(InputSystemTest, MouseButtonReleased)
{
    {
        MouseButtonPressedEvent e(MOUSE_BUTTON_LEFT);
        Input::OnEvent(e);
    }
    Input::UpdateState();
    {
        MouseButtonReleasedEvent e(MOUSE_BUTTON_LEFT);
        Input::OnEvent(e);
    }
    EXPECT_FALSE(Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(Input::IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
}

TEST_F(InputSystemTest, MultipleKeysSimultaneous)
{
    {
        KeyPressedEvent e(KEY_W, false);
        Input::OnEvent(e);
    }
    {
        KeyPressedEvent e(KEY_LEFT_SHIFT, false);
        Input::OnEvent(e);
    }
    EXPECT_TRUE(Input::IsKeyDown(KEY_W));
    EXPECT_TRUE(Input::IsKeyDown(KEY_LEFT_SHIFT));

    {
        KeyReleasedEvent e(KEY_W);
        Input::OnEvent(e);
    }
    EXPECT_FALSE(Input::IsKeyDown(KEY_W));
    EXPECT_TRUE(Input::IsKeyDown(KEY_LEFT_SHIFT)); // Still held
}

TEST_F(InputSystemTest, InvalidKeyCode)
{
    // Test boundary conditions
    EXPECT_FALSE(Input::IsKeyDown(-1));
    EXPECT_FALSE(Input::IsKeyDown(999));
    EXPECT_FALSE(Input::IsKeyPressed(-1));
    EXPECT_FALSE(Input::IsKeyPressed(999));
}

TEST_F(InputSystemTest, InvalidMouseButton)
{
    EXPECT_FALSE(Input::IsMouseButtonDown(-1));
    EXPECT_FALSE(Input::IsMouseButtonDown(10));
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(EventInputIntegrationTest, KeyPressGeneratesEvent)
{
    // Simulate the flow: Event dispatch -> Input state update
    Input::UpdateState();
    {
        KeyPressedEvent e(KEY_SPACE, false);
        Input::OnEvent(e);
    }
    KeyPressedEvent event(KEY_SPACE, false);
    bool jumpTriggered = false;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(
        [&](KeyPressedEvent &e)
        {
            if (e.GetKeyCode() == KEY_SPACE && Input::IsKeyPressed(KEY_SPACE))
            {
                jumpTriggered = true;
                return true;
            }
            return false;
        });

    EXPECT_TRUE(jumpTriggered);
}

TEST(EventInputIntegrationTest, MovementPolling)
{
    // Simulate WSAD movement (polling pattern)
    Input::UpdateState();
    {
        KeyPressedEvent e(KEY_W, false);
        Input::OnEvent(e);
    }
    {
        KeyPressedEvent e(KEY_D, false);
        Input::OnEvent(e);
    }
    // Movement should be detected via polling
    bool movingForward = Input::IsKeyDown(KEY_W);
    bool movingRight = Input::IsKeyDown(KEY_D);

    EXPECT_TRUE(movingForward);
    EXPECT_TRUE(movingRight);

    // Next frame
    Input::UpdateState();
    EXPECT_TRUE(Input::IsKeyDown(KEY_W));     // Still held
    EXPECT_FALSE(Input::IsKeyPressed(KEY_W)); // Not "just pressed"
}
