# Architecture

Chained Decos follows a Hazel-inspired layered structure. The goal is to keep the codebase easy to navigate: each layer has a clear responsibility, and global services stay narrow instead of becoming catch-all managers.

## Core Layers

- engine/: rendering, scene, physics, audio, assets, platform abstractions.
- editor/: authoring tools, panels, scene inspection, play-mode workflow.
- runtime/: lightweight executable that loads and runs a project without the editor UI.
- scripting/: C++/C# bridge, managed assembly hosting, glue registration, and scripting runtime helpers.
- game/chaineddecos/: project-specific gameplay content and managed scripts.
- tests/: native and managed validation coverage.

## Design Rules

- Keep Application as an orchestrator, not a monolith.
- Prefer small facades for process-wide services when they are infrastructure, not gameplay logic.
- Move policy and lifecycle concerns out of hot paths when they start to grow.
- Prefer the smallest change that improves clarity.
- Do not replace direct control flow with abstraction unless the branching logic is genuinely repeated or hard to maintain.

## Where To Look First

- engine/core/application.h and engine/core/application.cpp for app orchestration.
- engine/core/assets/ for asset lifecycle and loading.
- engine/scene/ for entity, scene, and serialization flow.
- editor/ for UI panels and editor-specific workflows.
- scripting/ for native-managed runtime integration.

## Documentation Boundaries

Architecture notes belong here when they explain ownership, lifecycle, or system boundaries.
Implementation details, troubleshooting steps, and build commands belong in the more focused docs pages.