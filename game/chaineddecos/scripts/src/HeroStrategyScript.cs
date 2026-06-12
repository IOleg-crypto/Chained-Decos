using System;
using System.Collections.Generic;
using Chained;

namespace ChainedDecos.Scripts
{
    // =========================================================================
    // 1. FLYWEIGHT (Легковаговик) - Глобальна база даних асетів
    // =========================================================================
    public class ItemData
    {
        public string Name { get; }
        public string Description { get; }
        public int BasePrice { get; }
        public string Category { get; }

        public ItemData(string name, string desc, int price, string category)
        {
            Name = name;
            Description = desc;
            BasePrice = price;
            Category = category;
        }
    }

    public static class GameStorage
    {
        private static readonly Dictionary<string, ItemData> _items = new Dictionary<string, ItemData>();

        static GameStorage()
        {
            Register("DragonSlayer", "Massive physical damage.", 1200, "Weapon");
            Register("ArchmageStaff", "+100% spell potency.", 1500, "Magic");
            Register("MerchantSignet", "Unlock secret VIP prices.", 2000, "Economy");
        }

        public static void Register(string name, string desc, int price, string category) =>
            _items[name] = new ItemData(name, desc, price, category);

        public static ItemData Get(string name) =>
            _items.TryGetValue(name, out var item) ? item : new ItemData(name, "Unknown", 9999, "None");
            
        public static IEnumerable<ItemData> AllItems => _items.Values;
    }

    // =========================================================================
    // 2. DECORATOR (Декоратор) - Динамічне розширення можливостей
    // =========================================================================
    public interface IHero
    {
        string GetTitle();
        int GetPower();
        int GetMagic();
        float GetTradeRatio();
    }

    public class BasicHero : IHero
    {
        private string _name;
        public BasicHero(string name) => _name = name;
        public string GetTitle() => _name;
        public int GetPower() => 10;
        public int GetMagic() => 5;
        public float GetTradeRatio() => 1.0f;
    }

    public abstract class HeroDecorator : IHero
    {
        protected IHero _inner;
        protected HeroDecorator(IHero inner) => _inner = inner;
        public virtual string GetTitle() => _inner.GetTitle();
        public virtual int GetPower() => _inner.GetPower();
        public virtual int GetMagic() => _inner.GetMagic();
        public virtual float GetTradeRatio() => _inner.GetTradeRatio();
    }

    public class WarriorPath : HeroDecorator
    {
        public WarriorPath(IHero inner) : base(inner) { }
        public override string GetTitle() => _inner.GetTitle() + " (Warrior)";
        public override int GetPower() => _inner.GetPower() + 50;
    }

    public class MagePath : HeroDecorator
    {
        public MagePath(IHero inner) : base(inner) { }
        public override string GetTitle() => _inner.GetTitle() + " (Mage)";
        public override int GetMagic() => _inner.GetMagic() + 50;
    }

    // =========================================================================
    // 3. FACADE (Фасад) - Єдина точка входу в систему розвитку
    // =========================================================================
    public class HeroSystem
    {
        private IHero _hero;
        private int _gold = 500;

        public HeroSystem(string name) => _hero = BasicHeroFactory.Create(name);

        public string Upgrade(string type)
        {
            if (type == "warrior") _hero = new WarriorPath(_hero);
            else if (type == "mage") _hero = new MagePath(_hero);
            return $"Hero evolved to: {_hero.GetTitle()}";
        }

        public string Purchase(string itemName)
        {
            var item = GameStorage.Get(itemName);
            int price = (int)(item.BasePrice / _hero.GetTradeRatio());
            
            if (_gold >= price)
            {
                _gold -= price;
                return $"Bought {item.Name} for {price}g.";
            }
            return $"Not enough gold for {item.Name}!";
        }

        public void AddGold(int amount) => _gold += amount;
        public string GetInfo() => $"{_hero.GetTitle()} [Power:{_hero.GetPower()} Magic:{_hero.GetMagic()}] Gold:{_gold}g";
    }

    internal static class BasicHeroFactory 
    {
        public static IHero Create(string name) => new BasicHero(name);
    }

    // =========================================================================
    // ГЕЙМПЛЕЙНИЙ КОНТРОЛЕР
    // =========================================================================
    public class HeroStrategyScript : Script
    {
        private HeroSystem? _heroSystem;
        private List<string> _onScreenLogs = new List<string>();

        public override void OnCreate()
        {
            _heroSystem = new HeroSystem("Arthur");
            AddLog("RPG System Initialized (Facade/Decorator/Flyweight)");
        }

        private void AddLog(string msg)
        {
            Log.Info(msg); // Console log
            _onScreenLogs.Add($"> {msg}");
            if (_onScreenLogs.Count > 10) _onScreenLogs.RemoveAt(0);
        }

        public override void OnUpdate(float deltaTime)
        {
            if (_heroSystem == null) return;

            if (Input.IsKeyPressed(Key.D1)) AddLog(_heroSystem.Upgrade("warrior"));
            if (Input.IsKeyPressed(Key.D2)) AddLog(_heroSystem.Upgrade("mage"));
            if (Input.IsKeyPressed(Key.G)) { _heroSystem.AddGold(100); AddLog("Looted 100 gold."); }
            if (Input.IsKeyPressed(Key.B)) AddLog(_heroSystem.Purchase("DragonSlayer"));
        }

        public override void OnGUI()
        {
            if (_heroSystem == null) return;

            UI.Text("--- [ HERO STATUS ] ---");
            UI.Text(_heroSystem.GetInfo());
            UI.Text("Controls: 1 (Warrior), 2 (Mage), G (Gold), B (Buy)");
            
            UI.Text("");
            UI.Text("--- [ IN-GAME LOGS ] ---");
            foreach (var log in _onScreenLogs)
                UI.Text(log);
        }
    }
}
