using System;
using System.Collections.Generic;
using CHEngine;

namespace ChainedDecos.Scripts
{
    // =========================================================================
    // 1. OBSERVER (Спостерігач) - Моніторинг статусу героя
    // =========================================================================
    public interface IHeroObserver
    {
        void OnHeroStatChanged(string stat, float value);
    }

    // "Консольний інтерфейс" (Concrete Observer)
    public class HeroHUD : IHeroObserver
    {
        public void OnHeroStatChanged(string stat, float value)
        {
            Log.Info($"[Observer: HUD Update] {stat} is now: {value}");
        }
    }

    // =========================================================================
    // 2. STATE (Стан) - Управління поведінкою в різних режимах
    // =========================================================================
    public interface IHeroState
    {
        void UpdateState(HeroBehavioralContext context, float deltaTime);
        string GetName();
    }

    // Стан спокою
    public class IdleState : IHeroState
    {
        public string GetName() => "Idle";
        public void UpdateState(HeroBehavioralContext context, float deltaTime)
        {
            // Поступове відновлення енергії (умовно)
        }
    }

    // Стан тренування
    public class TrainingState : IHeroState
    {
        private float _timer = 0;
        public string GetName() => "Training";
        public void UpdateState(HeroBehavioralContext context, float deltaTime)
        {
            _timer += deltaTime;
            if (_timer >= 2.0f) // Кожні 2 секунди тренування +XP
            {
                context.AddXP(10);
                _timer = 0;
            }
        }
    }

    // =========================================================================
    // 3. COMMAND (Команда) - Виконання та скасування (Undo) дій
    // =========================================================================
    public interface IGameCommand
    {
        void Execute();
        void Undo();
    }

    // Команда покупки
    public class PurchaseCommand : IGameCommand
    {
        private HeroBehavioralContext _context;
        private int _cost;
        private string _item;

        public PurchaseCommand(HeroBehavioralContext context, string item, int cost)
        {
            _context = context;
            _item = item;
            _cost = cost;
        }

        public void Execute()
        {
            if (_context.Gold >= _cost)
            {
                _context.Gold -= _cost;
                Log.Info($"[Command] Purchased {_item}. Gold left: {_context.Gold}");
            }
        }

        public void Undo()
        {
            _context.Gold += _cost;
            Log.Info($"[Command Undo] Returned {_item}. Gold restored to: {_context.Gold}");
        }
    }

    // =========================================================================
    // КОНТЕКСТ ГЕРОЯ (Subject for Observer, Context for State)
    // =========================================================================
    public class HeroBehavioralContext
    {
        private List<IHeroObserver> _observers = new List<IHeroObserver>();
        private IHeroState _currentState = new IdleState();
        private Stack<IGameCommand> _history = new Stack<IGameCommand>();

        public int Gold { get; set; } = 500;
        public int XP { get; private set; } = 0;

        public void AddObserver(IHeroObserver observer) => _observers.Add(observer);
        
        public void SetState(IHeroState newState)
        {
            Log.Info($"[State Change] Hero is now: {newState.GetName()}");
            _currentState = newState;
        }

        public void AddXP(int amount)
        {
            XP += amount;
            NotifyObservers("XP", XP);
        }

        public void ExecuteCommand(IGameCommand cmd)
        {
            cmd.Execute();
            _history.Push(cmd);
            NotifyObservers("Gold", Gold);
        }

        public void UndoLastAction()
        {
            if (_history.Count > 0)
            {
                _history.Pop().Undo();
                NotifyObservers("Gold", Gold);
            }
            else Log.Info("No actions to undo!");
        }

        public void Update(float deltaTime) => _currentState.UpdateState(this, deltaTime);

        public void RenderUI()
        {
            UI.Text("=== BEHAVIORAL (State/Obs/Cmd) ===");
            UI.Text($"Activity: {_currentState.GetName()}");
            UI.Text($"Action Stats -> XP: {XP} | Gold: {Gold}g");
            UI.Text($"History Stack: {_history.Count} actions");
            UI.Text("---------------------------------");
        }

        private void NotifyObservers(string stat, float value)
        {
            foreach (var obs in _observers) obs.OnHeroStatChanged(stat, value);
        }
    }

    // =========================================================================
    // MAIN SCRIPT
    // =========================================================================
    public class HeroBehavioralScript : Script
    {
        private HeroBehavioralContext? _hero;

        public override void OnCreate()
        {
            Log.Info("=== BEHAVIORAL PATTERNS SYSTEM READY ===");
            _hero = new HeroBehavioralContext();
            
            // Підключаємо спостерігача
            _hero.AddObserver(new HeroHUD());
        }

        public override void OnUpdate(float deltaTime)
        {
            _hero?.Update(deltaTime);

            // 1. Тест Станів
            if (Input.IsKeyPressed(Key.T)) _hero?.SetState(new TrainingState());
            if (Input.IsKeyPressed(Key.I)) _hero?.SetState(new IdleState());

            // 2. Тест Команд
            if (Input.IsKeyPressed(Key.B)) // Buy
            {
                _hero?.ExecuteCommand(new PurchaseCommand(_hero, "Mystic Scroll", 150));
            }

            // 3. Тест Undo
            if (Input.IsKeyPressed(Key.U)) // Undo
            {
                _hero?.UndoLastAction();
            }

            // 4. Додаткове золото для тесту
            if (Input.IsKeyPressed(Key.G))
            {
                if (_hero != null)
                {
                    _hero.Gold += 100;
                    Log.Info($"New Gold: {_hero.Gold}");
                }
            }
        }

        public override void OnGUI()
        {
            _hero?.RenderUI();
        }
    }
}
