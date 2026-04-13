# Chained Engine Architecture

This document provides a deep dive into the internal structure of Chained Engine, from the entry point to the main loop.

## 1. Bootstrapping Flow

The engine uses a modular bootstrapping system centered around the `ProjectLauncher` class. This allows the engine to be initialized in different modes (Editor, Runtime, or Headless) without duplicating initialization logic.

### Entry Point (`main`)
The `main` function (found in `entry_point.h`) is minimalist. It delegages application creation to a project-specific `CreateApplication` function.

### ProjectLauncher
The `ProjectLauncher` utility is responsible for:
1.  **Parsing Command Line Arguments**: Determining the project path and window overrides.
2.  **Loading Project Metadata**: Reading the `.chproject` YAML file.
3.  **Preparing Application Specifications**: Setting VSync, window dimensions, and titles based on project data.

## 2. System Initialization (SRP)

We adhere to the **Single Responsibility Principle**. Instead of a monolithic initialization block, each engine system is responsible for its own setup.

### Decentralized Asset Loaders
Asset loaders are registered during the `Init()` phase of the relevant subsystem:
*   **Renderer**: Registers `TextureLoader`, `ModelLoader`, `ShaderLoader`, and `EnvironmentLoader`.
*   **UIRenderer**: Registers `FontLoader`.

## 3. Layer Stack Model

The engine handles functionality through a `LayerStack`. Layers are processed in the following order:
1.  **Engine Layers** (e.g., `RuntimeLayer` or `EditorLayer`): Handle the main logic and viewport rendering.
2.  **Overlays** (e.g., `ImGuiLayer`): Render UI and debugging information on top of the layers.

## 4. Main Loop

The `Application::Run()` method is the heart of the engine:
1.  **Timing**: Calculates `Timestep` (delta time).
2.  **Updates**: Iterates through the `LayerStack`, calling `OnUpdate` for each layer.
3.  **UI Rendering**: Calls `Begin()` on `ImGuiLayer`, then `OnImGuiRender` for all layers, and finally `End()`.
4.  **Events**: Dispatches platform events (input, window resize) through the stack.
