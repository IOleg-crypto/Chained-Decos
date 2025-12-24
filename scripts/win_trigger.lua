function OnInit(self)
    LogInfo("Win Trigger Initialized")
end

function OnUpdate(self, dt)
    if IsColliding(self) then
        LogInfo("Level Complete! You Win!")
        -- In a real game, you would load the next level or menu
        -- LoadScene("Menu")
        -- For now, just respawn to restart
        RespawnPlayer()
    end
end
