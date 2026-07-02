using System;
using Chained;

namespace ChainedDecos.Scripts
{
    
    public class GameController : Script
    {
        public static GameController? Instance;
        public int Coins = 0;

        public override void OnCreate()
        {
            Instance = this;
            Log.Info("GameController: Singleton ready!");
        }

        public void AddCoin()
        {
            Coins++;
            Log.Info($"Coins: {Coins}");
        }
    }

    
    public abstract class ParkourStyleFactory
    {
        public abstract string GetObstacleName();
    }

    public class UrbanStyle : ParkourStyleFactory
    {
        public override string GetObstacleName() => "Metal Bench";
    }

    public class NatureStyle : ParkourStyleFactory
    {
        public override string GetObstacleName() => "Wood Log";
    }

    
    public class SimpleUnit
    {
        public string Name;
        public int HP;

        public SimpleUnit(string name, int hp) { Name = name; HP = hp; }

        public SimpleUnit Clone() => new SimpleUnit(this.Name, this.HP);
        
        public void Show() => Log.Info($"Unit: {Name}, HP: {HP}");
    }

    
    public class PatternUsage : Script
    {
        private ParkourStyleFactory style = new UrbanStyle();
        private SimpleUnit prototype = new SimpleUnit("Obstacle", 100);

        public override void OnUpdate(float deltaTime)
        {
            // Singleton
            if (Input.IsKeyPressed(Key.Space)) GameController.Instance.AddCoin();

            // Factory
            if (Input.IsKeyPressed(Key.F)) Log.Info("Style item: " + style.GetObstacleName());

            // Prototype
            if (Input.IsKeyPressed(Key.P))
            {
                SimpleUnit copy = prototype.Clone();
                copy.Show();
            }
        }
    }
}
