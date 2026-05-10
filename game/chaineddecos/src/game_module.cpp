#include "components/game_components.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/entity.h"
#include "engine/core/reflection.h"
#include "engine/core/base.h"
#include "engine/core/service_locator.h"
#include "IconsFontAwesome6.h"
#ifndef GAME_BUILD_DLL
#include "editor/ui_properties.h"
#endif
#include <entt/entt.hpp>

namespace CHEngine
{
    void RegisterGameScriptBindings();

    void RegisterGameComponents()
    {
        RegisterGameScriptBindings();

        // Registration for SpawnComponent
        ComponentMetadata spawnMetadata;
        spawnMetadata.Name = "SpawnZone";
        spawnMetadata.SerializationKey = "SpawnComponent";
        spawnMetadata.Icon = ICON_FA_LOCATION_DOT;
        spawnMetadata.Add = [](Entity e) { e.AddComponent<SpawnComponent>(); };
        spawnMetadata.Remove = [](Entity e) { if (e.HasComponent<SpawnComponent>()) e.RemoveComponent<SpawnComponent>(); };
        spawnMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<SpawnComponent>()) dst.AddOrReplaceComponent<SpawnComponent>(src.GetComponent<SpawnComponent>()); };
        
#ifndef GAME_BUILD_DLL
        spawnMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<SpawnComponent>())
            {
                auto& comp = e.GetComponent<SpawnComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        
        ComponentRegistry::Register(::entt::type_hash<SpawnComponent>::value(), spawnMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<SpawnComponent>(spawnMetadata.SerializationKey);

        // Registration for PlayerComponent
        ComponentMetadata playerMetadata;
        playerMetadata.Name = "Player";
        playerMetadata.SerializationKey = "PlayerComponent";
        playerMetadata.Icon = ICON_FA_USER;
        playerMetadata.Add = [](Entity e) { e.AddComponent<PlayerComponent>(); };
        playerMetadata.Remove = [](Entity e) { if (e.HasComponent<PlayerComponent>()) e.RemoveComponent<PlayerComponent>(); };
        playerMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<PlayerComponent>()) dst.AddOrReplaceComponent<PlayerComponent>(src.GetComponent<PlayerComponent>()); };
#ifndef GAME_BUILD_DLL
        playerMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<PlayerComponent>())
            {
                auto& comp = e.GetComponent<PlayerComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        ComponentRegistry::Register(::entt::type_hash<PlayerComponent>::value(), playerMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<PlayerComponent>(playerMetadata.SerializationKey);

        // SceneTransitionComponent
        ComponentMetadata transitionMetadata;
        transitionMetadata.Name = "SceneTransition";
        transitionMetadata.SerializationKey = "SceneTransitionComponent";
        transitionMetadata.Icon = ICON_FA_DOOR_OPEN;
        transitionMetadata.Add = [](Entity e) { e.AddComponent<SceneTransitionComponent>(); };
        transitionMetadata.Remove = [](Entity e) { if (e.HasComponent<SceneTransitionComponent>()) e.RemoveComponent<SceneTransitionComponent>(); };
        transitionMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<SceneTransitionComponent>()) dst.AddOrReplaceComponent<SceneTransitionComponent>(src.GetComponent<SceneTransitionComponent>()); };
#ifndef GAME_BUILD_DLL
        transitionMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<SceneTransitionComponent>())
            {
                auto& comp = e.GetComponent<SceneTransitionComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        ComponentRegistry::Register(::entt::type_hash<SceneTransitionComponent>::value(), transitionMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<SceneTransitionComponent>(transitionMetadata.SerializationKey);
        
        // RPGStatsComponent
        ComponentMetadata rpgMetadata;
        rpgMetadata.Name = "RPG Stats";
        rpgMetadata.SerializationKey = "RPGStatsComponent";
        rpgMetadata.Icon = ICON_FA_CHART_BAR;
        rpgMetadata.Add = [](Entity e) { e.AddComponent<RPGStatsComponent>(); };
        rpgMetadata.Remove = [](Entity e) { if (e.HasComponent<RPGStatsComponent>()) e.RemoveComponent<RPGStatsComponent>(); };
        rpgMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<RPGStatsComponent>()) dst.AddOrReplaceComponent<RPGStatsComponent>(src.GetComponent<RPGStatsComponent>()); };
#ifndef GAME_BUILD_DLL
        rpgMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<RPGStatsComponent>())
            {
                auto& comp = e.GetComponent<RPGStatsComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        ComponentRegistry::Register(::entt::type_hash<RPGStatsComponent>::value(), rpgMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<RPGStatsComponent>(rpgMetadata.SerializationKey);

        // Registration for SkillComponent
        ComponentMetadata skillMetadata;
        skillMetadata.Name = "Skill";
        skillMetadata.SerializationKey = "SkillComponent";
        skillMetadata.Icon = ICON_FA_BOLT;
        skillMetadata.Add = [](Entity e) { if (!e.HasComponent<SkillComponent>()) e.AddComponent<SkillComponent>(); };
        skillMetadata.Remove = [](Entity e) { if (e.HasComponent<SkillComponent>()) e.RemoveComponent<SkillComponent>(); };
        skillMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<SkillComponent>()) dst.AddOrReplaceComponent<SkillComponent>(src.GetComponent<SkillComponent>()); };
#ifndef GAME_BUILD_DLL
        skillMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<SkillComponent>())
            {
                auto& comp = e.GetComponent<SkillComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        ComponentRegistry::Register(::entt::type_hash<SkillComponent>::value(), skillMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<SkillComponent>(skillMetadata.SerializationKey);

        // Registration for InventoryComponent
        ComponentMetadata inventoryMetadata;
        inventoryMetadata.Name = "Inventory";
        inventoryMetadata.SerializationKey = "InventoryComponent";
        inventoryMetadata.Icon = ICON_FA_BOXES_STACKED;
        inventoryMetadata.Add = [](Entity e) { if (!e.HasComponent<InventoryComponent>()) e.AddComponent<InventoryComponent>(); };
        inventoryMetadata.Remove = [](Entity e) { if (e.HasComponent<InventoryComponent>()) e.RemoveComponent<InventoryComponent>(); };
        inventoryMetadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<InventoryComponent>()) dst.AddOrReplaceComponent<InventoryComponent>(src.GetComponent<InventoryComponent>()); };
#ifndef GAME_BUILD_DLL
        inventoryMetadata.DrawUI = [](Entity e) {
            if (e.HasComponent<InventoryComponent>())
            {
                auto& comp = e.GetComponent<InventoryComponent>();
                UIProperties ui;
                Properties props(ui);
                comp.Reflect(props);
            }
        };
#endif
        ComponentRegistry::Register(::entt::type_hash<InventoryComponent>::value(), inventoryMetadata);
        ServiceLocator::Get<ComponentSerializer>().Register<InventoryComponent>(inventoryMetadata.SerializationKey);
    }
}
