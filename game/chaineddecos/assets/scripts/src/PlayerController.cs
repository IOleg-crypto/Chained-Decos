using System;
// Build trigger test 2
using Chained;

namespace ChainedDecos.Scripts
{
    public class PlayerController : Script
    {
        private float MovementSpeed => Entity.GetComponent<PlayerComponent>()?.MovementSpeed ?? 15.0f;
        private float JumpForce    => Entity.GetComponent<PlayerComponent>()?.JumpForce ?? 15.0f;
        public  float Gravity;  
        public  string MenuScene   = "scenes/start_menu.chscene";

        // Animation Indices
        public int IdleAnim = 0;
        public int RunAnim = 1;
        public int JumpAnim = 2;
        public float CrossFadeTime = 0.2f;

        private enum PlayerState
        {
            None,
            Idle,
            Running,
            Jumping,
            Falling
        }

        private PlayerState _currentState = PlayerState.None;

        public override void OnCreate()
        {
            Log.Info("C# PlayerController initialized!");

            // Read gravity from the engine's project configuration so it always
            // matches the value set in Project Settings -> Physics -> World Gravity.
            Gravity = Physics.GetGravity();
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
            Vector3 forward = new Vector3(0.0f, 0.0f, -1.0f);
            Vector3 right   = new Vector3(1.0f, 0.0f, 0.0f);

            Entity? camEntity = Scene.GetMainCamera();
            if (camEntity != null)
            {
                CameraComponent? camera = camEntity.GetComponent<CameraComponent>();
                if (camera != null)
                {
                    Vector3 camFwd = new Vector3(camera.Forward.X, 0.0f, camera.Forward.Z);
                    Vector3 camRight = new Vector3(camera.Right.X, 0.0f, camera.Right.Z);

                    if (camFwd.LengthSquared() > 0.0001f)
                        forward = Vector3.Normalize(camFwd);

                    if (camRight.LengthSquared() > 0.0001f)
                        right = Vector3.Normalize(camRight);
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

            // --- Застосування швидкості ---
            if (isKinematic)
            {
                const float kTerminalVelocity = -50.0f;

                // Apply gravity only when not grounded
                if (!rb.IsGrounded)
                    velocity.Y -= Gravity * deltaTime;

                // Terminal velocity cap — prevents extreme penetration
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
                
                // Для кінематики ми повністю контролюємо весь вектор
                rb.Velocity = velocity; 
            }
            else
            {
                bool wantsJump = Input.IsKeyPressed(Key.Space);

                if (wantsJump && rb.IsGrounded)
                {
                    rb.Velocity = new Vector3(velocity.X, effectiveJumpForce, velocity.Z);
                }
                else
                {
                    // Тільки горизонталь — Y залишається за Jolt
                    rb.Velocity = new Vector3(velocity.X, 0.0f, velocity.Z);
                }
            }

            // --- Animation State Machine ---
            UpdateAnimation(rb, movementDir);
        }

        private void UpdateAnimation(RigidBodyComponent rb, Vector3 movementDir)
        {
            // If we have an AnimationComponent with a graph, feed variables to it.
            AnimationComponent? anim = Entity.GetComponent<AnimationComponent>();
            if (anim != null)
            {
                bool isSprinting = Input.IsKeyDown(Key.LeftShift);
                bool isMoving = movementDir.LengthSquared() > 0.0001f;
                float speedVal = 0.0f;
                if (isMoving)
                {
                    speedVal = isSprinting ? 1.0f : 0.5f;
                }

                // Feed speed, isMoving, and isGrounded to graph variables
                anim.SetFloat("speed", speedVal);
                anim.SetFloat("Speed", speedVal);
                anim.SetBool("isMoving", isMoving);
                anim.SetBool("IsMoving", isMoving);
                anim.SetBool("IsGrounded", rb.IsGrounded);
                anim.SetBool("isGrounded", rb.IsGrounded);
            }

            // Fallback for when no graph is attached (manual C# state machine)
            if (anim == null) return;

            PlayerState newState = _currentState;

            if (rb.IsGrounded)
            {
                if (movementDir.LengthSquared() > 0.0001f)
                    newState = PlayerState.Running;
                else
                    newState = PlayerState.Idle;
            }
            else
            {
                // In air
                if (rb.Velocity.Y > 0)
                    newState = PlayerState.Jumping;
                else
                    newState = PlayerState.Falling;
            }

            if (newState != _currentState)
            {
                _currentState = newState;
                switch (newState)
                {
                    case PlayerState.Idle:
                        anim.CrossFade(IdleAnim, CrossFadeTime);
                        break;
                    case PlayerState.Running:
                        anim.CrossFade(RunAnim, CrossFadeTime);
                        break;
                    case PlayerState.Jumping:
                    case PlayerState.Falling:
                        anim.CrossFade(JumpAnim, CrossFadeTime);
                        break;
                }
            }
        }
    }
}