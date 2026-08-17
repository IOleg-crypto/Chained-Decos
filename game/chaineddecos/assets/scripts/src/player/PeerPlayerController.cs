using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Lightweight controller for peer avatars spawned from player.chprefab.
    /// Only handles animation updates based on replicated velocity — no input.
    /// </summary>
    public class PeerPlayerController : Script
    {
        private RigidBodyComponent? _rb;
        private AnimationComponent? _anim;

        public override void OnCreate()
        {
            _rb = Entity.GetComponent<RigidBodyComponent>();
            _anim = Entity.GetComponent<AnimationComponent>();
        }

        public override void OnUpdate(float deltaTime)
        {
            if (_rb == null || _anim == null) return;

            // Animation driven by replicated velocity from host
            bool isMoving = _rb.Velocity.LengthSquared() > 0.1f;
            float speed = isMoving ? 0.5f : 0.0f;
            bool isGrounded = _rb.IsGrounded;

            _anim.SetBool("isMoving", isMoving);
            _anim.SetBool("isGrounded", isGrounded);
            _anim.SetFloat("speed", speed);
        }
    }
}
