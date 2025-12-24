-- Example Usage: Attach to a trigger volume and set Script Path
-- Note: You can hardcode the level name here or we can add property support later.

local targetLevel = "Level1" -- Change this to the level you want to load

function OnInit(self)
    LogInfo("Load Level Trigger Initialized for: " .. targetLevel)
end

function OnUpdate(self, dt)
    if IsColliding(self) then
        LogInfo("Player entered Load Level Trigger! Loading: " .. targetLevel)
        LoadScene(targetLevel)
    end
end
