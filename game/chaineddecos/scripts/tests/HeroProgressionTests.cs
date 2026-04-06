using ChainedDecos.Scripts;
using Xunit;

[assembly: CollectionBehavior(DisableTestParallelization = true)]

namespace ChainedDecos.Scripts.Tests;

public class HeroProgressionTests
{
    public HeroProgressionTests()
    {
        HeroProgression.ResetCampaign();
    }

    [Fact]
    public void ResetCampaign_ShouldRestoreDefaults()
    {
        HeroProgression.AddExperience(210, "seed");
        HeroProgression.AddGold(250, "seed");
        HeroProgression.UpgradeBranch(ProgressBranch.Strength);

        HeroProgression.ResetCampaign();

        Assert.Equal(1, HeroProgression.HeroLevel);
        Assert.Equal(0, HeroProgression.Experience);
        Assert.Equal(100, HeroProgression.ExperienceToNextLevel);
        Assert.Equal(0, HeroProgression.SkillPoints);
        Assert.Equal(0, HeroProgression.Gold);
        Assert.Equal(0, HeroProgression.StrengthLevel);
        Assert.Equal(0, HeroProgression.MagicLevel);
        Assert.Equal(0, HeroProgression.EconomyLevel);
        Assert.Equal(1, HeroProgression.WeaponTier);
        Assert.Equal(1, HeroProgression.MagicTier);
        Assert.Equal(0, HeroProgression.ClearedLevelsCount);
    }

    [Fact]
    public void AddExperience_ShouldLevelUpAndGrantSkillPoint()
    {
        HeroProgression.AddExperience(100, "combat");

        Assert.Equal(2, HeroProgression.HeroLevel);
        Assert.Equal(0, HeroProgression.Experience);
        Assert.Equal(160, HeroProgression.ExperienceToNextLevel);
        Assert.Equal(1, HeroProgression.SkillPoints);
    }

    [Fact]
    public void UpgradeBranch_ShouldRequireSkillPoints()
    {
        Assert.False(HeroProgression.UpgradeBranch(ProgressBranch.Strength));

        HeroProgression.AddExperience(100, "combat");

        Assert.True(HeroProgression.UpgradeBranch(ProgressBranch.Strength));
        Assert.Equal(1, HeroProgression.StrengthLevel);
        Assert.Equal(0, HeroProgression.SkillPoints);
    }

    [Fact]
    public void BuyWeaponUpgrade_ShouldSpendGoldAndIncreaseTier()
    {
        HeroProgression.AddGold(500, "loot");

        int beforeCost = HeroProgression.GetWeaponUpgradeCost();
        bool upgraded = HeroProgression.BuyWeaponUpgrade();

        Assert.True(upgraded);
        Assert.Equal(2, HeroProgression.WeaponTier);
        Assert.Equal(500 - beforeCost, HeroProgression.Gold);
        Assert.Equal(25, HeroProgression.Experience);
    }

    [Fact]
    public void EconomyUpgrade_ShouldReduceShopCosts()
    {
        int baseWeaponCost = HeroProgression.GetWeaponUpgradeCost();
        int baseMagicCost = HeroProgression.GetMagicUpgradeCost();

        HeroProgression.AddExperience(100, "combat");
        Assert.True(HeroProgression.UpgradeBranch(ProgressBranch.Economy));

        int discountedWeaponCost = HeroProgression.GetWeaponUpgradeCost();
        int discountedMagicCost = HeroProgression.GetMagicUpgradeCost();

        Assert.True(discountedWeaponCost < baseWeaponCost);
        Assert.True(discountedMagicCost < baseMagicCost);
    }

    [Fact]
    public void CompleteLevel_ShouldRewardOnlyOnFirstClear()
    {
        bool first = HeroProgression.CompleteLevel("tutorial");
        bool second = HeroProgression.CompleteLevel("tutorial");

        Assert.True(first);
        Assert.False(second);
        Assert.Equal(1, HeroProgression.ClearedLevelsCount);
        Assert.Equal(2, HeroProgression.HeroLevel);
        Assert.Equal(35, HeroProgression.Experience);
        Assert.Equal(100, HeroProgression.Gold);
        Assert.Equal(2, HeroProgression.SkillPoints);
    }
}
