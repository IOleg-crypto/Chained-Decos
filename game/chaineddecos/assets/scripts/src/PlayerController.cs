using System;
// Build trigger test 2
using Chained;

namespace ChainedDecos.Scripts
{
public class PlayerController : Script
{
    public float MovementSpeed = 15.0f;
    public float JumpForce    = 15.0f;
    public float Gravity      = 20.0f;   // Units per second^2 (should match physics config)
    public string MenuScene   = "scenes/start_menu.chscene";

    public override void OnCreate()
    {
        Log.Info("C# PlayerController initialized!");

        // Read gravity from the engine's project configuration so it always
        // matches the value set in Project Settings → Physics → World Gravity.
        Gravity = Physics.GetGravity();

        // Read settings from the C++ PlayerComponent if the entity has one.
        // This allows tweaking values from the Inspector without modifying the script.
        PlayerComponent? pc = Entity.GetComponent<PlayerComponent>();
        if (pc != null)
        {
            if (pc.MovementSpeed > 0.0f) MovementSpeed = pc.MovementSpeed;
            if (pc.JumpForce > 0.0f)     JumpForce     = pc.JumpForce;
        }
    }

    public override void OnUpdate(float deltaTime)
    {
        // Allow leaving gameplay back to menu with a single key press.
        if (Input.IsKeyPressed(Key.Escape))
        {
            Log.Info("PlayerController: Escape pressed, returning to menu: " + MenuScene);
            Scene.LoadScene(MenuScene);
            return;
        }

        float currentSpeed = MovementSpeed;
        float effectiveJumpForce = JumpForce;

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
        bool wDown = Input.IsKeyDown(Key.W);
        bool sDown = Input.IsKeyDown(Key.S);
        bool aDown = Input.IsKeyDown(Key.A);
        bool dDown = Input.IsKeyDown(Key.D);
        if (wDown) movementDir += forward;
        if (sDown) movementDir -= forward;
        if (aDown) movementDir -= right;
        if (dDown) movementDir += right;

        RigidBodyComponent?  rb        = Entity.GetComponent<RigidBodyComponent>();
        TransformComponent?  transform = Entity.GetComponent<TransformComponent>();
        if (rb == null || transform == null) { return; }

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
            velocity.Y = effectiveJumpForce;
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
