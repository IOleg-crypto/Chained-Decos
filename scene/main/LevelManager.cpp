#include "LevelManager.h"
#include "MapCollisionInitializer.h"
#include "core/Log.h"
#include "core/assets/AssetManager.h"
#include "scene/core/Entity.h"
#include "scene/core/Scene.h"
#include "scene/core/SceneManager.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/PlayerComponent.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/ScriptingComponents.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UIComponents.h"
#include "scene/ecs/components/UtilityComponents.h"
#include "scene/resources/map/MapRenderer.h"
#include "scene/resources/map/MapService.h"
#include "scene/resources/map/SceneSerializer.h"
#include "scene/resources/model/ModelAnalyzer.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace CHEngine
{
static std::unique_ptr<LevelManager> s_Instance = nullptr;

void LevelManager::Init(const LevelManagerConfig &config)
{
    s_Instance = std::make_unique<LevelManager>(config);
}

void LevelManager::Shutdown()
{
    if (s_Instance)
        s_Instance->InternalShutdown();
    s_Instance.reset();
}

bool LevelManager::IsInitialized()
{
    return s_Instance != nullptr;
}

LevelManager &LevelManager::Get()
{
    return *s_Instance;
}

void LevelManager::SetActiveScene(std::shared_ptr<CHEngine::Scene> scene)
{
    Get().InternalSetActiveScene(scene);
}

void LevelManager::SetUIScene(std::shared_ptr<CHEngine::Scene> scene)
{
    Get().InternalSetUIScene(scene);
}

std::shared_ptr<CHEngine::Scene> LevelManager::GetUIScene()
{
    return Get().InternalGetUIScene();
}

bool LevelManager::LoadScene(const std::string &path)
{
    return s_Instance ? s_Instance->InternalLoadScene(path) : false;
}

bool LevelManager::LoadSceneByIndex(int index)
{
    return s_Instance ? s_Instance->InternalLoadSceneByIndex(index) : false;
}

bool LevelManager::LoadSceneByName(const std::string &name)
{
    return s_Instance ? s_Instance->InternalLoadSceneByName(name) : false;
}

bool LevelManager::LoadUIScene(const std::string &path)
{
    return s_Instance ? s_Instance->InternalLoadUIScene(path) : false;
}

void LevelManager::UnloadUIScene()
{
    if (s_Instance)
        s_Instance->InternalUnloadUIScene();
}

bool LevelManager::IsMapLoaded()
{
    return s_Instance ? s_Instance->InternalIsMapLoaded() : false;
}

const std::string &LevelManager::GetCurrentMapPath()
{
    static std::string empty = "";
    return s_Instance ? s_Instance->InternalGetCurrentMapPath() : empty;
}

std::string LevelManager::GetCurrentMapName()
{
    return s_Instance ? s_Instance->InternalGetCurrentMapName() : "";
}

Vector3 LevelManager::GetSpawnPosition()
{
    return s_Instance ? s_Instance->InternalGetSpawnPosition() : Vector3{0};
}

void LevelManager::Update(float deltaTime)
{
    if (s_Instance)
        s_Instance->InternalUpdate(deltaTime);
}

void LevelManager::Render()
{
    if (s_Instance)
        s_Instance->InternalRender();
}

void LevelManager::RenderUI()
{
    if (s_Instance)
        s_Instance->InternalRenderUI();
}

void LevelManager::LoadEditorMap(const std::string &mapPath)
{
    if (s_Instance)
        s_Instance->InternalLoadEditorMap(mapPath);
}

std::string LevelManager::ConvertMapNameToPath(const std::string &mapName)
{
    // Simple enough to be static without instance if it only uses PROJECT_ROOT_DIR
    if (mapName.empty())
        return "";

    if (std::filesystem::path(mapName).is_absolute() ||
        std::filesystem::path(mapName).has_extension())
    {
        return mapName;
    }

    return std::string(PROJECT_ROOT_DIR) + "/resources/maps/" + mapName + ".chscene";
}

void LevelManager::RenderEditorMap()
{
    if (s_Instance)
        s_Instance->InternalRenderEditorMap();
}

void LevelManager::RefreshMapEntities()
{
    if (s_Instance)
        s_Instance->InternalRefreshMapEntities();
}

void LevelManager::RefreshUIEntities()
{
    if (s_Instance)
        s_Instance->InternalRefreshUIEntities();
}

void LevelManager::SyncEntitiesToMap()
{
    if (s_Instance)
        s_Instance->InternalSyncEntitiesToMap();
}

void LevelManager::RenderSpawnZone()
{
    if (s_Instance)
        s_Instance->InternalRenderSpawnZone();
}

void LevelManager::DumpMapDiagnostics()
{
    if (s_Instance)
        s_Instance->InternalDumpMapDiagnostics();
}

void LevelManager::InitCollisions()
{
    if (s_Instance)
        s_Instance->InternalInitCollisions();
}

void LevelManager::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    if (s_Instance)
        s_Instance->InternalInitCollisionsWithModels(requiredModels);
}

bool LevelManager::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    return s_Instance ? s_Instance->InternalInitCollisionsWithModelsSafe(requiredModels) : false;
}

GameScene &LevelManager::GetGameScene()
{
    return s_Instance->InternalGetGameScene();
}

Vector3 LevelManager::GetPlayerSpawnPosition()
{
    return s_Instance ? s_Instance->InternalGetSpawnPosition() : Vector3{0};
}

bool LevelManager::HasSpawnZone()
{
    return s_Instance ? s_Instance->InternalHasSpawnZone() : false;
}

MapCollisionInitializer *LevelManager::GetCollisionInitializer()
{
    return s_Instance ? s_Instance->InternalGetCollisionInitializer() : nullptr;
}

void LevelManager::SetPlayer(std::shared_ptr<IPlayer> player)
{
    if (s_Instance)
        s_Instance->InternalSetPlayer(std::move(player));
}

UISystem &LevelManager::GetUISystem()
{
    return Get().InternalGetUISystem();
}

LevelManager::LevelManager(const LevelManagerConfig &config)
    : m_config(config), m_gameScene(std::make_unique<GameScene>()), m_currentMapPath(""),
      m_playerSpawnZone({0}), m_spawnTexture({0}), m_hasSpawnZone(false),
      m_spawnTextureLoaded(false), m_collisionInitializer(nullptr), m_modelLoader(nullptr),
      m_renderManager(nullptr), m_player(nullptr), m_menu(nullptr),
      m_uiSystem(std::make_unique<UISystem>())
{
    m_collisionInitializer = std::make_unique<MapCollisionInitializer>(nullptr, nullptr);

    // Register Default Primitive Models for Runtime / Non-Model Objects
    if (ModelLoader::IsInitialized())
    {
        ModelLoader::RegisterLoadedModel("primitive_cube",
                                         LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f)));
        ModelLoader::RegisterLoadedModel("primitive_sphere",
                                         LoadModelFromMesh(GenMeshSphere(0.5f, 16, 16)));
        ModelLoader::RegisterLoadedModel("primitive_plane",
                                         LoadModelFromMesh(GenMeshPlane(1.0f, 1.0f, 1, 1)));
        ModelLoader::RegisterLoadedModel("primitive_cylinder",
                                         LoadModelFromMesh(GenMeshCylinder(0.5f, 1.0f, 16)));
        CD_CORE_INFO("[LevelManager] Registered default primitive models");
    }

    // Load build manifest (scenes available for runtime)
    std::string manifestPath = std::string(PROJECT_ROOT_DIR) + "/build.manifest";
    if (std::filesystem::exists(manifestPath))
    {
        try
        {
            std::ifstream file(manifestPath);
            if (file.is_open())
            {
                nlohmann::json manifest;
                file >> manifest;
                m_scenes = manifest.value("scenes", std::vector<std::string>());
                CD_CORE_INFO("[LevelManager] Build manifest loaded. Found %zu scenes.",
                             m_scenes.size());
            }
        }
        catch (const std::exception &e)
        {
            CD_CORE_ERROR("[LevelManager] Failed to parse manifest: %s", e.what());
        }
    }
}

LevelManager::~LevelManager()
{
    InternalShutdown();
}

void LevelManager::InternalShutdown()
{
    if (m_gameScene)
        m_gameScene->Cleanup();
    m_currentMapPath = "";
    m_hasSpawnZone = false;
    if (m_spawnTextureLoaded)
    {
        UnloadTexture(m_spawnTexture);
        m_spawnTextureLoaded = false;
    }
    m_collisionInitializer = nullptr;
    m_modelLoader = nullptr;
    m_renderManager = nullptr;
    m_player = nullptr;
    m_menu = nullptr;
}

void LevelManager::InternalSetActiveScene(std::shared_ptr<CHEngine::Scene> scene)
{
    m_activeScene = scene;
    if (CHEngine::SceneManager::IsInitialized())
    {
        CHEngine::SceneManager::LoadScene(scene);
    }
    CD_CORE_INFO("[LevelManager] Active scene set: %s", scene ? scene->GetName().c_str() : "None");
}

bool LevelManager::InternalLoadUIScene(const std::string &path)
{
    CD_CORE_INFO("[LevelManager] Loading UI scene: %s", path.c_str());

    CHEngine::SceneLoader loader;
    CHEngine::GameScene gameScene = loader.LoadScene(path);

    if (gameScene.GetUIElements().empty())
    {
        CD_CORE_ERROR("[LevelManager] Failed to load UI scene or scene has no UI elements: %s",
                      path.c_str());
        return false;
    }

    auto uiScene = std::make_shared<CHEngine::Scene>(std::filesystem::path(path).stem().string());
    InternalPopulateUIFromData(uiScene->GetRegistry(), gameScene.GetUIElements());

    // Apply Background Settings from Metadata
    const auto &meta = gameScene.GetMapMetaData();
    if (meta.backgroundColor.a > 0 || !meta.backgroundTexture.empty())
    {
        auto bgEntity = uiScene->CreateEntity("UI_Background");
        auto &bg = bgEntity.AddComponent<CHEngine::UIBackground>();
        bg.color = meta.backgroundColor;
        bg.texturePath = meta.backgroundTexture;
    }

    if (CHEngine::SceneManager::IsInitialized())
    {
        CHEngine::SceneManager::LoadUIScene(uiScene);
        return true;
    }

    return false;
}

void LevelManager::InternalUnloadUIScene()
{
    if (CHEngine::SceneManager::IsInitialized())
    {
        CHEngine::SceneManager::UnloadUIScene();
    }
}

bool LevelManager::InternalLoadScene(const std::string &path)
{
    std::string mapPath = path;

    // Convert map name to path if it's not already a path or absolute
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos &&
        path.find(".json") == std::string::npos && path.find(".chscene") == std::string::npos)
    {
        mapPath = ConvertMapNameToPath(path);
    }

    CD_CORE_INFO("[LevelManager] Loading level: %s", mapPath.c_str());

    try
    {
        std::string extension = std::filesystem::path(mapPath).extension().string();
        std::vector<std::string> requiredModels;

        // Handling based on file type
        if (extension == ".chscene")
        {
            // Binary Scene Workflow:
            // 1. Deserialize scene first to get object data
            InternalLoadEditorMap(mapPath);

            // 2. Extract models from loaded scene objects
            requiredModels.push_back("player_low"); // Always required
            if (m_gameScene)
            {
                for (const auto &obj : m_gameScene->GetMapObjects())
                {
                    if (!obj.modelName.empty())
                    {
                        // Avoid duplicates
                        bool exists = false;
                        for (const auto &m : requiredModels)
                        {
                            if (m == obj.modelName)
                            {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists)
                            requiredModels.push_back(obj.modelName);
                    }
                }
            }
        }
        else
        {
            // JSON/Legacy Workflow:
            // 1. Analyze map file text for models
            requiredModels = ModelAnalyzer::GetModelsRequiredForMap(mapPath);
            if (requiredModels.empty())
            {
                requiredModels.emplace_back("player_low");
            }
        }

        // 3. Load required models
        if (ModelLoader::IsInitialized())
        {
            ModelLoader::LoadGameModelsSelective(requiredModels);
        }

        // 4. Initialize collision system with models
        if (!InternalInitCollisionsWithModelsSafe(requiredModels))
        {
            CD_CORE_ERROR("[LevelManager] Failed to initialize collision system for map: %s",
                          mapPath.c_str());
            return false;
        }

        // 5. Load map contents (Only for JSON - .chscene already loaded in step 1)
        if (extension != ".chscene")
        {
            InternalLoadEditorMap(mapPath);
        }

        CD_CORE_INFO("[LevelManager] Level loaded successfully: %s", mapPath.c_str());
        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("[LevelManager] Failed to load level %s: %s", mapPath.c_str(), e.what());
        return false;
    }
}

bool LevelManager::InternalLoadSceneByIndex(int index)
{
    if (index < 0 || index >= (int)m_scenes.size())
    {
        CD_CORE_ERROR("[LevelManager] Scene index out of bounds: %d (Total: %zu)", index,
                      m_scenes.size());
        return false;
    }

    return InternalLoadSceneByName(m_scenes[index]);
}

bool LevelManager::InternalLoadSceneByName(const std::string &name)
{
    // Try to find the scene in mapped scenes
    std::string sceneFile = name;

    // Add .chscene extension if no extension present
    if (name.find(".chscene") == std::string::npos && name.find(".json") == std::string::npos)
    {
        sceneFile += ".chscene";
    }

    // Attempt to load from resources/maps/ folder relative to project root
    std::string fullPath = std::string(PROJECT_ROOT_DIR) + "/resources/maps/" + sceneFile;
    if (!std::filesystem::exists(fullPath))
    {
        fullPath = std::string(PROJECT_ROOT_DIR) + "/Scenes/" + sceneFile;
    }

    return InternalLoadScene(fullPath);
}

bool LevelManager::InternalIsMapLoaded() const
{
    return !m_currentMapPath.empty();
}

void LevelManager::InternalUpdate(float deltaTime)
{
    // Note: Player is now managed via ECS (PlayerComponent)
    // No need to update IPlayer service reference

    // Map update logic if needed
    (void)deltaTime;
}

void LevelManager::InternalRender()
{
    // Rendering is handled by RenderingSystem or MapRenderer
}

void LevelManager::InternalRenderUI()
{
    if (m_uiScene && m_uiSystem)
    {
        m_uiSystem->Render(m_uiScene->GetRegistry(), GetScreenWidth(), GetScreenHeight());
    }
}

// Collision initialization methods
void LevelManager::InternalInitCollisions()
{
    if (m_collisionInitializer && m_activeScene)
    {
        m_collisionInitializer->InitializeCollisions(m_activeScene->GetRegistry(), *m_gameScene);
    }
}

void LevelManager::InternalInitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer && m_activeScene)
    {
        m_collisionInitializer->InitializeCollisionsWithModels(m_activeScene->GetRegistry(),
                                                               *m_gameScene, requiredModels);
    }
}

bool LevelManager::InternalInitCollisionsWithModelsSafe(
    const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer && m_activeScene)
    {
        return m_collisionInitializer->InitializeCollisionsWithModelsSafe(
            m_activeScene->GetRegistry(), *m_gameScene, requiredModels);
    }
    return false;
}

void LevelManager::InternalRenderEditorMap()
{
    MapRenderer renderer;
    Camera3D dummyCamera = {0};

    for (const auto &object : m_gameScene->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL || object.type == MapObjectType::SPAWN_ZONE)
        {
            continue;
        }

        renderer.RenderMapObject(object, m_gameScene->GetMapModels(), m_gameScene->GetMapTextures(),
                                 dummyCamera, false);
    }
}

void LevelManager::InternalRenderSpawnZone() const
{
    if (!m_hasSpawnZone)
    {
        return;
    }

    if (!m_spawnTextureLoaded)
    {
        std::string texturePath =
            std::string(PROJECT_ROOT_DIR) + "/resources/boxes/playerSpawnTexture.png";
        if (FileExists(texturePath.c_str()))
        {
            auto *self = const_cast<LevelManager *>(this);
            self->m_spawnTexture = LoadTexture(texturePath.c_str());
            if (self->m_spawnTexture.id != 0)
            {
                self->m_spawnTextureLoaded = true;
            }
        }
    }

    if (m_spawnTextureLoaded)
    {
        Vector3 size = {m_playerSpawnZone.max.x - m_playerSpawnZone.min.x,
                        m_playerSpawnZone.max.y - m_playerSpawnZone.min.y,
                        m_playerSpawnZone.max.z - m_playerSpawnZone.min.z};

        Vector3 center = {(m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
                          (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
                          (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f};

        MapRenderer renderer;
        float spawnSize = (size.x + size.y + size.z) / 3.0f;
        renderer.RenderSpawnZone(m_spawnTexture, center, spawnSize, WHITE, true);
    }
}

void LevelManager::InternalDumpMapDiagnostics() const
{
    // Implementation here or remove if not needed
}

void LevelManager::InternalLoadEditorMap(const std::string &mapPath)
{

    if (!std::filesystem::exists(mapPath))
    {
        CD_CORE_ERROR("LevelManager::LoadEditorMap() - Map file does not exist: %s",
                      mapPath.c_str());
        return;
    }

    ModelLoader::ClearInstances();

    for (const auto &pair : m_gameScene->GetMapModels())
    {
        ModelLoader::UnloadModel(pair.first);
    }

    m_gameScene->Cleanup();
    m_hasSpawnZone = false;
    m_playerSpawnZone = {0};
    CollisionManager::ClearColliders();

    std::string extension = std::filesystem::path(mapPath).extension().string();
    if (extension == ".json" || extension == ".JSON" || extension == ".chscene")
    {
        // Hazel-style: .chscene for scenes (binary), .json for maps only
        if (extension == ".chscene")
        {
            // Binary scene format - primary format for editor scenes
            auto sharedScene = std::shared_ptr<CHEngine::GameScene>(m_gameScene.get(),
                                                                    [](CHEngine::GameScene *) {});
            CHEngine::SceneSerializer serializer(sharedScene);
            if (!serializer.DeserializeBinary(mapPath))
            {
                CD_CORE_ERROR(
                    "LevelManager::LoadEditorMap() - Failed to deserialize binary scene: %s",
                    mapPath.c_str());
                return;
            }
            CHEngine::SceneLoader().LoadSkyboxForScene(*m_gameScene);
            if (m_gameScene->GetSkyBox())
                m_gameScene->GetSkyBox()->UpdateGammaFromConfig();
        }
        else
        {
            // JSON format - for maps only, not scenes
            MapService mapService;
            if (!mapService.LoadScene(mapPath, *m_gameScene))
            {
                CD_CORE_ERROR("LevelManager::LoadEditorMap() - MapService failed to load map: %s",
                              mapPath.c_str());
                return;
            }
        }

        m_currentMapPath = mapPath;

        if (!m_gameScene->GetMapModels().empty())
        {
            for (const auto &p : m_gameScene->GetMapModels())
            {
                if (p.second.meshCount > 0)
                {
                    ModelLoader::RegisterLoadedModel(p.first, p.second);
                }
            }
        }

        if (!m_gameScene->GetMapMetaData().skyboxTexture.empty())
        {
            CHEngine::SceneLoader mapLoader;
            mapLoader.LoadSkyboxForScene(*m_gameScene);
            if (m_gameScene->GetSkyBox())
                m_gameScene->GetSkyBox()->UpdateGammaFromConfig();
        }

        // Automatic texture loading for map objects (Runtime/Editor support)
        auto &textures = m_gameScene->GetMapTexturesMutable();
        for (const auto &obj : m_gameScene->GetMapObjects())
        {
            if (!obj.texturePath.empty() && textures.find(obj.texturePath) == textures.end())
            {
                Texture2D tex = LoadTexture(obj.texturePath.c_str());
                if (tex.id != 0)
                {
                    CD_CORE_INFO("[LevelManager] Loaded texture for object: %s",
                                 obj.texturePath.c_str());
                    textures[obj.texturePath] = tex;
                }
                else
                {
                    CD_CORE_WARN("[LevelManager] Failed to load texture: %s",
                                 obj.texturePath.c_str());
                }
            }
        }
    }

    for (size_t i = 0; i < m_gameScene->GetMapObjects().size(); ++i)
    {
        const auto &object = m_gameScene->GetMapObjects()[i];

        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) ||
            !std::isfinite(object.position.z))
            continue;

        Vector3 colliderSize = object.scale;
        bool useBVHCollision = false;
        Collision collision;

        switch (object.type)
        {
        case MapObjectType::CUBE:
            colliderSize = Vector3{std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                                   std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                                   std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)};
            break;
        case MapObjectType::MODEL:
        {
            const Model *modelPtr = nullptr;
            if (!object.modelName.empty())
            {
                auto modelOpt = ModelLoader::GetModelByName(object.modelName);
                if (modelOpt)
                    modelPtr = &modelOpt->get();
            }

            if (modelPtr)
            {
                Matrix translation =
                    MatrixTranslate(object.position.x, object.position.y, object.position.z);
                Matrix scaleMatrix = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
                Vector3 rot = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                               object.rotation.z * DEG2RAD};
                Matrix rotationMatrix = MatrixRotateXYZ(rot);
                Matrix transform =
                    MatrixMultiply(scaleMatrix, MatrixMultiply(rotationMatrix, translation));

                collision = Collision();
                collision.BuildFromModel(const_cast<Model *>(modelPtr), transform);
                collision.SetCollisionType(CollisionType::BVH_ONLY);
                useBVHCollision = true;
            }
        }
        break;
        case MapObjectType::SPAWN_ZONE:
        {
            const float spawnSize = 2.0f;
            m_playerSpawnZone.min = {object.position.x - spawnSize / 2,
                                     object.position.y - spawnSize / 2,
                                     object.position.z - spawnSize / 2};
            m_playerSpawnZone.max = {object.position.x + spawnSize / 2,
                                     object.position.y + spawnSize / 2,
                                     object.position.z + spawnSize / 2};
            m_hasSpawnZone = true;
            CD_CORE_INFO("[LevelManager] Found SPAWN_ZONE object at (%.2f, %.2f, %.2f)",
                         object.position.x, object.position.y, object.position.z);
        }
        break;
        default:
            break;
        }

        if (useBVHCollision)
        {
            collision.SetCollisionType(CollisionType::BVH_ONLY);
            CollisionManager::AddCollider(std::make_shared<Collision>(std::move(collision)));
        }
        else if (object.type != MapObjectType::LIGHT && object.type != MapObjectType::SPAWN_ZONE)
        {
            Vector3 halfSize = Vector3Scale(colliderSize, 0.5f);
            collision = Collision(object.position, halfSize);
            collision.SetCollisionType(CollisionType::BVH_ONLY);
            CollisionManager::AddCollider(std::make_shared<Collision>(std::move(collision)));
        }
    }

    if (m_gameScene->GetMapMetaData().startPosition.x != 0.0f ||
        m_gameScene->GetMapMetaData().startPosition.y != 0.0f ||
        m_gameScene->GetMapMetaData().startPosition.z != 0.0f)
    {
        const float spawnSize = 2.0f;
        Vector3 spawnPos = m_gameScene->GetMapMetaData().startPosition;
        m_playerSpawnZone.min = {spawnPos.x - spawnSize / 2, spawnPos.y - spawnSize / 2,
                                 spawnPos.z - spawnSize / 2};
        m_playerSpawnZone.max = {spawnPos.x + spawnSize / 2, spawnPos.y + spawnSize / 2,
                                 spawnPos.z + spawnSize / 2};
        m_hasSpawnZone = true;
    }

    CollisionManager::Initialize();

    // Create instances for all MODEL objects
    auto available = ModelLoader::GetAvailableModels();
    for (const auto &object : m_gameScene->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string candidateName = object.modelName;
            bool exists =
                (std::find(available.begin(), available.end(), candidateName) != available.end());

            if (!exists)
            {
                std::string stem = std::filesystem::path(candidateName).stem().string();
                if (std::find(available.begin(), available.end(), stem) != available.end())
                {
                    candidateName = stem;
                    exists = true;
                }
            }

            if (exists)
            {
                ModelInstanceConfig cfg;
                cfg.position = object.position;
                cfg.rotation = object.rotation;
                cfg.scale = (object.scale.x != 0.0f) ? object.scale.x : 1.0f;
                cfg.color = object.color;
                cfg.spawn = true;
                ModelLoader::AddInstanceEx(candidateName, cfg);
            }
        }
    }
}
void LevelManager::InternalRefreshUIEntities()
{
    if (!m_activeScene)
        return;
    auto &registry = m_activeScene->GetRegistry();

    // 1. Remove all existing UI entities
    auto view = registry.view<CHEngine::UIElementIndex>();
    registry.destroy(view.begin(), view.end());

    // 2. Clear all entities with UI components just to be safe
    auto uiGroup = registry.view<CHEngine::RectTransform>();
    registry.destroy(uiGroup.begin(), uiGroup.end());

    // 3. Recreate entities from GameScene data
    InternalPopulateUIFromData(registry, m_gameScene->GetUIElements());

    CD_INFO("[LevelManager] Refreshed %d UI entities in ECS.",
            (int)m_gameScene->GetUIElements().size());
}

void LevelManager::InternalPopulateUIFromData(entt::registry &registry,
                                              const std::vector<UIElementData> &uiElements)
{
    for (int i = 0; i < (int)uiElements.size(); i++)
    {
        const auto &data = uiElements[i];

        auto entity = registry.create();

        // Always add Index and Transform
        registry.emplace<CHEngine::UIElementIndex>(entity, i);
        registry.emplace<CHEngine::NameComponent>(entity, data.name);

        CHEngine::RectTransform transform;
        transform.position = data.position;
        transform.size = data.size;
        transform.pivot = data.pivot;
        transform.anchor = (CHEngine::UIAnchor)data.anchor;
        transform.active = data.isActive;
        registry.emplace<CHEngine::RectTransform>(entity, transform);

        // Add specialized components based on type
        if (data.type == "button")
        {
            CHEngine::UIButton button;
            button.normalColor = data.normalColor;
            button.hoverColor = data.hoverColor;
            button.pressedColor = data.pressedColor;
            button.borderRadius = data.borderRadius;
            button.borderWidth = data.borderWidth;
            button.borderColor = data.borderColor;
            button.actionType = data.actionType;
            button.actionTarget = data.actionTarget;
            button.eventId = data.eventId;
            registry.emplace<CHEngine::UIButton>(entity, button);

            if (!data.texturePath.empty())
            {
                CHEngine::UIImage image;
                image.texturePath = data.texturePath;
                image.tint = data.tint;
                image.borderRadius = data.borderRadius;
                image.borderWidth = data.borderWidth;
                image.borderColor = data.borderColor;
                registry.emplace<CHEngine::UIImage>(entity, image);
            }

            CHEngine::UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            text.spacing = data.spacing;
            registry.emplace<CHEngine::UIText>(entity, text);
        }
        else if (data.type == "imgui_button")
        {
            CHEngine::ImGuiComponent imgui;
            imgui.label = data.text;
            imgui.eventId = data.eventId;
            imgui.isButton = true;
            registry.emplace<CHEngine::ImGuiComponent>(entity, imgui);
        }
        else if (data.type == "text")
        {
            CHEngine::UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            text.spacing = data.spacing;
            registry.emplace<CHEngine::UIText>(entity, text);
        }
        else if (data.type == "imgui_text")
        {
            CHEngine::ImGuiComponent imgui;
            imgui.label = data.text;
            imgui.isButton = false;
            registry.emplace<CHEngine::ImGuiComponent>(entity, imgui);
        }
        else if (data.type == "image")
        {
            CHEngine::UIImage image;
            image.tint = data.tint;
            image.borderRadius = data.borderRadius;
            image.borderWidth = data.borderWidth;
            image.borderColor = data.borderColor;
            image.texturePath = data.texturePath;
            registry.emplace<CHEngine::UIImage>(entity, image);
        }

        // Add Scripting if present
        if (!data.scriptPath.empty())
        {
            registry.emplace<CHEngine::CSharpScriptComponent>(entity, data.scriptPath, false);
        }
    }
}

void LevelManager::InternalRefreshMapEntities()
{
    if (!m_activeScene)
    {
        CD_CORE_WARN("[LevelManager] No active scene to refresh entities!");
        return;
    }

    auto &registry = m_activeScene->GetRegistry();

    // 1. Remove all existing map entities (those created by this system)
    auto mapEntities = registry.view<CHEngine::MapObjectIndex>();
    registry.destroy(mapEntities.begin(), mapEntities.end());

    // 2. Recreate entities from GameScene data
    auto &mapObjects = m_gameScene->GetMapObjects();
    for (int i = 0; i < (int)mapObjects.size(); i++)
    {
        const auto &data = mapObjects[i];

        auto entity = registry.create();
        registry.emplace<CHEngine::MapObjectIndex>(entity, i);
        registry.emplace<CHEngine::NameComponent>(entity, data.name);

        registry.emplace<CHEngine::TransformComponent>(entity, data.position, data.rotation,
                                                       data.scale);

        if (!data.scriptPath.empty())
        {
            registry.emplace<CHEngine::CSharpScriptComponent>(entity, data.scriptPath, false);
        }

        // 4. Add RenderComponent for Visibility in Runtime
        CHEngine::RenderComponent rc;
        bool hasRender = false;

        if (!data.modelName.empty())
        {
            rc.modelName = data.modelName;

            // Resolve model via AssetManager
            auto modelOpt = CHEngine::AssetManager::GetModel(data.modelName);
            if (modelOpt)
            {
                rc.model = &modelOpt->get();
                hasRender = true;
            }
        }
        else
        {
            // Handle Primitives
            std::string primitiveName;
            switch (data.type)
            {
            case MapObjectType::CUBE:
                primitiveName = "primitive_cube";
                break;
            case MapObjectType::SPHERE:
                primitiveName = "primitive_sphere";
                break;
            case MapObjectType::PLANE:
                primitiveName = "primitive_plane";
                rc.renderLayer = -1; // Standard for floors
                break;
            case MapObjectType::CYLINDER:
                primitiveName = "primitive_cylinder";
                break;
            default:
                break;
            }

            if (!primitiveName.empty())
            {
                auto modelOpt = CHEngine::AssetManager::GetModel(primitiveName);
                if (modelOpt)
                {
                    rc.model = &modelOpt->get();
                    rc.modelName = primitiveName;
                    hasRender = true;
                }
            }
        }

        // 4a. Handle Textures
        if (!data.texturePath.empty())
        {
            auto &textures = m_gameScene->GetMapTextures();
            if (textures.count(data.texturePath))
            {
                rc.texture = const_cast<Texture2D *>(&textures.at(data.texturePath));
                rc.tiling = data.tiling;
            }
        }

        if (hasRender)
        {
            rc.tint = data.color;
            rc.visible = true;
            // Default layer if not set
            if (rc.renderLayer == 0 && data.type != MapObjectType::PLANE)
                rc.renderLayer = 0;

            registry.emplace<CHEngine::RenderComponent>(entity, rc);
        }

        // 4b. Handle Lights
        if (data.type == MapObjectType::LIGHT)
        {
            // Add LightSource tag for identification
            registry.emplace<CHEngine::TagComponent>(entity, "LightSource");

            // In future, add a LightComponent here
            // registry.emplace<LightComponent>(entity, data.color, 10.0f);
        }

        // 5. Add CollisionComponent for ECS-based queries
        if (data.isPlatform || data.isObstacle)
        {
            CHEngine::CollisionComponent cc;
            cc.collisionLayer = 0; // Default layer

            // Primitive bounds approximation if not specified
            if (data.type == MapObjectType::CUBE)
                cc.bounds = {Vector3{-0.5f, -0.5f, -0.5f}, Vector3{0.5f, 0.5f, 0.5f}};
            else if (data.type == MapObjectType::PLANE)
                cc.bounds = {Vector3{-0.5f, -0.05f, -0.5f}, Vector3{0.5f, 0.05f, 0.5f}};

            registry.emplace<CHEngine::CollisionComponent>(entity, cc);
        }

        CD_INFO("[LevelManager] Created ECS Entity for Map Object[%d]: %s (Type: %d)", i,
                data.name.c_str(), (int)data.type);
    }

    // 3. Sync Spawn Position for Player
    auto playerView = registry.view<CHEngine::PlayerComponent>();
    if (!playerView.empty())
    {
        auto playerEntity = playerView.front();
        auto &pc = registry.get<CHEngine::PlayerComponent>(playerEntity);

        Vector3 spawnPos = {0, 0, 0};
        bool foundSpawn = false;

        for (const auto &obj : mapObjects)
        {
            if (obj.type == MapObjectType::SPAWN_ZONE)
            {
                spawnPos = obj.position;
                foundSpawn = true;
                break;
            }
        }

        if (!foundSpawn)
        {
            spawnPos = m_gameScene->GetMapMetaData().startPosition;
            if (spawnPos.x != 0 || spawnPos.y != 0 || spawnPos.z != 0)
                foundSpawn = true;
        }

        if (foundSpawn)
        {
            pc.spawnPosition = spawnPos;
            CD_INFO("[LevelManager] Updated Player spawn position to (%.2f, %.2f, %.2f)",
                    spawnPos.x, spawnPos.y, spawnPos.z);
        }
    }

    // 4. Initialize collisions for the new entities
    InternalInitCollisions();
}

void LevelManager::InternalSyncEntitiesToMap()
{
    if (!m_activeScene)
        return;

    auto &registry = m_activeScene->GetRegistry();
    auto &mapObjects = m_gameScene->GetMapObjectsMutable();

    // Sync 3D Map Objects
    auto view = registry.view<CHEngine::MapObjectIndex, CHEngine::TransformComponent>();
    for (auto entity : view)
    {
        auto &idxComp = view.get<CHEngine::MapObjectIndex>(entity);
        auto &transform = view.get<CHEngine::TransformComponent>(entity);

        if (idxComp.index >= 0 && idxComp.index < (int)mapObjects.size())
        {
            auto &data = mapObjects[idxComp.index];
            data.position = transform.position;
            data.rotation = transform.rotation;
            data.scale = transform.scale;
        }
    }

    // Sync UI Elements
    auto &uiElements = m_gameScene->GetUIElementsMutable();
    auto uiView = registry.view<CHEngine::UIElementIndex, CHEngine::RectTransform>();
    for (auto entity : uiView)
    {
        auto &idxComp = uiView.get<CHEngine::UIElementIndex>(entity);
        auto &transform = uiView.get<CHEngine::RectTransform>(entity);

        if (idxComp.index >= 0 && idxComp.index < (int)uiElements.size())
        {
            auto &data = uiElements[idxComp.index];
            data.position = transform.position;
            data.size = transform.size;
        }
    }
}

} // namespace CHEngine
