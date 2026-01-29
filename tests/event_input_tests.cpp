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

// Note: Input status tests removed as Input is now a direct wrapper for Raylib
// and doesn't maintain internal state that can be trivially mocked in unit tests
// without Raylib context.
