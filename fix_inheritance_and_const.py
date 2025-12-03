import re

def fix_iplayer_h():
    file_path = r'core\interfaces\IPlayer.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Make SetSpeed and SetRotationY const to match IPlayerMediator and Player implementation
    content = content.replace('virtual void SetSpeed(float speed) = 0;', 'virtual void SetSpeed(float speed) const = 0;')
    content = content.replace('virtual void SetRotationY(float rotation) = 0;', 'virtual void SetRotationY(float rotation) const = 0;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed IPlayer.h (added const)")

def fix_player_h():
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix inheritance if missing
    if 'public IPlayer,' not in content and 'public IPlayerMediator' in content:
        content = content.replace('class Player : public IPlayerMediator', 'class Player : public IPlayer, public IPlayerMediator')
        
    # Fix SetSpeed/SetRotationY signatures in Player.h to be const override
    # Previously I made them non-const override. Now reverting to const override.
    content = content.replace('void SetSpeed(float speed) override;', 'void SetSpeed(float speed) const override;')
    content = content.replace('void SetRotationY(float rotation) override;', 'void SetRotationY(float rotation) const override;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Player.h inheritance and signatures")

def fix_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Revert SetSpeed/SetRotationY to const
    content = content.replace('void Player::SetSpeed(const float speed)', 'void Player::SetSpeed(const float speed) const')
    content = content.replace('void Player::SetRotationY(float rotationY)', 'void Player::SetRotationY(float rotationY) const')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Player.cpp signatures")

fix_iplayer_h()
fix_player_h()
fix_player_cpp()
