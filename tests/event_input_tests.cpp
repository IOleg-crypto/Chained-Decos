#include "engine/core/events.h"
#include "engine/core/key_codes.h"
#include "engine/core/mouse_codes.h"
#include "gtest/gtest.h"

using namespace CHEngine;

TEST(EventSystemTest, KeyPressedEventCreation)
{
    KeyPressedEvent event(Key::W, false);
    EXPECT_EQ(event.GetKeyCode(), Key::W);
    EXPECT_FALSE(event.IsRepeat());
    EXPECT_EQ(event.GetEventType(), EventType::KeyPressed);
    EXPECT_TRUE(event.IsInCategory(EventCategoryKeyboard));
    EXPECT_TRUE(event.IsInCategory(EventCategoryInput));
}

TEST(EventSystemTest, KeyReleasedEventCreation)
{
    KeyReleasedEvent event(Key::Space);
    EXPECT_EQ(event.GetKeyCode(), Key::Space);
    EXPECT_EQ(event.GetEventType(), EventType::KeyReleased);
}

TEST(EventSystemTest, MouseButtonEventCreation)
{
    MouseButtonPressedEvent pressEvent(Mouse::ButtonLeft);
    EXPECT_EQ(pressEvent.GetMouseButton(), Mouse::ButtonLeft);
    EXPECT_EQ(pressEvent.GetAction(), MouseButtonEvent::Action::Pressed);
    EXPECT_TRUE(pressEvent.IsInCategory(EventCategoryMouseButton));
    EXPECT_TRUE(pressEvent.IsInCategory(EventCategoryInput));

    MouseButtonReleasedEvent releaseEvent(Mouse::ButtonRight);
    EXPECT_EQ(releaseEvent.GetMouseButton(), Mouse::ButtonRight);
    EXPECT_EQ(releaseEvent.GetAction(), MouseButtonEvent::Action::Released);
}

TEST(EventSystemTest, EventDispatcherKeyPressed)
{
    KeyPressedEvent event(Key::W, false);
    bool handlerCalled = false;
    int receivedKey = -1;

    EventDispatcher dispatcher(event);
    const bool dispatched = dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {
        handlerCalled = true;
        receivedKey = e.GetKeyCode();
        return true;
    });

    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(handlerCalled);
    EXPECT_EQ(receivedKey, Key::W);
    EXPECT_TRUE(event.Handled);
}

TEST(EventSystemTest, EventDispatcherWrongType)
{
    KeyPressedEvent event(Key::W, false);
    bool handlerCalled = false;

    EventDispatcher dispatcher(event);
    const bool dispatched = dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent&) {
        handlerCalled = true;
        return true;
    });

    EXPECT_FALSE(dispatched);
    EXPECT_FALSE(handlerCalled);
    EXPECT_FALSE(event.Handled);
}

TEST(EventSystemTest, EventDispatcherMultipleHandlers)
{
    KeyPressedEvent event(Key::Space, false);
    int handlerCount = 0;

    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {
        handlerCount++;
        return false; // Don't mark as handled
    });

    dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {
        handlerCount++;
        return true; // Mark as handled
    });

    EXPECT_EQ(handlerCount, 2);
    EXPECT_TRUE(event.Handled);
}

TEST(EventSystemTest, WindowResizeToStringIncludesDimensions)
{
    WindowResizeEvent event(1280, 720);

    EXPECT_EQ(event.GetWidth(), 1280u);
    EXPECT_EQ(event.GetHeight(), 720u);
    EXPECT_NE(event.ToString().find("1280"), std::string::npos);
    EXPECT_NE(event.ToString().find("720"), std::string::npos);
}
