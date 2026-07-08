using System;
using Chained;

namespace ChainedDecos.Scripts
{
public class CameraController : Script
{
    public float LookSensitivity = 0.5f;
    public float Distance = 10.0f;
    public float Pitch = 45.0f;
    public float Yaw = 0.0f;
    public string TargetTag = "Player";

    public override void OnCreate()
    {
        CameraComponent? camera = Entity.GetComponent<CameraComponent>();
        if (camera == null)
        {
            Log.Error($"[CameraController] FAILED: Entity '{Entity}' has no CameraComponent!");
            return;
        }

        // One-time setup: make this the primary orbit camera
        if (!camera.Primary)
        {
            Log.Warn($"[CameraController] Setting '{Entity}' as Primary camera.");
            camera.Primary = true;
        }

        camera.IsOrbitCamera = true;
        camera.TargetEntityTag = "Player";
        camera.SetOrbit(Yaw, Pitch, Distance);
    }

    public override void OnUpdate(float deltaTime)
    {
        CameraComponent? camera = Entity.GetComponent<CameraComponent>();
        if (camera == null)
            return;

        Entity? player = Scene.FindEntityByTag("Player");
        if (player == null)
            return;

        camera.GetOrbit(out float yaw, out float pitch, out float distance);

        // Handle Input (Orbit & Zoom)
        if (Input.IsMouseButtonDown(MouseButton.Right))
        {
            Vector3 mouseDelta = Input.MouseDelta;
            yaw -= mouseDelta.X * LookSensitivity;
            pitch -= mouseDelta.Y * LookSensitivity;

            // Clamp pitch
            pitch = Mathf.Clamp(pitch, -10.0f, 85.0f);
        }

        float wheel = Input.GetMouseWheelMove();
        distance -= wheel * 2.0f;
        distance = Mathf.Clamp(distance, 0.0f, 40.0f);

        camera.SetOrbit(yaw, pitch, distance);
    }
}
}
