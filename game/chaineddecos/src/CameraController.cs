using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class CameraController : Script
{
    public const float LookSensitivity = 0.5f;

    public override void OnUpdate(float deltaTime)
    {
        CameraComponent? camera = Entity.GetComponent<CameraComponent>();
        if (camera == null)
        {
            Log.Error($"[CameraController] FAILED: Attached to Entity '{Entity}' which has NO CameraComponent. CameraController must be on the Camera entity!");
            return;
        }

        // Auto-configure if needed
        if (!camera.Primary)
        {
             Log.Warn($"[CameraController] Camera on '{Entity}' was not Primary. Setting it to Primary now.");
             camera.Primary = true;
        }

        Entity? player = Scene.FindEntityByTag("Player");
        if (player == null)
        {
            Log.Warn("[CameraController] Player entity (tagged 'Player') not found. Camera will not orbit correctly.");
            return;
        }
        
        // Ensure orbit is enabled
        if (!camera.IsOrbitCamera || camera.TargetEntityTag != "Player")
        {
            Log.Info("[CameraController] Correcting Camera orbit settings...");
            camera.IsOrbitCamera = true;
            camera.TargetEntityTag = "Player";
        }

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
