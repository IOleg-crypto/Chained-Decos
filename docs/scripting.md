# Scripting

Gameplay scripting is managed C# hosted through Coral and CoreCLR. The native engine owns the runtime integration, while managed code owns gameplay behavior.

## Main Pieces

- scripting/scriptengine.h and scripting/scriptengine.cpp for the public script-engine facade.
- scripting/scriptengine_services.h and scripting/scriptengine_services.cpp for the internal host, type registry, and runtime-session helpers.
- scripting/script_glue*.cpp for native-to-managed internal calls.
- scripting/managed/src/ for the managed public API surface.
- game/chaineddecos/ for gameplay-side scripts and content.

## Lifecycle Overview

1. The native side loads the managed assembly.
2. Script types are discovered and registered.
3. Glue bindings connect engine functionality to managed entry points.
4. The active scene enters play mode and script update callbacks run.
5. Reload and runtime state are handled by the script-engine session layer.

## Commenting Guidance

- Document contracts, ownership, lifecycle, and side effects.
- Prefer short comments that explain why something exists, not comments that repeat the code.
- Keep interop helpers explicit when the native and managed sides need a contract that is not obvious from the signature.

## Good Places For Notes

- Scene and entity access rules.
- Managed wrapper responsibilities.
- Reload behavior and runtime state transitions.
- Unsafe interop helpers and the assumptions they rely on.