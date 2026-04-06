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

    // Global progression state shared across gameplay scripts.
    public static class HeroProgression
    {
        private static readonly HashSet<string> s_CompletedLevelIds = new(StringComparer.OrdinalIgnoreCase);

        public static int HeroLevel { get; private set; } = 1;
        public static int Experience { get; private set; } = 0;
        public static int ExperienceToNextLevel { get; private set; } = 100;
        public static int SkillPoints { get; private set; } = 0;
        public static int Gold { get; private set; } = 0;

        public static int StrengthLevel { get; private set; } = 0;
        public static int MagicLevel { get; private set; } = 0;
        public static int EconomyLevel { get; private set; } = 0;

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

            StrengthLevel = 0;
            MagicLevel = 0;
            EconomyLevel = 0;

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

            SkillPoints--;

            switch (branch)
            {
                case ProgressBranch.Strength:
                    StrengthLevel++;
                    break;
                case ProgressBranch.Magic:
                    MagicLevel++;
                    break;
                case ProgressBranch.Economy:
                    EconomyLevel++;
                    break;
            }

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
