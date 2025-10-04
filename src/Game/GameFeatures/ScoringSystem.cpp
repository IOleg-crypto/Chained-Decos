#include "ScoringSystem.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <raylib.h>

// Define static constexpr members
constexpr float ScoringSystem::COMBO_DECAY_TIME;

ScoreEntry::ScoreEntry(const std::string& name, int pts, float t, const std::string& diff)
    : playerName(name), score(pts), time(t), difficulty(diff),
      timestamp(std::chrono::system_clock::now()) {}

Achievement::Achievement(const std::string& aid, const std::string& aname,
                        const std::string& desc, int pts)
    : id(aid), name(aname), description(desc), unlocked(false), points(pts) {}

ScoringSystem::ScoringSystem()
    : m_currentScore(0), m_comboCount(0), m_comboMultiplier(BASE_COMBO_MULTIPLIER),
      m_currentDifficulty("Easy"), m_totalPausedTime(0.0f), m_isRunning(false), m_isPaused(false) {
    InitializeAchievements();
}

void ScoringSystem::InitializeAchievements() {
    m_achievements = {
        Achievement("first_jump", "First Jump", "Complete your first jump", 10),
        Achievement("speed_demon", "Speed Demon", "Reach maximum speed", 25),
        Achievement("combo_master", "Combo Master", "Achieve 10x combo", 50),
        Achievement("perfectionist", "Perfectionist", "Complete level without falling", 100),
        Achievement("time_trial", "Time Trial", "Complete level in under 2 minutes", 75),
        Achievement("high_scorer", "High Scorer", "Achieve a score over 1000", 100),
        Achievement("marathon_runner", "Marathon Runner", "Run for 5 minutes total", 150),
        Achievement("daredevil", "Daredevil", "Jump from height over 20m", 200)
    };
}

void ScoringSystem::AddScore(int points) {
    int actualPoints = points * m_comboMultiplier * GetDifficultyMultiplier();
    m_currentScore += actualPoints;
    IncrementCombo();
    CheckAchievements();
    TraceLog(LOG_INFO, "ScoringSystem::AddScore() - Added %d points (x%d combo, x%d difficulty)",
             points, m_comboMultiplier, GetDifficultyMultiplier());
}

void ScoringSystem::ResetScore() {
    m_currentScore = 0;
    ResetCombo();
    TraceLog(LOG_INFO, "ScoringSystem::ResetScore() - Score reset");
}

void ScoringSystem::StartTimer() {
    if (!m_isRunning) {
        m_startTime = std::chrono::steady_clock::now();
        m_isRunning = true;
        m_isPaused = false;
        m_totalPausedTime = 0.0f;
        TraceLog(LOG_INFO, "ScoringSystem::StartTimer() - Timer started");
    }
}

void ScoringSystem::StopTimer() {
    if (m_isRunning) {
        m_isRunning = false;
        m_isPaused = false;
        TraceLog(LOG_INFO, "ScoringSystem::StopTimer() - Timer stopped");
    }
}

void ScoringSystem::PauseTimer() {
    if (m_isRunning && !m_isPaused) {
        m_pauseTime = std::chrono::steady_clock::now();
        m_isPaused = true;
        TraceLog(LOG_INFO, "ScoringSystem::PauseTimer() - Timer paused");
    }
}

void ScoringSystem::ResumeTimer() {
    if (m_isRunning && m_isPaused) {
        auto now = std::chrono::steady_clock::now();
        m_totalPausedTime += std::chrono::duration<float>(now - m_pauseTime).count();
        m_isPaused = false;
        TraceLog(LOG_INFO, "ScoringSystem::ResumeTimer() - Timer resumed");
    }
}

float ScoringSystem::GetElapsedTime() const {
    if (!m_isRunning) {
        return 0.0f;
    }

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();

    if (m_isPaused) {
        elapsed -= std::chrono::duration<float>(now - m_pauseTime).count();
    }

    return elapsed - m_totalPausedTime;
}

std::string ScoringSystem::GetFormattedTime() const {
    float time = GetElapsedTime();
    int minutes = static_cast<int>(time) / 60;
    int seconds = static_cast<int>(time) % 60;
    int milliseconds = static_cast<int>((time - static_cast<float>(static_cast<int>(time))) * 1000);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds << ":"
       << std::setfill('0') << std::setw(3) << milliseconds;

    return ss.str();
}

void ScoringSystem::IncrementCombo() {
    m_comboCount++;
    UpdateComboMultiplier();
}

void ScoringSystem::ResetCombo() {
    m_comboCount = 0;
    m_comboMultiplier = BASE_COMBO_MULTIPLIER;
}

void ScoringSystem::UpdateComboMultiplier() {
    if (m_comboCount >= 10) {
        m_comboMultiplier = MAX_COMBO_MULTIPLIER;
    } else if (m_comboCount >= 7) {
        m_comboMultiplier = 4;
    } else if (m_comboCount >= 5) {
        m_comboMultiplier = 3;
    } else if (m_comboCount >= 3) {
        m_comboMultiplier = 2;
    } else {
        m_comboMultiplier = BASE_COMBO_MULTIPLIER;
    }
}

void ScoringSystem::SetDifficulty(const std::string& difficulty) {
    m_currentDifficulty = difficulty;
    TraceLog(LOG_INFO, "ScoringSystem::SetDifficulty() - Difficulty set to: %s", difficulty.c_str());
}

int ScoringSystem::GetDifficultyMultiplier() const {
    if (m_currentDifficulty == "Easy") return 1;
    if (m_currentDifficulty == "Medium") return 2;
    if (m_currentDifficulty == "Hard") return 3;
    if (m_currentDifficulty == "Speedrun") return 4;
    return 1;
}

void ScoringSystem::CheckAchievements() {
    // First Jump
    if (m_currentScore > 0 && !m_achievements[0].unlocked) {
        m_achievements[0].unlocked = true;
        AddScore(m_achievements[0].points);
    }

    // Speed Demon (check if player speed is high)
    // This would need integration with Player class

    // Combo Master
    if (m_comboMultiplier >= 10 && !m_achievements[2].unlocked) {
        m_achievements[2].unlocked = true;
        AddScore(m_achievements[2].points);
    }

    // Time Trial
    if (GetElapsedTime() < 120.0f && GetElapsedTime() > 0 && !m_achievements[4].unlocked) {
        m_achievements[4].unlocked = true;
        AddScore(m_achievements[4].points);
    }

    // High Scorer
    if (m_currentScore > 1000 && !m_achievements[5].unlocked) {
        m_achievements[5].unlocked = true;
        AddScore(m_achievements[5].points);
    }

    // Marathon Runner
    if (GetElapsedTime() > 300.0f && !m_achievements[6].unlocked) {
        m_achievements[6].unlocked = true;
        AddScore(m_achievements[6].points);
    }
}

std::vector<Achievement> ScoringSystem::GetUnlockedAchievements() const {
    std::vector<Achievement> unlocked;
    for (const auto& achievement : m_achievements) {
        if (achievement.unlocked) {
            unlocked.push_back(achievement);
        }
    }
    return unlocked;
}

void ScoringSystem::SaveHighScore(const std::string& playerName) {
    int finalScore = CalculateFinalScore();
    float finalTime = GetElapsedTime();

    ScoreEntry entry(playerName, finalScore, finalTime, m_currentDifficulty);
    m_highScores.push_back(entry);

    SortHighScores();

    if (m_highScores.size() > MAX_HIGH_SCORES) {
        m_highScores.resize(MAX_HIGH_SCORES);
    }

    TraceLog(LOG_INFO, "ScoringSystem::SaveHighScore() - Saved high score: %s - %d points in %.2f seconds",
             playerName.c_str(), finalScore, finalTime);
}

bool ScoringSystem::IsHighScore() const {
    if (m_highScores.size() < MAX_HIGH_SCORES) {
        return true;
    }

    int finalScore = CalculateFinalScore();
    return finalScore > m_highScores.back().score;
}

int ScoringSystem::CalculateFinalScore() const {
    return m_currentScore * GetDifficultyMultiplier();
}

void ScoringSystem::SortHighScores() {
    std::sort(m_highScores.begin(), m_highScores.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) {
                  return a.score > b.score; // Descending order
              });
}

void ScoringSystem::SaveToFile(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            TraceLog(LOG_ERROR, "ScoringSystem::SaveToFile() - Could not open file: %s", filename.c_str());
            return;
        }

        // Save current score and time
        file << "current_score:" << m_currentScore << "\n";
        file << "current_difficulty:" << m_currentDifficulty << "\n";
        file << "elapsed_time:" << GetElapsedTime() << "\n";

        // Save high scores
        file << "high_scores_count:" << m_highScores.size() << "\n";
        for (const auto& entry : m_highScores) {
            file << "entry:" << entry.playerName << ","
                 << entry.score << "," << entry.time << ","
                 << entry.difficulty << "\n";
        }

        // Save achievements
        file << "achievements_count:" << m_achievements.size() << "\n";
        for (const auto& achievement : m_achievements) {
            file << "achievement:" << achievement.id << ","
                 << (achievement.unlocked ? "1" : "0") << "\n";
        }

        file.close();
        TraceLog(LOG_INFO, "ScoringSystem::SaveToFile() - Saved scoring data to %s", filename.c_str());
    }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "ScoringSystem::SaveToFile() - Exception while saving: %s", e.what());
    }
}

void ScoringSystem::LoadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "ScoringSystem::LoadFromFile() - Could not open file: %s", filename.c_str());
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            if (key == "current_score") {
                m_currentScore = std::stoi(value);
            } else if (key == "current_difficulty") {
                m_currentDifficulty = value;
            }
            // Handle other loading...
        }

        file.close();
        TraceLog(LOG_INFO, "ScoringSystem::LoadFromFile() - Loaded scoring data from %s", filename.c_str());
    }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "ScoringSystem::LoadFromFile() - Exception while loading: %s", e.what());
    }
}