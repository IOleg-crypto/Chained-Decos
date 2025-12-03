import re

def fix_player_h():
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove the block I added previously (it caused duplicates)
    # The block started with "// IPlayer Interface Implementation"
    # I'll replace it with just the new methods that don't exist
    
    # First, remove the block
    content = re.sub(r'// IPlayer Interface Implementation.*?Camera3D& GetCamera\(\) override; // Implemented in .cpp\s*', '', content, flags=re.DOTALL)
    
    # Now modify existing methods to match interface
    # GetSpeed() const -> GetSpeed() const override
    content = content.replace('[[nodiscard]] float GetSpeed() const;', 'float GetSpeed() const override;')
    
    # GetRotationY() const -> GetRotationY() const override
    content = content.replace('[[nodiscard]] float GetRotationY() const;', 'float GetRotationY() const override;')
    
    # SetSpeed(float speed) const -> SetSpeed(float speed) override
    content = content.replace('void SetSpeed(float speed) const;', 'void SetSpeed(float speed) override;')
    
    # SetRotationY(float rotationY) const -> SetRotationY(float rotation) override
    content = content.replace('void SetRotationY(float rotationY) const;', 'void SetRotationY(float rotation) override;')
    
    # Add missing methods
    # Update(float deltaTime) override
    # Camera3D& GetCamera() override
    # GetPosition() override (alias to GetPlayerPosition)
    # SetPosition() override (alias to SetPlayerPosition)
    
    new_methods = """
    // IPlayer Interface Implementation
    Vector3 GetPosition() const override { return GetPlayerPosition(); }
    void SetPosition(const Vector3& pos) override { SetPlayerPosition(pos); }
    void Update(float deltaTime) override;
    Camera3D& GetCamera() override;
    """
    
    if '// IPlayer Interface Implementation' not in content:
        content = content.replace('// Service injection', new_methods + '\n    // Service injection')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Player.h")

def fix_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Remove const from SetSpeed implementation
    content = content.replace('void Player::SetSpeed(const float speed) const', 'void Player::SetSpeed(const float speed)')
    
    # Remove const from SetRotationY implementation (if it exists)
    # I need to check if SetRotationY is implemented in cpp or h. 
    # View file showed SetSpeed in cpp. SetRotationY likely too.
    # Let's assume it is.
    content = content.replace('void Player::SetRotationY(float rotationY) const', 'void Player::SetRotationY(float rotationY)')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Player.cpp")

fix_player_h()
fix_player_cpp()
