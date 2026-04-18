using System;
using System.Collections.Generic;
using CHEngine;

namespace ChainedDecos.Scripts
{
    // =========================================================================
    // 1. FLYWEIGHT (Легковаговик) - Захоплююча бібліотека спорядження та магії
    // =========================================================================
    public class ItemData
    {
        public string Name { get; }
        public string Description { get; }
        public int BasePrice { get; }
        public string Category { get; } // Weapon, Magic, Skill, Perk

        public ItemData(string name, string desc, int price, string category)
        {
            Name = name;
            Description = desc;
            BasePrice = price;
            Category = category;
        }
    }

    public static class ItemFactory
    {
        private static readonly Dictionary<string, ItemData> _items = new Dictionary<string, ItemData>();

        static ItemFactory()
        {
            // Початкова ініціалізація бази знань предметів
            Register("DragonSlayer", "Massive physical damage. For true warriors.", 1200, "Weapon");
            Register("ArchmageStaff", "Focuses mana. +100% spell potency.", 1500, "Magic");
            Register("MerchantGuildSignet", "Unlock secret VIP prices.", 2000, "Economy");
            Register("HealthPotion", "Basic recovery item.", 100, "Consumable");
            Register("FireballScroll", "One-time powerful AoE burst.", 450, "Magic");
        }

        public static void Register(string name, string desc, int price, string category)
        {
            _items[name] = new ItemData(name, desc, price, category);
        }

        public static ItemData Get(string name)
        {
            if (_items.TryGetValue(name, out var item)) return item;
            return new ItemData(name, "Unknown relic", 9999, "Mystery");
        }

        public static void ShowCatalog()
        {
            Log.Info("--- [Flyweight Catalog] ---");
            foreach (var item in _items.Values)
                Log.Info($"{item.Category}: {item.Name} ({item.BasePrice}g) - {item.Description}");
        }
    }

    // =========================================================================
    // 2. DECORATOR (Декоратор) - Структура розвитку героя по гілках
    // =========================================================================
    public interface IHero
    {
        string GetClassTitle();
        int GetTotalPower();
        int GetMagicSkill();
        float GetTradeBonus(); // Economy branch factor
        void LogStatus();
    }

    public class BaseHero : IHero
    {
        private string _name;
        public BaseHero(string name) => _name = name;

        public virtual string GetClassTitle() => "Novice Wanderer";
        public virtual int GetTotalPower() => 10;
        public virtual int GetMagicSkill() => 5;
        public virtual float GetTradeBonus() => 1.0f;

        public virtual void LogStatus()
        {
            Log.Info($"=== HERO: {_name} ({GetClassTitle()}) ===");
            Log.Info($"> Power: {GetTotalPower()} | Magic: {GetMagicSkill()} | Trade: {GetTradeBonus():F2}x");
        }
    }

    public abstract class HeroUpgrade : IHero
    {
        protected IHero _hero;
        protected HeroUpgrade(IHero hero) => _hero = hero;

        public virtual string GetClassTitle() => _hero.GetClassTitle();
        public virtual int GetTotalPower() => _hero.GetTotalPower();
        public virtual int GetMagicSkill() => _hero.GetMagicSkill();
        public virtual float GetTradeBonus() => _hero.GetTradeBonus();
        public virtual void LogStatus() => _hero.LogStatus();
    }

    // Гілка Сили
    public class StrengthMastery : HeroUpgrade
    {
        public StrengthMastery(IHero hero) : base(hero) { }
        public override string GetClassTitle() => base.GetClassTitle() + " + Gladiator";
        public override int GetTotalPower() => base.GetTotalPower() + 25;
    }

    // Гілка Магії
    public class MagicMastery : HeroUpgrade
    {
        public MagicMastery(IHero hero) : base(hero) { }
        public override string GetClassTitle() => base.GetClassTitle() + " + Sorcerer";
        public override int GetMagicSkill() => base.GetMagicSkill() + 30;
    }

    // Гілка Економіки
    public class EconomyMastery : HeroUpgrade
    {
        public EconomyMastery(IHero hero) : base(hero) { }
        public override string GetClassTitle() => base.GetClassTitle() + " + Baron";
        public override float GetTradeBonus() => base.GetTradeBonus() * 1.6f;
    }

    // =========================================================================
    // 3. FACADE (Фасад) - Управління всією стратегією через єдиний інтерфейс
    // =========================================================================
    public class HeroStrategyFacade
    {
        private IHero _hero;
        private int _gold = 250;
        private int _level = 1;

        public HeroStrategyFacade(string name)
        {
            _hero = new BaseHero(name);
            Log.Info($"[Strategy] New journey started for {name}.");
        }

        public void ProcessLevelUp(string selection)
        {
            _level++;
            Log.Info($"\n--- [ LEVEL UP TO {_level} ] ---");
            
            switch (selection.ToLower())
            {
                case "strength": _hero = new StrengthMastery(_hero); break;
                case "magic": _hero = new MagicMastery(_hero); break;
                case "economy": _hero = new EconomyMastery(_hero); break;
            }
            
            Log.Info($"Evolved specialization: {selection.ToUpper()} branch.");
            _hero.LogStatus();
        }

        public void ExecuteTrade(string itemName)
        {
            var item = ItemFactory.Get(itemName);
            int finalPrice = (int)(item.BasePrice / _hero.GetTradeBonus());

            if (_gold >= finalPrice)
            {
                _gold -= finalPrice;
                Log.Info($"[Shop] Bought {item.Name} for {finalPrice}g (Market Price: {item.BasePrice}g). Gold: {_gold}");
                Log.Info($"[Item Effect] {item.Description}");
            }
            else
            {
                Log.Info($"[Shop] Insufficient funds for {item.Name}! Remaining: {_gold}g");
            }
        }

        public void EarnRewards(int amount)
        {
            int realAmount = (int)(amount * _hero.GetTradeBonus());
            _gold += realAmount;
            Log.Info($"[Economy] Looted {realAmount}g (Bonus: {(_hero.GetTradeBonus()-1)*100:F0}%). Total Gold: {_gold}g");
        }

        public void ShowHeroSheet() => _hero.LogStatus();

        public void RenderUI()
        {
            UI.Text("=== STRATEGY (Structural) ===");
            UI.Text($"Hero: {_hero.GetClassTitle()}");
            UI.Text($"Power: {_hero.GetTotalPower()} | Magic: {_hero.GetMagicSkill()} | Economy: {_hero.GetTradeBonus():F2}x");
            UI.Text($"Progress: Level {_level}");
            UI.Text($"Gold: {_gold}g");
            UI.Text("-----------------------------");
        }
    }

    // =========================================================================
    // ГЕЙМПЛЕЙНИЙ СКРИПТ (Controller)
    // =========================================================================
    public class HeroStrategyScript : Script
    {
        private HeroStrategyFacade? _facade;

        public override void OnCreate()
        {
            Log.Info("=== RPG PROGRESSION SYSTEM READY ===");
            _facade = new HeroStrategyFacade("Arthur");
            
            ItemFactory.ShowCatalog();
            _facade.ShowHeroSheet();

            Log.Info(">>> CONTROLS:");
            Log.Info("1-3: Choose Branch (Strength, Magic, Economy)");
            Log.Info("G: Farm Gold | W/M/E: Buy specific items");
        }

        public override void OnUpdate(float deltaTime)
        {
            // Розвиток героя
            if (Input.IsKeyPressed(Key.D1)) _facade?.ProcessLevelUp("strength");
            if (Input.IsKeyPressed(Key.D2)) _facade?.ProcessLevelUp("magic");
            if (Input.IsKeyPressed(Key.D3)) _facade?.ProcessLevelUp("economy");

            // Економіка
            if (Input.IsKeyPressed(Key.G)) _facade?.EarnRewards(150);

            // Магазин (Flyweights)
            if (Input.IsKeyPressed(Key.W)) _facade?.ExecuteTrade("DragonSlayer");
            if (Input.IsKeyPressed(Key.M)) _facade?.ExecuteTrade("ArchmageStaff");
            if (Input.IsKeyPressed(Key.E)) _facade?.ExecuteTrade("MerchantGuildSignet");

            // Інфо
            if (Input.IsKeyPressed(Key.I)) _facade?.ShowHeroSheet();

            // Фасад: загальний системний контроль
            if (Input.IsKeyPressed(Key.Escape))
            {
                Log.Info("Terminating RPG Strategy Simulation...");
                AppWindow.SetFullscreen(false);
            }
        }

        public override void OnGUI()
        {
            _facade?.RenderUI();
        }
    }
}
