#include "engine/core/base.h"
#include "gtest/gtest.h"
#include "editor/panels/project_browser_panel.h"
#include "editor/editor_panels.h"
#include "editor/editor_layer.h"
#include "engine/core/events.h"
#include <memory>

using namespace CHEngine;

class EditorIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#if defined(CH_CI) && defined(CH_PLATFORM_WINDOWS)
        GTEST_SKIP() << "Skipping UI integration tests on Windows CI due to lack of OpenGL support.";
#endif
    }
};

TEST_F(EditorIntegrationTest, ProjectBrowserDialogState)
{
    ProjectBrowserPanel panel;
    
    // Initial state
    EXPECT_FALSE(panel.IsCreateDialogVisible());
    EXPECT_FALSE(panel.HasPendingCreatePopupRequest());
    
    // Simulate interaction via exposed state
    panel.SetCreateDialogVisible(true);
    EXPECT_TRUE(panel.IsCreateDialogVisible());
    
    panel.SetCreateDialogVisible(false);
    EXPECT_FALSE(panel.IsCreateDialogVisible());
}

TEST_F(EditorIntegrationTest, PanelVisibilityToggle)
{
    ProjectBrowserPanel panel;
    EXPECT_TRUE(panel.IsOpen());
    
    panel.IsOpen() = false;
    EXPECT_FALSE(panel.IsOpen());
    
    panel.IsOpen() = true;
    EXPECT_TRUE(panel.IsOpen());
}

TEST_F(EditorIntegrationTest, GlobalPanelConsistency)
{
    EditorPanels panels;
    panels.Init();
    
    auto& panelList = panels.GetPanels();
    EXPECT_GT(panelList.size(), 0);
    
    for (auto& panel : panelList)
    {
        // Test opening/closing for EVERY panel registered in the engine
        panel->IsOpen() = false;
        EXPECT_FALSE(panel->IsOpen()) << "Failed to close panel: " << panel->GetName();
        
        panel->IsOpen() = true;
        EXPECT_TRUE(panel->IsOpen()) << "Failed to open panel: " << panel->GetName();
        
        // Ensure name exists
        EXPECT_FALSE(panel->GetName().empty());
    }
}
