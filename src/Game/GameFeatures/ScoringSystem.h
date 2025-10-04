#ifndef SCORING_SYSTEM_H
#define SCORING_SYSTEM_H

#include <string>
#include <vector>
#include <chrono>
#include <raylib.h>

struct ScoreEntry {
    std::string playerName;
    int score;
    float time;
    std::string difficulty;
    std::chrono::system_clock::time_point timestamp;

    ScoreEntry() = default;
    ScoreEntry(const std::string& name, int pts, float t, const std::string& diff);
};

struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    bool unlocked;
    int points;

    Achievement(const std::string& aid, const std::string& aname,
                const std::string& desc, int pts = 0);
};

class ScoringSystem {
public:
    ScoringSystem();
    ~ScoringSystem() = default;

    // Score management
    void AddScore(int points);
    void ResetScore();
    int GetCurrentScore() const { return m_currentScore; }

    // Time tracking
    void StartTimer();
    void StopTimer();
    void PauseTimer();
    void ResumeTimer();
    float GetElapsedTime() const;
    std::string GetFormattedTime() const;

    // Combo system
    void IncrementCombo();
    void ResetCombo();
    int GetComboMultiplier() const { return m_comboMultiplier; }
    int GetComboCount() const { return m_comboCount; }

    // Difficulty scoring
    void SetDifficulty(const std::string& difficulty);
    std::string GetDifficulty() const { return m_currentDifficulty; }
    int GetDifficultyMultiplier() const;

    // Achievements
    void CheckAchievements();
    const std::vector<Achievement>& GetAchievements() const { return m_achievements; }
    std::vector<Achievement> GetUnlockedAchievements() const;

    // High scores
    void SaveHighScore(const std::string& playerName);
    std::vector<ScoreEntry> GetHighScores() const { return m_highScores; }
    bool IsHighScore() const;

    // Persistence
    void SaveToFile(const std::string& filename);
    void LoadFromFile(const std::string& filename);

private:
    // Score state
    int m_currentScore;
    int m_comboCount;
    int m_comboMultiplier;
    std::string m_currentDifficulty;

    // Time tracking
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_pauseTime;
    float m_totalPausedTime;
    bool m_isRunning;
    bool m_isPaused;

    // Achievements
    std::vector<Achievement> m_achievements;

    // High scores
    std::vector<ScoreEntry> m_highScores;
    static const int MAX_HIGH_SCORES = 10;

    // Scoring constants
    static const int BASE_COMBO_MULTIPLIER = 1;
    static const int MAX_COMBO_MULTIPLIER = 5;
    static constexpr float COMBO_DECAY_TIME = 3.0f; // seconds

    // Helper methods
    void InitializeAchievements();
    void UpdateComboMultiplier();
    int CalculateFinalScore() const;
    void SortHighScores();
};

#endif // SCORING_SYSTEM_H