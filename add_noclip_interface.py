import re

def update_iplayer_h():
    file_path = r'core\interfaces\IPlayer.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'SetNoclip' not in content:
        # Add Noclip methods
        new_methods = """    virtual void SetNoclip(bool enabled) = 0;
    virtual bool IsNoclip() const = 0;
"""
        content = content.replace('virtual Camera3D &GetCamera() = 0;', 'virtual Camera3D &GetCamera() = 0;\n\n    // Debug/Cheat\n' + new_methods)
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated IPlayer.h")

def update_player_h():
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'SetNoclip' not in content:
        new_methods = """    void SetNoclip(bool enabled) override;
    bool IsNoclip() const override;
"""
        content = content.replace('Camera3D& GetCamera() override;', 'Camera3D& GetCamera() override;\n' + new_methods)
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.h")

def update_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'Player::SetNoclip' not in content:
        methods = """
void Player::SetNoclip(bool enabled)
{
    GetCollisionMutable().EnableBVHCollision(!enabled); // Noclip means NO collision, so EnableBVHCollision(false) if enabled?
    // Wait, existing code was: collision.EnableBVHCollision(!current);
    // If noclip is ENABLED, collision should be DISABLED?
    // "Noclip: enabled" -> collision disabled.
    // So SetNoclip(true) -> EnableBVHCollision(false)?
    // Or EnableBVHCollision means "use complex collision"?
    // Let's check ConsoleManager logic:
    // bool current = collision.IsUsingBVH();
    // collision.EnableBVHCollision(!current);
    // console->AddOutput("Noclip: " + std::string(!current ? "enabled" : "disabled"));
    // If !current is true (was false), noclip is enabled.
    // So if EnableBVHCollision(true), noclip is enabled?
    // That sounds backwards. BVH is Bounding Volume Hierarchy, usually for mesh collision.
    // Maybe "Noclip" here just means "Toggle Collision Mode"?
    // Let's assume SetNoclip(true) means "disable collision" or "enable ghost mode".
    // But the console command toggles BVH.
    // I'll implement SetNoclip to toggle BVH for now, matching console logic.
    // Actually, I'll just expose ToggleNoclip() or SetNoclip(bool).
    // Let's stick to SetNoclip(bool).
    // If enabled=true, we want noclip.
    // If existing logic says !current -> enabled, then EnableBVHCollision(!current) enables noclip?
    // That implies EnableBVHCollision(true) = Noclip Enabled.
    // That's weird. Usually BVH = Physics.
    // Maybe the log message is just "Noclip: [state]".
    // I will implement SetNoclip(bool enabled) and assume enabled=true means "No Collision".
    // But I need to know what EnableBVHCollision does.
    // I'll check PlayerCollision.h if possible, but for now I'll just map it to what ConsoleManager did.
    // ConsoleManager: collision.EnableBVHCollision(!current);
    // If I want to set specific state, I need to know what true/false means.
    // I'll implement ToggleNoclip() instead? No, interface should be explicit.
    // I'll check PlayerCollision.h quickly.
    
    // For now, I'll just add the method stub and check file in next step.
}

bool Player::IsNoclip() const
{
    return GetCollision().IsUsingBVH(); // Assuming this relates to noclip
}
"""
        # I'll write the file but I need to verify logic.
        # Let's pause writing cpp and check PlayerCollision.h first.
        pass

    # with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    #     f.write(content)
    # print("Updated Player.cpp")

update_iplayer_h()
update_player_h()
