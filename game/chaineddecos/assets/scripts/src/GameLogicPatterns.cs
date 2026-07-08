using System;
using Chained;

namespace ChainedDecos.Scripts
{
    // --- 1. SINGLETON ---
    // Manages global game state, score, and current mode.
    // Used as a global access point.
    // NOTE: See GameManager.cs for the actual implementation.
    public class GameManagerPattern : Script
    {
        private static GameManagerPattern? _instance;
        public static GameManagerPattern Instance => _instance ?? throw new Exception("GameManagerPattern not initialized.");

        public int Score { get; set; } = 0;
        public int Coins = 0;
        public string ActiveTheme { get; set; } = "Basic";

        public override void OnCreate()
        {
            if (_instance != null && _instance != this)
            {
                Log.Info("[Singleton] Duplicate GameManagerPattern detected. Ignoring.");
                return;
            }
            _instance = this;
            Log.Info("[Singleton] GameManagerPattern Initialized.");
        }

        public void ResetGame()
        {
            Score = 0;
            Log.Info("[Singleton] Game Reset via GameManagerPattern.");
        }
    }

    // --- 2. PROTOTYPE ---
    // Allows cloning object settings (scale, model) on the fly.
    public class CubePrototype
    {
        public Vector3 Scale;
        public string ModelPath;

        public CubePrototype(Vector3 scale, string modelPath)
        {
            Scale = scale;
            ModelPath = modelPath;
        }

        // Returns a new instance of the prototype (pure C# Prototype)
        public CubePrototype Clone() => new CubePrototype(Scale, ModelPath);

        // Applies prototype parameters to an existing entity
        public void ApplyTo(Entity target)
        {
            if (target == null || !target.IsValid) return;

            // 1. Scale
            var transform = target.GetComponent<TransformComponent>();
            if (transform != null) transform.Scale = Scale;

            // 2. Model (if present)
            var model = target.GetComponent<ModelComponent>();
            if (model == null && !string.IsNullOrEmpty(ModelPath))
                model = target.AddComponent<ModelComponent>();
            
            if (model != null) model.ModelPath = ModelPath;

            Log.Info($"[Prototype] Applied config to Entity {target.ID} (Scale: {Scale}, Model: {ModelPath})");
        }
    }

    
    // Interface for creating game configurations depending on extension/style.
    // Binds logic with system settings (AppWindow) and scenes.
    public interface IGameConfigFactory
    {
        void ConfigureWindow();
        string GetTargetScene();
    }

    // Concrete factory for "Classic" mode (low resolution)
    public class ClassicConfigFactory : IGameConfigFactory
    {
        public void ConfigureWindow()
        {
            AppWindow.SetSize(1280, 720);
            AppWindow.SetFullscreen(false);
            AppWindow.SetVSync(true);
            Log.Info("[Factory] Classic Mode: 720p Windowed");
        }

        public string GetTargetScene() => "scenes/untitled100.chscene";
    }

    // Concrete factory for "Ultra" mode (high resolution, graphics settings)
    public class UltraConfigFactory : IGameConfigFactory
    {
        public void ConfigureWindow()
        {
            AppWindow.SetSize(1920, 1080);
            AppWindow.SetFullscreen(true);
            AppWindow.SetAntialiasing(true);
            Log.Info("[Factory] Ultra Mode: 1080p Fullscreen + AA");
        }

        public string GetTargetScene() => "scenes/test_platform_scene.chscene";
    }

    // --- usage: Script controller for pattern demonstration ---
    public class PatternUsageController : Script
    {
        private IGameConfigFactory? _activeFactory;
        private CubePrototype _smallCube = new CubePrototype(new Vector3(0.5f, 0.5f, 0.5f), "assets/models/cube.obj");
        private CubePrototype _wallCube = new CubePrototype(new Vector3(1.0f, 5.0f, 10.0f), ""); // No model change for wall yet

        public override void OnUpdate(float deltaTime) // Key presses for testing
        {
            // 1. Singleton Test (Space)
            if (Input.IsKeyPressed(Key.Space))
            {
                GameManagerPattern.Instance.Score += 10;
                Log.Info($"[Singleton Test] Score is now: {GameManagerPattern.Instance.Score}");
            }

            // 2. Abstract Factory Test (Keys 1 and 2)
            if (Input.IsKeyPressed(Key.D1)) ApplyConfig(new ClassicConfigFactory());
            if (Input.IsKeyPressed(Key.D2)) ApplyConfig(new UltraConfigFactory());

            // 3. Prototype Test (P - apply, N - clone)
            if (Input.IsKeyPressed(Key.P))
            {
                // Apply wall prototype to current object
                _wallCube.ApplyTo(Entity);
            }

            if (Input.IsKeyPressed(Key.N))
            {
                // Create new entity as a copy of current and apply "small cube" prototype
                Entity? newEntity = Scene.CopyEntity(Entity);
                if (newEntity != null)
                {
                    _smallCube.ApplyTo(newEntity);
                    Log.Info($"[Prototype Test] Spawning new entity {newEntity.ID} via Prototype.");
                }
            }
        }

        private void ApplyConfig(IGameConfigFactory factory)
        {
            _activeFactory = factory;
            _activeFactory.ConfigureWindow();
            Log.Info($"[Factory Test] Configuration applied for: {_activeFactory.GetTargetScene()}");
        }
    }
}
