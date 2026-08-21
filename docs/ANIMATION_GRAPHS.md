# Animation Graph Tutorial

Chained Engine features a visual, data-driven Animation Graph system (`.chag`) for driving state machine animations with smooth blending.

## Table of Contents

- [Overview & Graph File Format](#1-overview--graph-file-format-chag)
- [Editor Workflow](#2-editor-workflow-animgraphpanel)
- [Driving from C# Scripts](#3-driving-animation-graph-from-c-scripts)
- [Transition Condition Operators](#4-transition-condition-comparison-operators)
- [Runtime vs. Edit Mode](#5-runtime-vs-edit-mode-execution)

---

## 1. Overview & Graph File Format (`.chag`)

Animation Graphs are node-based state machines serialized as YAML. A graph contains:
- **Default Variables**: Declares the schema of parameters (e.g., `speed`, `isMoving`, `isGrounded`).
- **Nodes (`AnimNode`)**: Define animation states, frame ranges, looping, and playback speed.
- **Transitions (`AnimTransition`)**: Define connections between states with blend durations and variable-based conditions.

[![Znimok-ekrana-2026-08-01-173424.png](https://i.postimg.cc/sgHrQcjq/Znimok-ekrana-2026-08-01-173424.png)](https://postimg.cc/FYSqw0JV)

[![Znimok-ekrana-2026-08-01-175555.png](https://i.postimg.cc/bNJ8zW8D/Znimok-ekrana-2026-08-01-175555.png)](https://postimg.cc/kDkLsj2q)

Example `new_graph.chag` structure:

```yaml
AnimationGraph:
  EntryNodeID: 4
  NextNodeID: 5
  Variables:
    speed: 0
    isMoving: 0
    isGrounded: 1
  Nodes:
    - ID: 1
      Name: Walk
      AnimationIndex: 0
      IsLooping: true
      StartFrame: 690
      EndFrame: 780
      Speed: 1.0
    - ID: 2
      Name: Run
      AnimationIndex: 0
      IsLooping: true
      StartFrame: 1450
      EndFrame: 1500
      Speed: 1.0
    - ID: 4
      Name: Stop
      AnimationIndex: 0
      IsLooping: true
      StartFrame: 0
      EndFrame: 0
      Speed: 1.0
  Transitions:
    - ID: 1
      SourceNodeID: 1
      TargetNodeID: 2
      BlendDuration: 0.2
      HasExitTime: false
      Conditions:
        - VariableName: speed
          Op: 4 # GreaterThan (>)
          Value: 0.9
```

## 2. Editor Workflow (`AnimGraphPanel`)

1. **Open the Animation Graph Panel:** In the editor top menu, open **Panels -> Animation Graph Editor**.
2. **Create/Load Graph:** Click **New Graph** or select an existing `.chag` file.
3. **Configure Nodes:**
   - Add nodes for states like `Idle`/`Stop`, `Walk`, `Run`, `Jump`.
   - Set `StartFrame` and `EndFrame` according to your model asset's animation tracks.
4. **Manage Variables:**
   - Add variables (e.g., `speed`, `isMoving`, `isGrounded`).
   - Use the type toggle in the editor to switch between `Float` and `Bool` parameter types.
5. **Create Transitions & Conditions:**
   - Drag connections between nodes.
   - Select a transition to edit `BlendDuration` (e.g., `0.2s` crossfade) and add conditions (e.g., `isMoving == 1`, `speed >= 0.9`).
6. **Assign to Entity:**
   - Select an entity with an `AnimationComponent`.
   - Set `GraphPath` to `assets/animations/new_graph.chag`.

## 3. Driving Animation Graph from C# Scripts

C# gameplay scripts pass parameters directly to `AnimationComponent`. The engine automatically seeds graph variables on entity initialization and updates state transitions at runtime during simulation.

```csharp
using Chained;

public class PlayerController : Script
{
    private AnimationComponent? m_Anim;

    public override void OnCreate()
    {
        m_Anim = GetComponent<AnimationComponent>();
    }

    public override void OnUpdate(float ts)
    {
        Vector3 movement = GetInputVector();
        bool isMoving = movement.LengthSquared() > 0.01f;
        bool isSprinting = Input.IsKeyDown(Key.LeftShift);

        float speed = 0.0f;
        if (isMoving)
        {
            speed = isSprinting ? 1.0f : 0.5f;
        }

        // Drive graph variables dynamically from script
        m_Anim?.SetBool("isMoving", isMoving);
        m_Anim?.SetFloat("speed", speed);
        m_Anim?.SetBool("isGrounded", IsGrounded());
    }
}
```

## 4. Transition Condition Comparison Operators

| Op Code | Operator | Description |
| :--- | :--- | :--- |
| `0` | `==` | Equal |
| `1` | `!=` | Not Equal |
| `2` | `<` | Less Than |
| `3` | `<=` | Less Than or Equal |
| `4` | `>` | Greater Than |
| `5` | `>=` | Greater Than or Equal |

## 5. Runtime vs. Edit Mode Execution

- **Simulation Mode (`Play`):** C# scripts update `AnimationComponent.Variables`, and `AnimationSystem` evaluates transitions frame-by-frame with smooth interpolation.
- **Edit Mode:** Animation transitions are paused to keep the editor viewport stable and prevent state jitter while authoring.
