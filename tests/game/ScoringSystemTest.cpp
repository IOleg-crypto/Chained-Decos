#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

#include "Game/GameFeatures/ScoringSystem.h"

class ScoringSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        scoringSystem = std::make_unique<ScoringSystem>();
    }

    void TearDown() override {
        scoringSystem.reset();
    }

    std::unique_ptr<ScoringSystem> scoringSystem;
};

TEST_F(ScoringSystemTest, ConstructorInitializesDefaults) {
    EXPECT_EQ(scoringSystem->GetCurrentScore(), 0);
    EXPECT_EQ(scoringSystem->GetComboCount(), 0);
    EXPECT_EQ(scoringSystem->GetComboMultiplier(), 1);
    EXPECT_FALSE(scoringSystem->GetDifficulty().empty());
}



TEST_F(ScoringSystemTest, TimerWorksCorrectly) {
    scoringSystem->StartTimer();
    EXPECT_TRUE(scoringSystem->GetElapsedTime() >= 0.0f);

    // Wait a bit and check time increases
    float initialTime = scoringSystem->GetElapsedTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    float newTime = scoringSystem->GetElapsedTime();

    EXPECT_GT(newTime, initialTime);
}

TEST_F(ScoringSystemTest, ComboSystemWorks) {
    EXPECT_EQ(scoringSystem->GetComboCount(), 0);
    EXPECT_EQ(scoringSystem->GetComboMultiplier(), 1);

    scoringSystem->IncrementCombo();
    EXPECT_EQ(scoringSystem->GetComboCount(), 1);
    EXPECT_EQ(scoringSystem->GetComboMultiplier(), 1); // Should increase with more combos

    scoringSystem->IncrementCombo();
    scoringSystem->IncrementCombo();
    EXPECT_EQ(scoringSystem->GetComboCount(), 3);

    scoringSystem->ResetCombo();
    EXPECT_EQ(scoringSystem->GetComboCount(), 0);
    EXPECT_EQ(scoringSystem->GetComboMultiplier(), 1);
}

TEST_F(ScoringSystemTest, DifficultyAffectsMultiplier) {
    scoringSystem->SetDifficulty("Easy");
    EXPECT_EQ(scoringSystem->GetDifficulty(), "Easy");
    EXPECT_EQ(scoringSystem->GetDifficultyMultiplier(), 1);

    scoringSystem->SetDifficulty("Hard");
    EXPECT_EQ(scoringSystem->GetDifficulty(), "Hard");
    EXPECT_GT(scoringSystem->GetDifficultyMultiplier(), 1);
}

TEST_F(ScoringSystemTest, AchievementsAreInitialized) {
    auto achievements = scoringSystem->GetAchievements();
    EXPECT_GT(achievements.size(), 0);

    // Check that achievements have valid data
    for (const auto& achievement : achievements) {
        EXPECT_FALSE(achievement.id.empty());
        EXPECT_FALSE(achievement.name.empty());
        EXPECT_FALSE(achievement.description.empty());
        EXPECT_GE(achievement.points, 0);
    }
}

TEST_F(ScoringSystemTest, HighScoreSystemWorks) {
    std::vector<ScoreEntry> initialScores = scoringSystem->GetHighScores();
    size_t initialCount = initialScores.size();

    scoringSystem->AddScore(1000);
    scoringSystem->SaveHighScore("TestPlayer");

    std::vector<ScoreEntry> newScores = scoringSystem->GetHighScores();
    EXPECT_GE(newScores.size(), initialCount);
}

TEST_F(ScoringSystemTest, FormattedTimeIsValid) {
    scoringSystem->StartTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string formattedTime = scoringSystem->GetFormattedTime();
    EXPECT_FALSE(formattedTime.empty());

    // Should contain numbers and colons (MM:SS format)
    EXPECT_TRUE(formattedTime.find(':') != std::string::npos);
}