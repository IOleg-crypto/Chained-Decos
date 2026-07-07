# Engine Simplification + UI Serialization Fix — Master Plan

## Overview

Complete plan to fix UI serialization and simplify the Chained Decos engine.
Total estimated savings: ~1,957 lines (~21% of analyzed code).

---

## Phase 1: Quick Wins (P0-P2) — Zero Risk

### P0: Remove dead `Color()` if-constexpr branch
**File:** `engine/reflection/reflection.h:149-159`
- Both branches execute identical code
- Delete the `if constexpr` wrapper, keep one call

### P1: Remove dead `Nested()` else branch
**File:** `engine/reflection/reflection.h:251-263`
- `T_Archive` is always `IPropertyArchive`-derived
- The `else` branch is unreachable

### P2: Remove commented-out code (~60 lines)
- `engine/graphics/api/opengl/opengl_renderer_api.cpp:10-33` (23 lines debug callback)
- `engine/reflection/reflection.h:443-474` (32 lines old macros)
- `engine/runtime/runtime_layer.cpp:184-185,262-263` (debug logging)
- `engine/platform/backends/glfw/glfw_window.cpp:280` (stbi flip)
- `engine/reflection/reflection.h:14` (CH_ARRAY_SIZE macro — use std::size)

---

## Phase 2: YAML Simplification — ~735 Lines

### Step 1: Delete `yaml_conversions.h` (100% duplicate)
**File:** `engine/scene/yaml_conversions.h` (245 lines)
- Contains identical `convert<>` and `operator<<` as `yaml.h`
- Has self-aware comment: `// TODO : some day, maybe i refactor this`
- Update all includes to use `yaml.h` instead

### Step 2: Activate reflect-cpp YAML backend
**File:** `cmake/external/reflect-cpp.cmake`
- `REFLECTCPP_YAML` is already `ON`
- Use `rfl::yaml::write` / `rfl::yaml::read` for component structs
- Replace manual `convert<>` specializations in `yaml.h`

### Step 3: Simplify `yaml.h`
**File:** `engine/scene/yaml.h` (490 lines → ~100 lines)
- Keep only types that reflect-cpp can't handle natively
- Remove all `convert<>` specializations that have rfl equivalents
- Remove `operator<<` overloads replaced by rfl

### Step 4: Fix variant serialization in `yaml.h`
**Current:** `convert<std::variant>::decode()` returns `false` always
**Fix:** Implement proper variant decode using type tag

---

## Phase 3: Remove cereal — ~100 Lines

### Step 1: Replace cereal with reflect-cpp binary
**Files:** `engine/assets/model_data.h`, `engine/graphics/api/model_data.h`
- Replace `#include <cereal/archives/binary.hpp>` with rfl binary
- cereal is only used for model cache serialization (3 files)

### Step 2: Remove cereal dependency
**File:** `cmake/external/cereal.cmake`
- Delete cmake config
- Remove from `cmake/Dependencies.cmake`

---

## Phase 4: Add magic_enum — ~50 Lines Saved

### Step 1: Add magic_enum as submodule
```bash
cd thirdparty && git submodule add https://github.com/Neargye/magic_enum.git
```

### Step 2: Create `cmake/external/magic_enum.cmake`
- Header-only, INTERFACE target

### Step 3: Replace manual enum serialization
**Files:** Various components with enum fields
- Use `magic_enum::enum_name()` instead of hand-written tables
- Remove commented-out `CH_ENUM` macros

---

## Phase 5: Remove Dead Dependencies — ~102 Lines

### Files to delete/modify:
- `cmake/external/protobuf.cmake` (46 lines) — delete
- `cmake/external/gamenetworkingsockets.cmake` (56 lines) — delete
- Remove `thirdparty/enet` submodule reference
- Remove any references in `cmake/Dependencies.cmake`

---

## Phase 6: UI Widget Types — ~170 Lines

### Step 1: Mark 16 unimplemented types
**File:** `engine/graphics/ui/ui_data_components.h`
- Add comment `// Not yet implemented — no render function`
- Keep structs in variant for future use

### Step 2: Remove from factory
**File:** `engine/graphics/ui/ui_factory.cpp`
- Remove `Register<T>()` calls for unimplemented types
- Keep struct definitions for API completeness

### Step 3: Simplify fallback rendering
**File:** `engine/graphics/ui/ui_control_renderer.cpp`
- The 16 types all fall through to generic rectangle rendering
- Document this explicitly in code

---

## Phase 7: UI Serialization Fix — THE MAIN BUG

### Root Causes:
1. Serialization key mismatch: code uses `"UI ControlComponent"`, YAML has `"WidgetComponent"`
2. `std::variant` not serialized through reflection
3. `BoxStyle`/`TextStyle` not serialized (plain structs, no RFL)

### Solution: Custom Serialize/Deserialize for UIControlComponent

#### Step 1: Register with custom lambdas
**File:** `engine/scene/component_registry.cpp`
```cpp
// Change from:
RegisterReflective<UIControlComponent>("UI Control", ICON_FA_WINDOW_RESTORE, "UI");
// To:
auto& metadata = ComponentRegistry::Register<UIControlComponent>("Widget", ICON_FA_WINDOW_RESTORE, "UI");
metadata.Serialize = [](YAML::Emitter& out, Entity entity) { ... };
metadata.Deserialize = [](Entity entity, YAML::Node node) { ... };
```

#### Step 2: Implement Serialize lambda
- Write `WidgetComponent:` key
- Write `Box Style:` nested map (all UIStyle fields)
- Write `Text Style:` nested map (all TextStyle fields)
- Write `Widget Type:` integer (1=Button, 3=Text, 9=Image, etc.)
- Write variant-specific fields using `std::visit` + `Overloaded`

#### Step 3: Implement Deserialize lambda
- Read `WidgetComponent` node
- Read `Box Style` → populate `comp.BoxStyle`
- Read `Text Style` → populate `comp.TextStyle`
- Read `Widget Type` → switch on value to create correct variant alternative
- Read variant-specific fields

#### Step 4: Map Widget Type integers
```cpp
enum WidgetType : int {
    None = 0,
    Button = 1,
    Label = 3,
    Slider = 5,
    Checkbox = 6,
    ProgressBar = 7,
    Panel = 8,
    Image = 9,
    // ... others
};
```

#### Step 5: Handle existing YAML format
The existing .chscene files use:
```yaml
WidgetComponent:
  Box Style: {...}
  Text Style: {...}
  Widget Type: 1
  Label: Start Game
  Interactable: true
  Auto Size: false
```

This format must be preserved for backward compatibility.

---

## Phase 8: Reflection System Cleanup — ~80 Lines

### Step 1: Deduplicate `Property()` overloads
**File:** `engine/reflection/reflection.h:266-297 vs 327-367`
- Merge the two overloads using a default parameter for `meta`
- ~80 lines saved

### Step 2: Remove dead interface methods
**File:** `engine/reflection/reflection.h`
- `Action()` — never called (CH_ACTION macro commented out)
- `WidgetHint::Input` — never referenced
- Simplify `IPropertyArchive` vtable

---

## Execution Order

1. **Phase 1** (P0-P2): Quick wins, zero risk — 5 min
2. **Phase 5**: Remove dead dependencies — 10 min
3. **Phase 2**: YAML simplification — 2-3 hours
4. **Phase 3**: Remove cereal — 30 min
5. **Phase 4**: Add magic_enum — 30 min
6. **Phase 6**: UI widget types — 30 min
7. **Phase 7**: UI serialization fix — 2-3 hours
8. **Phase 8**: Reflection cleanup — 1 hour

**Total estimated time: 7-8 hours**

---

## Risk Assessment

| Phase | Risk | Mitigation |
|---|---|---|
| P0-P2 | None | Just deletions |
| Phase 2 | Medium | Test all YAML load/save cycles |
| Phase 3 | Low | Binary format is isolated |
| Phase 4 | Low | Header-only, no ABI changes |
| Phase 5 | None | Dead code removal |
| Phase 6 | High | May break UI if types used in game |
| Phase 7 | Medium | Must preserve YAML format |
| Phase 8 | Low | Internal refactor |

---

## Files Modified (Complete List)

### Phase 1:
- `engine/reflection/reflection.h` — remove dead branches + commented code
- `engine/graphics/api/opengl/opengl_renderer_api.cpp` — remove debug callback
- `engine/runtime/runtime_layer.cpp` — remove debug logging
- `engine/platform/backends/glfw/glfw_window.cpp` — remove stbi flip

### Phase 2:
- `engine/scene/yaml_conversions.h` — DELETE
- `engine/scene/yaml.h` — simplify, remove convert<> specializations
- All files that include `yaml_conversions.h` — update include path

### Phase 3:
- `engine/assets/model_data.h` — replace cereal
- `engine/graphics/api/model_data.h` — replace cereal
- `engine/assets/loaders/model_loader.cpp` — replace cereal
- `cmake/external/cereal.cmake` — DELETE
- `cmake/Dependencies.cmake` — remove cereal

### Phase 4:
- `thirdparty/magic_enum/` — new submodule
- `cmake/external/magic_enum.cmake` — new cmake config
- Various component files — use magic_enum

### Phase 5:
- `cmake/external/protobuf.cmake` — DELETE
- `cmake/external/gamenetworkingsockets.cmake` — DELETE
- `cmake/Dependencies.cmake` — remove references

### Phase 6:
- `engine/graphics/ui/ui_data_components.h` — add comments
- `engine/graphics/ui/ui_factory.cpp` — remove unregistered types

### Phase 7:
- `engine/scene/component_registry.cpp` — custom serializer
- `engine/scene/component_registry.h` — update registration
- `engine/scene/component_serializer.cpp` — update deserialize path
- `engine/scene/components/control_component.h` — add helper methods

### Phase 8:
- `engine/reflection/reflection.h` — deduplicate Property()
