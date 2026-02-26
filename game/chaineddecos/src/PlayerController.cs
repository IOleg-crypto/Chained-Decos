using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class PlayerController : Script
{
    public float MovementSpeed = 15.0f;
    public float JumpForce = 10.0f;

    public override void OnCreate()
    {
        Log.Info("C# PlayerController initialized!");
    }

    public override void OnUpdate(float deltaTime)
    {
        // Debug Log to ensure OnUpdate is running
        if (Input.IsKeyPressed(Key.T)) Log.Info("C# PlayerController: T key pressed!");

        float currentSpeed = MovementSpeed;
        if (Input.IsKeyDown(Key.LeftShift))
            currentSpeed *= 2.0f;

        // Get movement directions relative to camera
        Vector3 forward = Vector3.Zero;
        Vector3 right = Vector3.Zero;

        Entity? camEntity = Scene.GetMainCamera();

        if (camEntity != null)
        {
            CameraComponent? camera = camEntity.GetComponent<CameraComponent>();
            if (camera != null)
            {
                Vector3 camForward = camera.Forward;
                Vector3 camRight = camera.Right;

                // Flatten for ground movement (XZ Plane)
                forward = Vector3.Normalize(new Vector3(camForward.X, 0.0f, camForward.Z));
                right = Vector3.Normalize(new Vector3(camRight.X, 0.0f, camRight.Z));
            }
            else
            {
                 Log.Info("[PlayerController] Found Camera Entity, but no CameraComponent attached.");
            }
        }
        else
        {
             Log.Info("[PlayerController] ERROR: Could not find primary camera!");
        }

        Vector3 movementVector = Vector3.Zero;

        if (Input.IsKeyDown(Key.W)) movementVector += forward;
        if (Input.IsKeyDown(Key.S)) movementVector -= forward;
        if (Input.IsKeyDown(Key.A)) movementVector -= right;
        if (Input.IsKeyDown(Key.D)) movementVector += right;

        RigidBodyComponent? rb = Entity.GetComponent<RigidBodyComponent>();
        TransformComponent? transform = Entity.GetComponent<TransformComponent>();

        if (rb != null && transform != null)
        {
            if (movementVector.LengthSquared() > 0.0001f)
            {
                movementVector = Vector3.Normalize(movementVector);

                Vector3 velocity = rb.Velocity;
                velocity.X = movementVector.X * currentSpeed;
                velocity.Z = movementVector.Z * currentSpeed;
                rb.Velocity = velocity;

                // Simple look-at rotation (Y axis)
                float targetYaw = Mathf.Atan2(movementVector.X, movementVector.Z);
                Vector3 rot = transform.Rotation;
                rot.Y = targetYaw * Mathf.Rad2Deg;
                transform.Rotation = rot;
            }
            else
            {
                Vector3 velocity = rb.Velocity;
                velocity.X = 0;
                velocity.Z = 0;
                rb.Velocity = velocity;
            }

            if (Input.IsKeyPressed(Key.Space) && rb.IsGrounded)
            {
                Vector3 velocity = rb.Velocity;
                velocity.Y = JumpForce;
                rb.Velocity = velocity;
                Log.Info("C# Jump triggered!");
            }
        }

        if (Input.IsKeyPressed(Key.T))
        {
            Entity? spawnZone = Scene.FindEntityByTag("SpawnPoint");
            if (spawnZone != null && transform != null)
            {
                transform.Translation = spawnZone.GetComponent<TransformComponent>().Translation;
                Log.Info("Teleported to spawn via C#!");
            }
        }
    }
}
}
