using System;
// Build trigger test 2
using CHEngine;

namespace ChainedDecos.Scripts
{
public class PlayerController : Script
{
    public float MovementSpeed = 15.0f;
    public float JumpForce    = 10.0f;
    public float Gravity      = 20.0f;   // Units per second^2 (should match physics config)

    public override void OnCreate()
    {
        Log.Info("C# PlayerController initialized!");
    }

    public override void OnUpdate(float deltaTime)
    {
        float currentSpeed = MovementSpeed;
        if (Input.IsKeyDown(Key.LeftShift))
            currentSpeed *= 2.0f;

        // --- Camera-relative directions ---
        Vector3 forward = Vector3.Zero;
        Vector3 right   = Vector3.Zero;

        Entity? camEntity = Scene.GetMainCamera();
        if (camEntity != null)
        {
            CameraComponent? camera = camEntity.GetComponent<CameraComponent>();
            if (camera != null)
            {
                // Flatten for XZ ground movement
                forward = Vector3.Normalize(new Vector3(camera.Forward.X, 0.0f, camera.Forward.Z));
                right   = Vector3.Normalize(new Vector3(camera.Right.X,   0.0f, camera.Right.Z));
            }
        }

        // --- Input ---
        Vector3 movementDir = Vector3.Zero;
        if (Input.IsKeyDown(Key.W)) movementDir += forward;
        if (Input.IsKeyDown(Key.S)) movementDir -= forward;
        if (Input.IsKeyDown(Key.A)) movementDir -= right;
        if (Input.IsKeyDown(Key.D)) movementDir += right;

        RigidBodyComponent?  rb        = Entity.GetComponent<RigidBodyComponent>();
        TransformComponent?  transform = Entity.GetComponent<TransformComponent>();
        if (rb == null || transform == null) return;

        bool isKinematic = rb.IsKinematic;
        Vector3 velocity = rb.Velocity;

        // --- Horizontal movement ---
        if (movementDir.LengthSquared() > 0.0001f)
        {
            movementDir = Vector3.Normalize(movementDir);
            velocity.X = movementDir.X * currentSpeed;
            velocity.Z = movementDir.Z * currentSpeed;

            // Face movement direction (Y-axis only)
            float targetYaw = Mathf.Atan2(movementDir.X, movementDir.Z);
            transform.Rotation = new Vector3(0, targetYaw, 0);
        }
        else
        {
            velocity.X = 0;
            velocity.Z = 0;
        }

        // --- Jump ---
        if (Input.IsKeyPressed(Key.Space) && rb.IsGrounded)
        {
            velocity.Y = JumpForce;
            Log.Info("C# Jump triggered!");
        }

        // Only apply gravity and integration manually if Kinematic.
        // If Dynamic, the physics engine handles these in ResolveSimulation/IntegrateVelocity.
        if (isKinematic)
        {
            // Apply gravity only when not grounded
            if (!rb.IsGrounded)
                velocity.Y -= Gravity * deltaTime;

            // Terminal velocity cap — prevents extreme penetration that multi-pass can't recover
            const float kTerminalVelocity = -50.0f;
            if (velocity.Y < kTerminalVelocity)
                velocity.Y = kTerminalVelocity;

            // Stop downward movement when grounded to prevent creep-through
            if (rb.IsGrounded && velocity.Y < 0.0f)
                velocity.Y = 0.0f;

            // Manual integration for Kinematic bodies
            transform.Translation = new Vector3(
                transform.Translation.X + velocity.X * deltaTime,
                transform.Translation.Y + velocity.Y * deltaTime,
                transform.Translation.Z + velocity.Z * deltaTime
            );
        }
        
        rb.Velocity = velocity;

        // --- Debug teleport ---
        if (Input.IsKeyPressed(Key.T))
        {
            Entity? spawnZone = Scene.FindEntityByTag("SpawnPoint");
            if (spawnZone != null)
            {
                TransformComponent? spawnTransform = spawnZone.GetComponent<TransformComponent>();
                if (spawnTransform != null)
                {
                    transform.Translation = spawnTransform.Translation;
                    Log.Info("Teleported to spawn via C#!");
                }
            }
        }
    }
}
}
