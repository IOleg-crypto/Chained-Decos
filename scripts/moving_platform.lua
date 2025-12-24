function OnInit(self)
    -- Store initial position
    self.startPos = GetPosition(self)
    LogInfo("Moving Platform Initialized")
end

function OnUpdate(self, dt)
    -- Simple sine wave movement
    local time = GetTime()
    local offset = math.sin(time) * 5.0
    -- Move along X axis for now (can be customized later via properties if we add property support)
    SetPosition(self, self.startPos.x + offset, self.startPos.y, self.startPos.z)
end
