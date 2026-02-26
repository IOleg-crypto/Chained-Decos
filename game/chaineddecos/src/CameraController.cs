using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class CameraController : Script
{
    public const float LookSensitivity = 0.5f;

    public override void OnUpdate(float deltaTime)
    {
        Log.Info($"[Camera] OnUpdate ticked for Entity {Entity.ID}");
        
        CameraComponent? camera = Entity.GetComponent<CameraComponent>();
        if (camera == null)
        {
            Log.Info($"[Camera] ERROR: No CameraComponent found on Entity {Entity.ID}");
            return;
        }

        Entity? player = Scene.FindEntityByTag("Player");
        if (player == null)
        {
            Log.Info("[Camera] ERROR: Player entity not found (FindEntityByTag returned null)");
            return;
        }
        
        Log.Info($"[Camera] Camera and Player found. Camera Entity: {Entity.ID}, Player Entity: {player.ID}");

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
