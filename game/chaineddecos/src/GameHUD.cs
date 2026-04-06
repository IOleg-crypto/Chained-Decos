using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class GameHUD : Script
{
    private float m_Timer = 0.0f;
    private int m_DebugLevelCounter = 1;

    public override void OnUpdate(float deltaTime)
    {
        m_Timer += deltaTime;

        if (Input.IsKeyPressed(Key.R))
        {
            m_Timer = 0.0f;
            Log.Info("timer reset via R");
        }

        if (Input.IsKeyPressed(Key.U))
            HeroProgression.AddExperience(45, "training");

        if (Input.IsKeyPressed(Key.I))
            HeroProgression.AddGold(35, "loot");

        if (Input.IsKeyPressed(Key.D1))
            HeroProgression.UpgradeBranch(ProgressBranch.Strength);

        if (Input.IsKeyPressed(Key.D2))
            HeroProgression.UpgradeBranch(ProgressBranch.Magic);

        if (Input.IsKeyPressed(Key.D3))
            HeroProgression.UpgradeBranch(ProgressBranch.Economy);

        if (Input.IsKeyPressed(Key.Q))
            HeroProgression.BuyWeaponUpgrade();

        if (Input.IsKeyPressed(Key.E))
            HeroProgression.BuyMagicUpgrade();

        if (Input.IsKeyPressed(Key.L))
        {
            string levelId = $"debug_level_{m_DebugLevelCounter++}";
            HeroProgression.CompleteLevel(levelId);
        }

        if (Input.IsKeyPressed(Key.F10))
        {
            HeroProgression.ResetCampaign();
            m_DebugLevelCounter = 1;
        }
    }

    public override void OnGUI()
    {
        TransformComponent? transform = Entity.GetComponent<TransformComponent>();
        float altitude = transform != null ? transform.Translation.Y : 0.0f;
        
        int hours = (int)(m_Timer / 3600.0f);
        int minutes = (int)((m_Timer - hours * 3600.0f) / 60.0f);
        int seconds = (int)(m_Timer) % 60;

        UI.Text($"Altitude: {altitude:F2}");
        UI.Text($"Time: {hours:D2}:{minutes:D2}:{seconds:D2}");

        UI.Text($"Hero Lvl {HeroProgression.HeroLevel}  XP {HeroProgression.Experience}/{HeroProgression.ExperienceToNextLevel}");
        UI.Text($"Gold: {HeroProgression.Gold}  SP: {HeroProgression.SkillPoints}  Cleared: {HeroProgression.ClearedLevelsCount}");
        UI.Text($"STR:{HeroProgression.StrengthLevel}  MAG:{HeroProgression.MagicLevel}  ECO:{HeroProgression.EconomyLevel}");
        UI.Text($"Weapon Tier:{HeroProgression.WeaponTier} ({HeroProgression.GetWeaponUpgradeCost()}g)  Magic Tier:{HeroProgression.MagicTier} ({HeroProgression.GetMagicUpgradeCost()}g)");
        UI.Text($"Bonuses -> SPD x{HeroProgression.MoveSpeedMultiplier:F2} JUMP x{HeroProgression.JumpMultiplier:F2} DMG x{HeroProgression.WeaponDamageMultiplier:F2}");
        UI.Text($"Magic x{HeroProgression.SpellPowerMultiplier:F2}  Gold x{HeroProgression.GoldGainMultiplier:F2}  Shop x{HeroProgression.ShopDiscountMultiplier:F2}");
        UI.Text("Keys: U XP, I Gold, 1/2/3 Branch, Q Weapon, E Magic, L Clear Level, F10 Reset");
    }
}
}
