using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    // --- 1. SINGLETON (Одинак) ---
    // Керує загальним станом гри, рахунком та поточним режимом.
    // Використовується як глобальна точка доступу.
    // NOTE: See GameManager.cs for the actual implementation.
    public class GameManagerPattern : Script
    {
        private static GameManagerPattern? _instance;
        public static GameManagerPattern Instance => _instance ?? throw new Exception("GameManagerPattern not initialized.");

        public int Score { get; set; } = 0;
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

    // --- 2. PROTOTYPE (Прототип) ---
    // Дозволяє клонувати налаштування кубів (масштаб, тег) "на ходу".
    // Оскільки фізичного Instantiate в C# немає, прототип копіює дані на існуючі Entity.
    public class CubePrototype
    {
        public Vector3 Scale;
        public string ModelName;

        public CubePrototype(Vector3 scale, string modelName)
        {
            Scale = scale;
            ModelName = modelName;
        }

        // "Клонування" властивостей прототипу на вказану сутність
        public void CloneTo(Entity target)
        {
            TransformComponent? transform = target.GetComponent<TransformComponent>();
            if (transform != null)
            {
                transform.Scale = Scale;
                Log.Info($"[Prototype] Applied {ModelName} prototype to Entity {target.ID}.");
            }
        }
    }

    // --- 3. ABSTRACT FACTORY (Абстрактна фабрика) ---
    // Інтерфейс для створення конфігурацій гри залежно від розширення/стилю.
    // Зв'язує логіку з системними налаштуваннями (AppWindow) та сценами.
    public interface IGameConfigFactory
    {
        void ConfigureWindow();
        string GetTargetScene();
    }

    // Конкретна фабрика для "Classic" режиму (мале розширення)
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

    // Конкретна фабрика для "Ultra" режиму (велике розширення, графічні налаштування)
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

    // --- usage: Скрипт-контролер для демонстрації шаблонів ---
    public class PatternUsageController : Script
    {
        private IGameConfigFactory? _activeFactory;
        private CubePrototype _smallCube = new CubePrototype(new Vector3(0.5f, 0.5f, 0.5f), "Obstacle_Small");
        private CubePrototype _wallCube = new CubePrototype(new Vector3(1.0f, 5.0f, 10.0f), "Wall_Prototype");

        public override void OnUpdate(float deltaTime) // Натискання клавіш для перевірки
        {
            // 1. Тест Singleton (Пробіл)
            if (Input.IsKeyPressed(Key.Space))
            {
                GameManagerPattern.Instance.Score += 10;
                Log.Info($"[Singleton Test] Score is now: {GameManagerPattern.Instance.Score}");
            }

            // 2. Тест Abstract Factory (Клавіші 1 та 2)
            if (Input.IsKeyPressed(Key.D1)) ApplyConfig(new ClassicConfigFactory());
            if (Input.IsKeyPressed(Key.D2)) ApplyConfig(new UltraConfigFactory());

            // 3. Тест Prototype (Клавіша P)
            if (Input.IsKeyPressed(Key.P))
            {
                // Наприклад, перетворюємо поточний об'єкт у стіну за допомогою прототипу
                _wallCube.CloneTo(Entity);
            }
        }

        private void ApplyConfig(IGameConfigFactory factory)
        {
            _activeFactory = factory;
            _activeFactory.ConfigureWindow();
            Log.Info($"[Factory Test] Loading scene: {_activeFactory.GetTargetScene()}");
            //Scene.LoadScene(_activeFactory.GetTargetScene());
        }
    }
}
