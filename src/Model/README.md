# 🎯 Enhanced Model Loader

An improved Model Loader for **Chained Decos** with OOP principles, caching, and cleaner code organization.

---

## ✨ Overview

The Enhanced Model Loader adds modern C++ practices (smart pointers, RAII), a model cache (LRU), detailed loading statistics, and a clean API while preserving **full backward compatibility** with the existing model manager.

This README contains usage examples, JSON configuration format, API reference snippets, and migration tips.

---

## ✅ Key Features

* **Backward compatibility** — existing methods keep working.
* **Smart pointers & RAII** — automatic memory management.
* **Model Cache** — automatic caching with LRU eviction and hit-rate stats.
* **LOD support** — enable/disable Level-of-Detail.
* **Detailed statistics** — loading time, success/failure counts, hit/miss rates.
* **Tags & categories** — flexible instance filtering.
* **Dynamic loading/unloading** — control models at runtime.

---

## 🎮 API Quick Reference

```cpp
Models modelManager;

// Settings
modelManager.SetCacheEnabled(true);
modelManager.SetMaxCacheSize(20);
modelManager.EnableLOD(true);

// Enhanced instance creation
ModelInstanceConfig config;
config.position = {5, 0, 5};
config.rotation = {0, 45, 0}; // new
config.scale = 1.5f;
config.color = BLUE;
config.tag = "player"; // for filtering
modelManager.AddInstanceEx("playerModel", config);

// Dynamic model management
modelManager.LoadSingleModel("enemy", "/path/to/enemy.glb");
modelManager.UnloadModel("unused");
modelManager.ReloadModel("updated");

// Search & filtering
auto enemies = modelManager.GetInstancesByTag("enemy");
auto structures = modelManager.GetInstancesByCategory("building");

// Statistics
const auto& stats = modelManager.GetLoadingStats();
modelManager.PrintStatistics();
modelManager.PrintCacheInfo();
```

---

## 📋 JSON Configuration Example

```json
[
  {
    "name": "player",
    "path": "/resources/player.glb",
    "category": "character",
    "spawn": false,
    "preload": true,
    "priority": 10,
    "lodDistance": 50.0,
    "hasAnimations": true,
    "instances": [
      {
        "position": { "x": 0, "y": 0, "z": 0 },
        "rotation": { "x": 0, "y": 45, "z": 0 },
        "scale": 1.0,
        "color": "blue",
        "tag": "main_character",
        "spawn": true
      }
    ]
  }
]
```

### 🆕 New JSON Fields

* `category` — Model categorization (e.g. `character`, `building`).
* `priority` — Loading priority (0..10). Higher values load earlier.
* `lodDistance` — Distance threshold for Level-of-Detail switching.
* `preload` — If `true`, load model at startup.
* `hasAnimations` — If `true`, activate animation support.
* `rotation` — Instance rotation (x, y, z in degrees).
* `tag` — A string tag for easy filtering of instances.

---

## 🎯 Usage Example (Engine.cpp)

```cpp
void Engine::Init() {
    // Configure the enhanced Model Manager
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(20);

    // Load from JSON (backward compatible)
    m_models.LoadModelsFromJson("src/models.json");

    // Print statistics
    m_models.PrintStatistics();

    // Create a dynamic instance
    ModelInstanceConfig decorConfig;
    decorConfig.position = {5, 0, 5};
    decorConfig.tag = "decoration";
    m_models.AddInstanceEx("arc", decorConfig);
}
```

### Debug Info

`DrawDebugInfo()` displays:

* Number of loaded models
* Loading success rate
* Cache statistics (size, hits, misses)
* Buttons to trigger model load/unload/optimize actions

---

## ⌨️ Hotkeys

| Key | Function                                               |
| --- | ------------------------------------------------------ |
| F5  | `LoadAdditionalModels()` — load extra models           |
| F6  | `SpawnEnemies()` — spawn enemies                       |
| F7  | `SpawnPickups()` — spawn pickups                       |
| F8  | `OptimizeModelPerformance()` — run model optimizations |
| F9  | Show models / instances info                           |

---

## 📊 Performance Features

### Model Cache

* Automatic LRU cache for frequently used models.
* Configurable max size and cleanup timers.

```cpp
modelManager.SetMaxCacheSize(50);
modelManager.CleanupUnusedModels(300); // seconds (5 minutes)
modelManager.OptimizeCache();
```

### Loading Statistics

```cpp
const auto& stats = modelManager.GetLoadingStats();
// stats.totalModels
// stats.loadedModels
// stats.failedModels
// stats.totalInstances
// stats.loadingTime
// stats.GetSuccessRate()
```

---

## 🔄 Migration / Backward Compatibility

### What stays the same

```cpp
m_models.LoadModelsFromJson("path/to/config.json");
m_models.DrawAllModels();
m_models.GetModelByName("modelName");
```

### Recommended (new) usage

```cpp
// old approach using raw json objects
// json instanceJson;
// instanceJson["position"]["x"] = 5.0f;
// m_models.AddInstance(instanceJson, modelPtr, "name", animPtr);

// new approach: use strongly-typed config
ModelInstanceConfig config;
config.position = {5, 0, 0};
config.tag = "decoration";
m_models.AddInstanceEx("name", config);
```

---

## 🛠 Build / CMake

Files are automatically added to `CMakeLists.txt`:

```cmake
# Enhanced Model Library with OOP improvements
add_library(model STATIC
    # Core model classes (unchanged)
    Model.cpp Model.h
    ModelInstance.cpp ModelInstance.h
    Animation.cpp Animation.h

    # Enhanced OOP components (new)
    ModelConfig.h
    JsonHelper.cpp JsonHelper.h
    ModelCache.cpp ModelCache.h
)
```

---

## 🚀 Examples

### Basic

```cpp
Models manager;
manager.LoadModelsFromJson("config.json");
manager.DrawAllModels(); // works as before
```

### Enhanced (dynamic spawn + filtering)

```cpp
Models manager;
manager.SetCacheEnabled(true);
manager.LoadModelsFromJson("enhanced_config.json");

for (int i = 0; i < 5; ++i) {
    ModelInstanceConfig enemy;
    enemy.position = {i * 5.0f, 0, 10};
    enemy.color = RED;
    enemy.tag = "enemy";
    manager.AddInstanceEx("enemyModel", enemy);
}

auto enemies = manager.GetInstancesByTag("enemy");
TraceLog(LOG_INFO, "Spawned %zu enemies", enemies.size());

manager.PrintStatistics();
```

---

## ✨ Advantages

1. **Backward Compatibility** — old code keeps working.
2. **Performance** — caching and optimization routines.
3. **Monitoring** — comprehensive runtime statistics.
4. **Flexibility** — tags, categories, LOD, dynamic loading.
5. **Clean code** — OOP, RAII, strongly-typed configs.
6. **Extensible** — easy to add features.

---

## 📌 Quick Start

1. Add module files to your `CMakeLists.txt` (see Build / CMake section).
2. Enable cache and set size in `Engine::Init()`.
3. Provide `models.json` using the improved schema (see JSON example).
4. Use `AddInstanceEx()` for new instances, or keep using `LoadModelsFromJson()` for full configs.

---

## 📬 Feedback

If you want a shorter **Quick Start** README for your repository root or a Ukrainian translation — tell me and I will add it.

---

**Ready for use! All improvements preserve full backward compatibility.** 🎉
