function OnInit(self)
    LogInfo("Death Trigger Initialized")
end

function OnUpdate(self, dt)
    if IsColliding(self) then
        LogInfo("Player hit death zone!")
        RespawnPlayer()
    end
end
