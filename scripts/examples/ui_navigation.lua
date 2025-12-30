-- Example: ui_navigation.lua
-- This script demonstrates how to create a basic main menu flow

LogInfo("Initializing UI Navigation Script...")

-- 1. Helper function for common logic
local function startLevel(levelName)
    LogInfo("Preparing to load level: " .. levelName)
    -- You can add save logic or cleanup here
    LoadScene("maps/" .. levelName .. ".json")
end

-- 2. "Play" Button Callback
-- Searches for a UI element named "PlayButton" in the current scene
OnButtonClick("PlayButton", function()
    LogInfo("Play button clicked!")
    startLevel("world_01")
end)

-- 3. "Options" Button Callback
OnButtonClick("OptionsButton", function()
    LogInfo("Opening options menu...")
    LoadScene("ui/options_menu.json")
end)

-- 4. "Credits" Button Callback
OnButtonClick("CreditsButton", function()
    LogInfo("Showing credits...")
    LoadScene("ui/credits.json")
end)

-- 5. "Exit" Button Callback
OnButtonClick("ExitButton", function()
    LogInfo("Exit button clicked. Farewell!")
    QuitGame()
end)

-- You can also run temporary logic on script load
LogInfo("Navigation setup complete. Use OnButtonClick to register logic.")
