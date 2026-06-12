#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/reflection/reflection.h"
#include <string>
#include "engine/assets/asset.h"

namespace Chained
{
struct SpawnComponent
{
    bool IsActive = true;
    AssetHandle TextureHandle = 0;
    glm::vec3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;
    glm::vec3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    std::string TexturePath;

    SpawnComponent() = default;
    SpawnComponent(const SpawnComponent&) = default;

    CH_REFLECT_BEGIN(SpawnComponent)
        props.Property("Active", IsActive);
        props.Property("ZoneSize", ZoneSize, PropertyMeta(0.1f, 100.0f, 0.1f));
        if (props.GetMode() != Chained::ReflectionMode::UI)
            props.Handle("TextureHandle", TextureHandle);
        if (props.File("TexturePath", TexturePath, "png,jpg,bmp,tga"))
        {
            TextureHandle = AssetHandle(0);
        }
        props.Property("RenderZone", RenderSpawnZoneInScene);
        props.Property("SpawnPoint", SpawnPoint);
    CH_REFLECT_END()
};

struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float JumpForce = 10.0f;
    float LookSensitivity = 0.9f;

    PlayerComponent() = default;

    CH_REFLECT_BEGIN(PlayerComponent)
        props.Header("Settings");
        props.Property("MovementSpeed", MovementSpeed, PropertyMeta(0.1f, 100.0f, 0.5f));
        props.Property("JumpForce", JumpForce, PropertyMeta(0.1f, 50.0f, 0.5f));
        props.Property("LookSensitivity", LookSensitivity, PropertyMeta(0.1f, 5.0f, 0.1f));
    CH_REFLECT_END()
};

enum class SkillBranch
{
    Strength,
    Magic,
    Economy,
    Defense
};

struct RPGStatsComponent
{
    int Level = 1;
    int Experience = 0;
    int ExperienceToNextLevel = 100;
    
    float Health = 100.0f;
    float MaxHealth = 100.0f;
    float Mana = 50.0f;
    float MaxMana = 50.0f;
    
    float Strength = 10.0f;
    float Intelligence = 10.0f;
    float Dexterity = 10.0f;
    
    int Gold = 0;
    int SkillPoints = 0;

    RPGStatsComponent() = default;

    CH_REFLECT_BEGIN(RPGStatsComponent)
        props.Property("Level", Level);
        props.Property("Experience", Experience);
        props.Property("Exp To Next", ExperienceToNextLevel);
        props.Property("Health", Health);
        props.Property("Max Health", MaxHealth);
        props.Property("Mana", Mana);
        props.Property("Max Mana", MaxMana);
        props.Property("Strength", Strength);
        props.Property("Intelligence", Intelligence);
        props.Property("Dexterity", Dexterity);
        props.Property("Gold", Gold);
        props.Property("Skill Points", SkillPoints);
    CH_REFLECT_END()
};

struct SkillComponent
{
    SkillBranch Branch = SkillBranch::Strength;
    std::string SkillName = "Basic Strike";
    int RequiredLevel = 1;
    bool IsUnlocked = false;
    float Cooldown = 0.0f;
    float MaxCooldown = 5.0f;

    SkillComponent() = default;

    CH_REFLECT_BEGIN(SkillComponent)
        static const char* branchNames[] = { "Strength", "Magic", "Economy", "Defense" };
        props.Enum("Branch", Branch, branchNames, 4);
        props.Property("Skill Name", SkillName);
        props.Property("Required Level", RequiredLevel);
        props.Property("Is Unlocked", IsUnlocked);
        props.Property("Cooldown", Cooldown);
        props.Property("Max Cooldown", MaxCooldown);
    CH_REFLECT_END()
};

struct InventoryComponent
{
    int SlotCount = 16;
    // For simplicity in C++, we just expose basic item count or weight
    float CurrentWeight = 0.0f;
    float MaxWeight = 100.0f;

    InventoryComponent() = default;

    CH_REFLECT_BEGIN(InventoryComponent)
        props.Property("Slots", SlotCount);
        props.Property("Current Weight", CurrentWeight);
        props.Property("Max Weight", MaxWeight);
    CH_REFLECT_END()
};

struct NetworkIdentity
{
    uint64_t NetworkID = 0;
    bool IsOwned = false;

    NetworkIdentity() = default;

    CH_REFLECT_BEGIN(NetworkIdentity)
        props.Property("Network ID", NetworkID);
        props.Property("Is Owned", IsOwned);
    CH_REFLECT_END()
};

} // namespace Chained

#endif // CH_GAME_COMPONENTS_H
