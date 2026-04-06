using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Singleton pattern for managing the game state in high-level scripting.
    /// Manages player score, game time, and global game state.
    /// </summary>
    public class GameManager : Script
    {
        private static GameManager? _instance;
        public static GameManager Instance => _instance ?? throw new InvalidOperationException("GameManager not initialized.");

        public int Score { get; private set; } = 0;
        public float GameTime { get; private set; } = 0.0f;
        public bool IsGameOver { get; private set; } = false;

        public override void OnCreate()
        {
            if (_instance != null && _instance != this)
            {
                Log.Info("Multiple GameManager instances detected. Destroying duplicate.");
                // In a real engine we might call Entity.Destroy() here if supported
                return;
            }
            _instance = this;
            Log.Info("GameManager Singleton initialized.");
        }

        public override void OnUpdate(float deltaTime)
        {
            if (!IsGameOver)
            {
                GameTime += deltaTime;
            }
        }

        public void AddScore(int points)
        {
            Score += points;
            Log.Info($"Score updated: {Score}");
        }

        public void EndGame()
        {
            IsGameOver = true;
            Log.Info($"Game Over! Final Score: {Score}, Time: {GameTime:F2}s");
        }
    }
}
