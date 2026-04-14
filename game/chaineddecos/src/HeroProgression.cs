using System;
using System.Collections.Generic;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public enum ProgressBranch
    {
        Strength,
        Magic,
        Economy
    }

    // Strategy contract for branch-specific hero development.
    public interface IHeroDevelopmentStrategy
    {
        ProgressBranch Branch { get; }
        void Apply(HeroBuildState state);
    }

    public sealed class HeroBuildState
    {
        public int StrengthLevel { get; set; }
        public int MagicLevel { get; set; }
        public int EconomyLevel { get; set; }
    }

    public sealed class StrengthDevelopmentStrategy : IHeroDevelopmentStrategy
    {
        public ProgressBranch Branch => ProgressBranch.Strength;

        public void Apply(HeroBuildState state)
        {
            state.StrengthLevel++;
        }
    }

    public sealed class MagicDevelopmentStrategy : IHeroDevelopmentStrategy
    {
        public ProgressBranch Branch => ProgressBranch.Magic;

        public void Apply(HeroBuildState state)
        {
            state.MagicLevel++;
        }
    }

    public sealed class EconomyDevelopmentStrategy : IHeroDevelopmentStrategy
    {
        public ProgressBranch Branch => ProgressBranch.Economy;

        public void Apply(HeroBuildState state)
        {
            state.EconomyLevel++;
        }
    }

    // Global progression state shared across gameplay scripts.
    public static class HeroProgression
    {
        private static readonly HashSet<string> s_CompletedLevelIds = new(StringComparer.OrdinalIgnoreCase);
        private static readonly HeroBuildState s_BuildState = new();
        private static readonly Dictionary<ProgressBranch, IHeroDevelopmentStrategy> s_DevelopmentStrategies =
            new()
            {
                { ProgressBranch.Strength, new StrengthDevelopmentStrategy() },
                { ProgressBranch.Magic, new MagicDevelopmentStrategy() },
                { ProgressBranch.Economy, new EconomyDevelopmentStrategy() },
            };

        public static int HeroLevel { get; private set; } = 1;
        public static int Experience { get; private set; } = 0;
        public static int ExperienceToNextLevel { get; private set; } = 100;
        public static int SkillPoints { get; private set; } = 0;
        public static int Gold { get; private set; } = 0;

        public static int StrengthLevel => s_BuildState.StrengthLevel;
        public static int MagicLevel => s_BuildState.MagicLevel;
        public static int EconomyLevel => s_BuildState.EconomyLevel;

        public static int WeaponTier { get; private set; } = 1;
        public static int MagicTier { get; private set; } = 1;

        public static int ClearedLevelsCount => s_CompletedLevelIds.Count;

        public static float MoveSpeedMultiplier => 1.0f + StrengthLevel * 0.08f;
        public static float JumpMultiplier => 1.0f + StrengthLevel * 0.06f;
        public static float WeaponDamageMultiplier => 1.0f + StrengthLevel * 0.10f + (WeaponTier - 1) * 0.18f;
        public static float SpellPowerMultiplier => 1.0f + MagicLevel * 0.12f + (MagicTier - 1) * 0.20f;
        public static float GoldGainMultiplier => 1.0f + EconomyLevel * 0.18f;
        public static float ShopDiscountMultiplier => Math.Max(0.55f, 1.0f - EconomyLevel * 0.05f);

        public static int GetWeaponUpgradeCost()
        {
            float baseCost = 120.0f + (WeaponTier - 1) * 90.0f;
            return Math.Max(50, (int)Math.Round(baseCost * ShopDiscountMultiplier));
        }

        public static int GetMagicUpgradeCost()
        {
            float baseCost = 130.0f + (MagicTier - 1) * 95.0f;
            return Math.Max(60, (int)Math.Round(baseCost * ShopDiscountMultiplier));
        }

        public static void ResetCampaign()
        {
            HeroLevel = 1;
            Experience = 0;
            ExperienceToNextLevel = 100;
            SkillPoints = 0;
            Gold = 0;

            s_BuildState.StrengthLevel = 0;
            s_BuildState.MagicLevel = 0;
            s_BuildState.EconomyLevel = 0;

            WeaponTier = 1;
            MagicTier = 1;

            s_CompletedLevelIds.Clear();
            Log.Info("progression: Campaign reset.");
        }

        public static void AddExperience(int amount, string reason = "progress")
        {
            if (amount <= 0)
            {
                return;
            }

            Experience += amount;
            Log.Info($"progression: +{amount} XP ({reason}). XP {Experience}/{ExperienceToNextLevel}");

            while (Experience >= ExperienceToNextLevel)
            {
                Experience -= ExperienceToNextLevel;
                HeroLevel++;
                SkillPoints++;

                ExperienceToNextLevel = 90 + HeroLevel * 35;
                Log.Info($"progression: LEVEL UP -> {HeroLevel}. Skill points: {SkillPoints}");
            }
        }

        public static void AddGold(int baseAmount, string reason = "reward")
        {
            if (baseAmount <= 0)
            {
                return;
            }

            int finalAmount = Math.Max(1, (int)Math.Round(baseAmount * GoldGainMultiplier));
            Gold += finalAmount;
            Log.Info($"progression: +{finalAmount} gold ({reason}). Total gold: {Gold}");
        }

        public static bool UpgradeBranch(ProgressBranch branch)
        {
            if (SkillPoints <= 0)
            {
                Log.Info($"progression: No skill points for {branch} upgrade.");
                return false;
            }

            if (!s_DevelopmentStrategies.TryGetValue(branch, out IHeroDevelopmentStrategy? strategy))
            {
                Log.Warn($"progression: No strategy found for branch {branch}.");
                return false;
            }

            SkillPoints--;
            strategy.Apply(s_BuildState);

            Log.Info($"progression: Upgraded {branch}. STR:{StrengthLevel} MAG:{MagicLevel} ECO:{EconomyLevel} | SP:{SkillPoints}");
            return true;
        }

        public static bool BuyWeaponUpgrade()
        {
            int cost = GetWeaponUpgradeCost();
            if (Gold < cost)
            {
                Log.Info($"progression: Need {cost} gold for weapon upgrade, have {Gold}.");
                return false;
            }

            Gold -= cost;
            WeaponTier++;
            AddExperience(25, "weapon upgrade");
            Log.Info($"progression: Weapon upgraded to tier {WeaponTier}. Gold left: {Gold}");
            return true;
        }

        public static bool BuyMagicUpgrade()
        {
            int cost = GetMagicUpgradeCost();
            if (Gold < cost)
            {
                Log.Info($"progression: Need {cost} gold for magic upgrade, have {Gold}.");
                return false;
            }

            Gold -= cost;
            MagicTier++;
            AddExperience(25, "magic upgrade");
            Log.Info($"progression: Magic upgraded to tier {MagicTier}. Gold left: {Gold}");
            return true;
        }

        public static bool CompleteLevel(string levelId)
        {
            if (string.IsNullOrWhiteSpace(levelId))
            {
                levelId = $"level_{ClearedLevelsCount + 1}";
            }

            string normalized = levelId.Trim();
            if (s_CompletedLevelIds.Contains(normalized))
            {
                Log.Info($"progression: Level '{normalized}' already completed.");
                return false;
            }

            s_CompletedLevelIds.Add(normalized);

            int xpReward = 120 + HeroLevel * 15;
            int goldReward = 90 + HeroLevel * 10;
            AddExperience(xpReward, $"clear {normalized}");
            AddGold(goldReward, $"clear {normalized}");

            // Bonus point for level completion milestones.
            SkillPoints++;
            Log.Info($"progression: Level '{normalized}' completed. Bonus SP +1 -> {SkillPoints}");

            return true;
        }
    }
}
