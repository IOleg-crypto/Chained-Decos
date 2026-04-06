using System;

namespace CHEngine
{
    public enum SkillBranch
    {
        Strength,
        Magic,
        Economy,
        Defense
    }

    public class Hero : Script
    {
        public RPGStatsComponent Stats;
        public InventoryComponent Inventory;

        public override void OnCreate()
        {
            Stats = GetComponent<RPGStatsComponent>()!;
            Inventory = GetComponent<InventoryComponent>()!;
            
            Log.Info("Hero initialized at level " + Stats.Level);
        }

        public void AddExperience(int amount)
        {
            // Simple logic for XP and Leveling
            // In a real scenario, this would involve ExperienceToNextLevel calculation
            Log.Info($"Hero gained {amount} XP");
        }

        public void PurchaseSkill(Entity skillEntity)
        {
            var skill = skillEntity.GetComponent<SkillComponent>();
            if (skill != null && !skill.IsUnlocked)
            {
                // Basic economy check
                if (Stats.Gold >= 100)
                {
                    Stats.Gold -= 100;
                    skill.IsUnlocked = true;
                    Log.Info("Skill purchased!");
                }
            }
        }
    }

    public class HeroSkill : Script
    {
        public SkillComponent Skill;
        
        public override void OnCreate()
        {
            Skill = GetComponent<SkillComponent>()!;
        }

        public void Activate()
        {
            if (Skill.IsUnlocked)
            {
                Log.Info("Activating skill: " + Entity.ID);
            }
        }
    }

    public enum ItemType
    {
        Weapon,
        Armor,
        MagicItem,
        EconomicBoost
    }

    public class EconomyManager : Script
    {
        private RPGStatsComponent _heroStats;

        public override void OnCreate()
        {
            // Find entity with RPGStats (assuming it's our Hero)
            var heroes = Entity.FindAllWithComponent<RPGStatsComponent>();
            if (heroes.Length > 0)
            {
                _heroStats = new Entity(heroes[0]).GetComponent<RPGStatsComponent>()!;
            }
        }

        public bool BuyItem(int cost, ItemType type)
        {
            if (_heroStats == null) return false;

            if (_heroStats.Gold >= cost)
            {
                _heroStats.Gold -= cost;
                Log.Info($"Purchased {type}! Current Gold: {_heroStats.Gold}");
                return true;
            }
            
            Log.Info("Not enough gold!");
            return false;
        }

        public void CollectGold(int amount)
        {
            if (_heroStats != null)
            {
                _heroStats.Gold += amount;
                Log.Info($"Collected {amount} gold. Total: {_heroStats.Gold}");
            }
        }
    }
}
