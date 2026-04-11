using System;

namespace CHEngine
{
    /// <summary>Skill tree branches.</summary>
    public enum SkillBranch
    {
        Strength,
        Magic,
        Economy,
        Defense
    }

    /// <summary>Hero gameplay script.</summary>
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

        /// <summary>Adds XP.</summary>
        public void AddExperience(int amount)
        {
            // Placeholder progression logic.
            Log.Info($"Hero gained {amount} XP");
        }

        /// <summary>Unlocks a skill if the hero can afford it.</summary>
        public void PurchaseSkill(Entity skillEntity)
        {
            var skillComponent = skillEntity.GetComponent<SkillComponent>();
            if (skillComponent != null && !skillComponent.IsUnlocked)
            {
                if (Stats.Gold >= 100)
                {
                    Stats.Gold -= 100;
                    skillComponent.IsUnlocked = true;
                    Log.Info("Skill purchased!");
                }
            }
        }
    }

    /// <summary>Hero skill script.</summary>
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

    /// <summary>Item categories.</summary>
    public enum ItemType
    {
        Weapon,
        Armor,
        MagicItem,
        EconomicBoost
    }

    /// <summary>Sample economy script.</summary>
    public class EconomyManager : Script
    {
        private RPGStatsComponent _heroStats;

        public override void OnCreate()
        {
            // Find the hero stats component.
            var heroEntityIds = Entity.FindAllWithComponent<RPGStatsComponent>();
            if (heroEntityIds.Length > 0)
            {
                _heroStats = new Entity(heroEntityIds[0]).GetComponent<RPGStatsComponent>()!;
            }
        }

        /// <summary>Buys an item if there is enough gold.</summary>
        public bool BuyItem(int cost, ItemType itemType)
        {
            if (_heroStats == null) return false;

            if (_heroStats.Gold >= cost)
            {
                _heroStats.Gold -= cost;
                Log.Info($"Purchased {itemType}! Current Gold: {_heroStats.Gold}");
                return true;
            }
            
            Log.Info("Not enough gold!");
            return false;
        }

        /// <summary>Adds gold to the hero stats.</summary>
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
