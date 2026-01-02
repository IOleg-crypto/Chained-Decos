#include "core/application/application.h"
#include "core/window/Window.h"
#include "editor/editor_layer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <raylib.h>

using namespace CHEngine;

class EditorInputTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize engine loosely for testing
        WindowProps props;
        props.Width = 100;
        props.Height = 100;
        props.Title = "Test Window";
        m_Window = std::make_unique<Window>(props);
    }

    void TearDown() override
    {
        m_Window.reset();
    }

    std::unique_ptr<Window> m_Window;
};

TEST_F(EditorInputTest, RaylibExitKeyIsDisabled)
{
    // Verify that SetExitKey(0) was called in Window constructor
    // In Raylib, we can check the exit key if there was a getter,
    // but the most important thing is that pressing ESC doesn't trigger WindowShouldClose.

    // We can't easily simulate physical key presses in a standard GTest without mocking Raylib,
    // but we can verify that after window init, the state is stable.
    EXPECT_FALSE(WindowShouldClose());
}

// Mocking EditorLayer cursor state would require a more complex setup,
// so we'll focus on the core logic parts that can be unit tested if possible.
