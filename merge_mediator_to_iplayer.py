import re

def update_iplayer_h():
    """Merge IPlayerMediator methods into IPlayer"""
    file_path = r'core\interfaces\IPlayer.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Add necessary includes
    includes = """#include <memory>
#include <raymath.h>
#include <scene/3d/camera/Core/CameraController.h>
#include <servers/physics/collision/System/CollisionSystem.h>
#include <servers/physics/dynamics/Components/PhysicsComponent.h>
"""
    
    if '#include <memory>' not in content:
        content = content.replace('#include <raylib.h>', '#include <raylib.h>\n' + includes)
    
    # Add IPlayerMediator methods before the closing brace
    mediator_methods = """
    // Position & Size (from IPlayerMediator)
    virtual Vector3 GetPlayerPosition() const = 0;
    virtual Vector3 GetPlayerSize() const = 0;
    virtual void SetPlayerPosition(const Vector3 &pos) const = 0;

    // Physics
    virtual LegacyPhysicsComponent &GetPhysics() = 0;
    virtual const LegacyPhysicsComponent &GetPhysics() const = 0;

    // Movement
    virtual void ApplyJumpImpulse(float impulse) = 0;

    // Collision
    virtual const Collision &GetCollision() const = 0;
    virtual void SyncCollision() const = 0;
    virtual void InitializeCollision() = 0;

    // Camera (detailed)
    virtual std::shared_ptr<CameraController> GetCameraController() const = 0;
"""
    
    if 'GetPlayerPosition' not in content:
        # Insert before closing brace
        content = content.replace('};', mediator_methods + '\n};')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated IPlayer.h")

def update_player_h():
    """Update Player to inherit only from IPlayer"""
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Remove IPlayerMediator include
    content = content.replace('#include "../Interfaces/IPlayerMediator.h"', '')
    
    # Change inheritance
    content = content.replace('class Player : public IPlayer, public IPlayerMediator', 'class Player : public IPlayer')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.h")

def update_player_collision_h():
    """Update PlayerCollision to use IPlayer instead of IPlayerMediator"""
    file_path = r'project\chaineddecos\Player\Collision\PlayerCollision.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('#include "../Interfaces/IPlayerMediator.h"', '#include "core/interfaces/IPlayer.h"')
    content = content.replace('IPlayerMediator* m_player', 'IPlayer* m_player')
    content = content.replace('explicit PlayerCollision(IPlayerMediator* player)', 'explicit PlayerCollision(IPlayer* player)')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated PlayerCollision.h")

def update_map_collision_initializer_h():
    """Update MapCollisionInitializer to use IPlayer"""
    file_path = r'project\chaineddecos\Managers\MapCollisionInitializer.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('#include "../Player/Interfaces/IPlayerMediator.h"', '#include "core/interfaces/IPlayer.h"')
    content = content.replace('IPlayerMediator* player', 'IPlayer* player')
    content = content.replace('IPlayerMediator* m_player', 'IPlayer* m_player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated MapCollisionInitializer.h")

def update_map_collision_initializer_cpp():
    """Update MapCollisionInitializer.cpp"""
    file_path = r'project\chaineddecos\Managers\MapCollisionInitializer.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('IPlayerMediator *player', 'IPlayer *player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated MapCollisionInitializer.cpp")

# Run all updates
update_iplayer_h()
update_player_h()
update_player_collision_h()
update_map_collision_initializer_h()
update_map_collision_initializer_cpp()

print("\nAll files updated. IPlayerMediator merged into IPlayer.")
