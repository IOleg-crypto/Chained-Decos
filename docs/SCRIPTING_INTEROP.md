# Scripting Interop & Roslyn Source Generator Architecture

This document describes how the C++/C# interop bridge works in Chained Engine, how the Roslyn Source Generator (`Chained.Managed.Generator`) automates native bindings, and how to expose new native C++ component functions to C# gameplay scripts.

---

## 1. Overview & Architecture

Chained Engine uses **[Coral](https://github.com/StudioCherno/Coral)** — a C++ wrapper around .NET CoreCLR — for C++/C# interoperability.

```
┌────────────────────────────────┐         ┌────────────────────────────────┐
│      Native C++ Engine         │         │       Managed C# Scripts       │
│                                │         │                                │
│  script_glue_*.cpp             │         │  src/Components/*.cs           │
│  extern "C" C++ functions      │         │  [NativeProperty] /            │
│       │                        │         │  [NativeCall] attributes       │
│       │                        │         │       │                        │
│       │ (Function Pointers)    │         │       │ (Roslyn Code Gen)      │
│       ▼                        │         │       ▼                        │
│  Coral::Assembly               │         │  .g.cs (Generated File)        │
│  AddInternalCall("Class",      │ ═══════>│  internal static unsafe        │
│    "Method_Ptr", &C++Fn)       │ Writes  │    delegate* unmanaged<...>   │
│  UploadInternalCalls()         │ Pointer │    Method_Ptr;                 │
└────────────────────────────────┘         └────────────────────────────────┘
```

### High-level data flow:
1. **C++ Glue Functions**: Declared as `extern "C"` (using macro `CH_SCRIPT_FUNC`).
2. **C++ Registration**: `ScriptGlue::RegisterInternalCalls()` binds C++ function pointers to string names (`ClassName.MethodName_Ptr`).
3. **C# Roslyn Generator**: `NativeCallGenerator` scans `[NativeCall]` and `[NativeProperty]` attributes at compile time and auto-generates `delegate* unmanaged<...>` function pointer fields and C# property getters/setters.
4. **Coral Binding**: At assembly load time, Coral matches string names against static `_Ptr` fields in C# assemblies and writes C++ function pointers directly into those fields.

---

## 2. Roslyn Source Generator (`Chained.Managed.Generator`)

The generator project lives in `scripting/managed/Chained.Managed.Generator/` and compiles to a Roslyn analyzer DLL (`Chained.Managed.Generator.dll`).

### Attributes

#### `[NativeCall]`
Declares a single native function binding.
```csharp
[NativeCall("Chained.AnimationComponent", "AnimationComponent_CrossFade", "void", "ulong", "int", "float")]
public partial class AnimationComponent : Component { ... }
```
- **Signature format**: `[returnType, param1, param2, ...]`
- **First parameter**: Always entity ID (`ulong`) for component accessors.
- **Auto-generates**:
  ```csharp
  internal static unsafe delegate* unmanaged<ulong, int, float, void> AnimationComponent_CrossFade_Ptr;
  ```

#### `[NativeProperty]`
Declares a full C# property getter/setter **AND** generates the corresponding `_Ptr` fields.
```csharp
[NativeProperty("MovementSpeed", "float", "PlayerComponent_GetMovementSpeed", "PlayerComponent_SetMovementSpeed")]
[NativeProperty("IsKinematic", "bool", "RigidBody_IsKinematic", "RigidBody_SetKinematic")]
[NativeProperty("Translation", "Vector3", "Transform_GetTranslation", "Transform_SetTranslation")]
public partial class PlayerComponent : Component { ... }
```
- **Auto-generates**:
  1. Both `Get_Ptr` and `Set_Ptr` unmanaged function pointer fields.
  2. The full C# property getter/setter with null-checks, `unsafe` blocks, and type marshaling (`bool` $\leftrightarrow$ `byte`, `Vector3*` out-pointer).

---

## 3. Type Mapping Convention (ABI)

The C++ glue functions and C# generator follow strict ABI type mapping rules:

| Logical Type | C++ Type (`script_glue_*.cpp`) | C# Attribute String | Generated C# Type / Pointer |
|---|---|---|---|
| Entity ID | `uint64_t` | `"ulong"` | `ulong` |
| Boolean | `uint8_t` | `"bool"` / `"byte"` | `byte` (`(byte)(value ? 1 : 0)`) |
| Integer | `int32_t` / `int` | `"int"` | `int` |
| Unsigned Int | `uint32_t` | `"uint"` | `uint` |
| Float | `float` | `"float"` | `float` |
| Double | `double` | `"double"` | `double` |
| UTF-16 String | `Coral::UCChar*` / `char16_t*` | `"char*"` | `char*` |
| Vector2 Struct | `glm::vec2*` | `"Vector2"` / `"Vector2*"` | `Chained.Vector2*` (out-pointer) |
| Vector3 Struct | `glm::vec3*` | `"Vector3"` / `"Vector3*"` | `Chained.Vector3*` (out-pointer) |
| Vector4 Struct | `glm::vec4*` | `"Vector4"` / `"Vector4*"` | `Chained.Vector4*` (out-pointer) |

> ⚠️ **Important**: Structs (`Vector2`, `Vector3`, `Vector4`) are **always passed by pointer** across the ABI boundary to ensure stack alignment and prevent platform ABI discrepancies.

---

## 4. Glue Generation Pipeline

For simple property get/set, you don't need to write any C++ glue code. The `tools/generate_glue.py` script scans C# `[NativeProperty]` attributes and generates everything automatically.

### How it works

```
C# [NativeProperty]          Python generator              C++ build
    attribute          ───>   tools/generate_glue.py   ───>   auto-compiled
                                                       │
CMakeLists.txt --classes ──────────────────────────────┘
```

1. **`tools/generate_glue.py`** reads `engine/scripting/managed/src/Components/*.cs`
2. Filters by `--classes` parameter (e.g. `PlayerComponent SpawnComponent`)
3. Generates three files in `engine/scripting/generated/`:
   - `script_glue_generated.h` — `CH_SCRIPT_FUNC` declarations
   - `script_glue_generated.cpp` — getter/setter implementations
   - `script_glue_generated_reg.cpp` — `AddInternalCall` registrations (included by `script_glue.cpp`)
4. CMake custom command runs the generator automatically when C# sources change

### Usage

Add your class to the `--classes` list in `engine/scripting/CMakeLists.txt`:

```cmake
COMMAND Python3::Interpreter "${GLUE_GENERATOR_SCRIPT}"
        --cs-dir "${MANAGED_PROJECT_DIR}/src/Components"
        --output-dir "${GLUE_OUTPUT_DIR}"
        --classes PlayerComponent SpawnComponent NetworkIdentityComponent YourComponent
```

Then build normally — the generator runs as part of the build.

### Limitations

The generator handles **simple field access only**: reading/writing a single field on a component. It does NOT handle:
- Physics synchronization (e.g. updating Jolt body when translation changes)
- String conversion (e.g. `Coral::UCChar*` ↔ `std::string`)
- Complex logic (e.g. finding entities by tag, conditional behavior)

For these cases, use `[NativeCall]` and write hand-written glue in `script_glue_*.cpp` — see the manual tutorial below.

---

## 5. Step-by-Step Tutorial: Adding a New Native Property

### Automatic (recommended for simple properties)

Suppose you want to expose a new `Stamina` property on `PlayerComponent`.

**Step 1: Add `[NativeProperty]` to C# Component**

In `scripting/managed/src/Components/PlayerComponent.cs`:

```csharp
namespace Chained
{
    [NativeProperty("MovementSpeed", "float", "PlayerComponent_GetMovementSpeed", "PlayerComponent_SetMovementSpeed")]
    [NativeProperty("Stamina", "float", "PlayerComponent_GetStamina", "PlayerComponent_SetStamina")]
    public partial class PlayerComponent : Component
    {
    }
}
```

**Step 2: Register the class** in `engine/scripting/CMakeLists.txt`:

```cmake
--classes PlayerComponent SpawnComponent NetworkIdentityComponent
```

**Step 3: Build**:

```bash
cmake --build --preset windows-clang-debug --parallel
```

Done. The generator creates the C++ getter, setter, and registration automatically. The Roslyn Source Generator on the C# side creates the `_Ptr` fields and property body.

### Manual (for complex glue logic)

If your property needs physics sync, string conversion, or other complex behavior, write the C++ glue by hand.

**Step 1: Implement C++ Glue Function**

In `scripting/script_glue_player.cpp`:

```cpp
CH_SCRIPT_FUNC float PlayerComponent_GetStamina(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<PlayerComponent>())
        return entity.GetComponent<PlayerComponent>().Stamina;
    return 0.0f;
}

CH_SCRIPT_FUNC void PlayerComponent_SetStamina(uint64_t entityID, float stamina)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<PlayerComponent>())
        entity.GetComponent<PlayerComponent>().Stamina = stamina;
}
```

**Step 2: Register in `script_glue.cpp`**

In `scripting/script_glue.cpp` under `ScriptGlue::RegisterInternalCalls()`:

```cpp
assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_GetStamina_Ptr", (void*)&PlayerComponent_GetStamina);
assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_SetStamina_Ptr", (void*)&PlayerComponent_SetStamina);
```

**Step 3: Declare in header**

In `scripting/script_glue_entity.h` (or the relevant `script_glue_*.h`):

```cpp
CH_SCRIPT_FUNC float PlayerComponent_GetStamina(uint64_t entityID);
CH_SCRIPT_FUNC void PlayerComponent_SetStamina(uint64_t entityID, float stamina);
```

**Step 4: Add `[NativeProperty]` to C# Component**

Same as the automatic path — the C# Roslyn generator needs the attribute to create the `_Ptr` fields and property body.

---

## 6. Troubleshooting & Generated Files

- **Viewing Generated Code**: During compilation with MSBuild (`/p:EmitCompilerGeneratedFiles=true`), generated C# files are saved under `scripting/managed/obj/GeneratedFiles/Chained.Managed.Generator/`.
- **Component Must Be `partial`**: Any C# class decorated with `[NativeCall]` or `[NativeProperty]` **must** have the `partial` keyword.
- **Null Safety**: All generated properties contain defensive `!= null` checks on the `_Ptr` fields. If a function is not registered on the C++ side, the property getter will return `default` instead of crashing with a `NullReferenceException`.
