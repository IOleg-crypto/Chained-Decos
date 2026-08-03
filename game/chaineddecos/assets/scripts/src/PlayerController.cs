using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class PlayerController : Script
    {
        private float MovementSpeed => Entity.GetComponent<PlayerComponent>()?.MovementSpeed ?? 15.0f;
        private float JumpForce    => Entity.GetComponent<PlayerComponent>()?.JumpForce ?? 15.0f;
        public  float Gravity;  
        public  string MenuScene   = "scenes/start_menu.chscene";

        public int IdleAnim = 0;
        public int RunAnim = 1;
        public int JumpAnim = 2;
        public float CrossFadeTime = 8.5f;

        private RigidBodyComponent? _rb;
        private TransformComponent? _transform;
        private AnimationComponent? _anim;



        public override void OnCreate()
        {
            Gravity = Physics.GetGravity();
            _rb        = Entity.GetComponent<RigidBodyComponent>();
            _transform = Entity.GetComponent<TransformComponent>();
            _anim      = Entity.GetComponent<AnimationComponent>();
            Log.Info("PlayerController initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
            if (Input.IsKeyPressed(Key.Escape))
            {
                Log.Info($"Returning to menu: {MenuScene}");
                Scene.LoadScene(MenuScene);
                return;
            }

            if (_rb == null || _transform == null) return;

            float speed = MovementSpeed;
            if (Input.IsKeyDown(Key.LeftShift)) speed *= 2.0f;

            // Camera-relative basis
            var (forward, right) = GetCameraBasis();

            // Input
            var movementDir = GetMovementInput(forward, right);

            // Movement & rotation
            if (movementDir.LengthSquared() > 0.0001f)
            {
                movementDir = Vector3.Normalize(movementDir);
                ApplyHorizontalMovement(movementDir, speed);
                RotateTowardsMovement(movementDir);
            }
            else
            {
                StopHorizontalMovement();
            }

            HandleVerticalMovement(deltaTime);

            // Animation
            if (_anim != null) UpdateAnimation(movementDir);
        }

        private (Vector3 forward, Vector3 right) GetCameraBasis()
        {
            // Default: camera behind player along +Z axis, looking along -Z
            var forward = new Vector3(0.0f, 0.0f, -1.0f);
            var right   = new Vector3(1.0f, 0.0f, 0.0f);

            var camEntity = Scene.GetMainCamera();
            if (camEntity != null)
            {
                var camera = camEntity.GetComponent<CameraComponent>();
                if (camera != null)
                {
                    // Camera.Forward = direction camera looks (towards player) = rotation*(0,0,-1)
                    // Flatten to XZ plane to get movement-relative forward
                    var camFwd = new Vector3(camera.Forward.X, 0.0f, camera.Forward.Z);
                    if (camFwd.LengthSquared() > 0.0001f)
                    {
                        forward = Vector3.Normalize(camFwd);
                        // Rotate forward 90° clockwise in XZ to get right: (-fwd.Z, 0, fwd.X)
                        // Verified: fwd=(0,0,-1) -> right=(1,0,0) ✓
                        right = Vector3.Normalize(new Vector3(-forward.Z, 0.0f, forward.X));
                    }
                }
            }
            return (forward, right);
        }

        private Vector3 GetMovementInput(Vector3 forward, Vector3 right)
        {
            var dir = Vector3.Zero;
            if (Input.IsKeyDown(Key.W)) dir += forward;
            if (Input.IsKeyDown(Key.S)) dir -= forward;
            if (Input.IsKeyDown(Key.A)) dir -= right;
            if (Input.IsKeyDown(Key.D)) dir += right;
            return dir;
        }

        private void ApplyHorizontalMovement(Vector3 dir, float speed)
        {
            if (_rb == null) return;
            _rb.Velocity = new Vector3(
                dir.X * speed,
                _rb.Velocity.Y,
                dir.Z * speed
            );
        }

        private void StopHorizontalMovement()
        {
            if (_rb == null) return;
            _rb.Velocity = new Vector3(0, _rb.Velocity.Y, 0);
        }

        private void RotateTowardsMovement(Vector3 dir)
        {
            if (_transform == null) return;
            float yaw = Mathf.Atan2(dir.X, dir.Z);
            _transform.Rotation = new Vector3(0, yaw, 0);
        }

        private void HandleVerticalMovement(float deltaTime)
        {
            if (_rb == null || _transform == null) return;

            if (_rb.IsKinematic)
            {
                const float terminalVelocity = -50.0f;
                if (!_rb.IsGrounded) _rb.Velocity = new Vector3(_rb.Velocity.X, _rb.Velocity.Y - Gravity * deltaTime, _rb.Velocity.Z);
                if (_rb.Velocity.Y < terminalVelocity) _rb.Velocity = new Vector3(_rb.Velocity.X, terminalVelocity, _rb.Velocity.Z);
                if (_rb.IsGrounded && _rb.Velocity.Y < 0) _rb.Velocity = new Vector3(_rb.Velocity.X, 0, _rb.Velocity.Z);

                _transform.Translation += _rb.Velocity * deltaTime;
            }
            else
            {
                if (Input.IsKeyPressed(Key.Space) && _rb.IsGrounded)
                {
                    _rb.Velocity = new Vector3(_rb.Velocity.X, JumpForce, _rb.Velocity.Z);
                }
                else
                {
                    _rb.Velocity = new Vector3(_rb.Velocity.X, 0, _rb.Velocity.Z);
                }
            }
        }

        private int _animLogCounter = 0;

        private void UpdateAnimation(Vector3 movementDir)
        {
            if (_anim == null) { Log.Warn("[PlayerController] _anim is null!"); return; }

            bool isMoving = movementDir.LengthSquared() > 0.0001f;
            bool isSprinting = Input.IsKeyDown(Key.LeftShift);
            float speed = isMoving ? (isSprinting ? 1.0f : 0.5f) : 0.0f;
            bool isGrounded = _rb?.IsGrounded ?? true;

            _anim.IsPlaying = true;

            _anim.SetFloat("speed", speed);
            _anim.SetBool("isMoving", isMoving);
            _anim.SetBool("isGrounded", isGrounded);

            if (_animLogCounter < 10)
            {
                Log.Info($"[PlayerController] UpdateAnimation #{_animLogCounter}: " +
                    $"movementDir={movementDir.X:F3},{movementDir.Y:F3},{movementDir.Z:F3} " +
                    $"isMoving={isMoving} speed={speed:F2} isGrounded={isGrounded} " +
                    $"isSprinting={isSprinting} rb={(_rb != null ? "ok" : "null")}");
                _animLogCounter++;
            }
        }
    }
}