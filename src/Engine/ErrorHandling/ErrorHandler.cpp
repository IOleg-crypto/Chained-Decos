#include "ErrorHandler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <raylib.h>

ErrorInfo::ErrorInfo(ErrorSeverity sev, ErrorCategory cat, const std::string& msg,
                     const std::string& srcFile, const std::string& srcFunc, int srcLine)
    : severity(sev), category(cat), message(msg), file(srcFile), function(srcFunc),
      line(srcLine), handled(false) {

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    timestamp = ss.str();

    // Generate stack trace (simplified)
    stackTrace = GenerateStackTrace();
}

ErrorHandler& ErrorHandler::GetInstance() {
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler()
    : m_logLevel(ErrorSeverity::DEBUG), m_consoleLogging(true), m_fileLogging(false) {

    m_stats.totalErrors = 0;
    m_stats.recoveredErrors = 0;
    m_stats.unhandledErrors = 0;
    m_stats.errorRate = 0.0f;

    for (int i = 0; i < 6; i++) m_stats.errorsBySeverity[i] = 0;
    for (int i = 0; i < 10; i++) m_stats.errorsByCategory[i] = 0;

    m_startTime = std::chrono::steady_clock::now();
}

void ErrorHandler::ReportError(ErrorSeverity severity, ErrorCategory category,
                              const std::string& message, const std::string& file,
                              const std::string& function, int line) {

    ErrorInfo error(severity, category, message, file, function, line);
    ProcessError(error);
}

void ErrorHandler::HandleException(const std::exception& e, const std::string& context) {
    std::string message = context.empty() ? e.what() : context + ": " + e.what();
    ReportError(ErrorSeverity::ERROR, ErrorCategory::GENERAL, message);

    if (m_exceptionHandler) {
        try {
            m_exceptionHandler(e);
        } catch (const std::exception& handlerException) {
            REPORT_ERROR(ErrorSeverity::CRITICAL, ErrorCategory::GENERAL,
                        "Exception handler failed: " + std::string(handlerException.what()));
        }
    }
}

void ErrorHandler::SetExceptionHandler(std::function<void(const std::exception&)> handler) {
    m_exceptionHandler = handler;
    REPORT_INFO("Exception handler registered");
}

bool ErrorHandler::AttemptRecovery(const ErrorInfo& error) {
    auto it = m_recoveryFunctions.find(error.category);
    if (it != m_recoveryFunctions.end()) {
        try {
            bool recovered = it->second();
            if (recovered) {
                error.handled = true;
                m_stats.recoveredErrors++;
                REPORT_INFO("Successfully recovered from error: " + error.message);
                return true;
            }
        } catch (const std::exception& e) {
            REPORT_ERROR(ErrorSeverity::WARNING, ErrorCategory::GENERAL,
                        "Recovery function failed: " + std::string(e.what()));
        }
    }

    REPORT_WARNING("No recovery function available for category: " +
                   std::to_string(static_cast<int>(error.category)));
    return false;
}

void ErrorHandler::RegisterRecoveryFunction(ErrorCategory category, std::function<bool()> recoveryFunc) {
    m_recoveryFunctions[category] = recoveryFunc;
    REPORT_INFO("Registered recovery function for category: " +
                std::to_string(static_cast<int>(category)));
}

void ErrorHandler::SetLogLevel(ErrorSeverity level) {
    m_logLevel = level;
    REPORT_INFO("Log level set to: " + std::to_string(static_cast<int>(level)));
}

void ErrorHandler::SetLogFile(const std::string& filename) {
    m_logFile = filename;
    REPORT_INFO("Log file set to: " + filename);
}

void ErrorHandler::EnableConsoleLogging(bool enable) {
    m_consoleLogging = enable;
    REPORT_INFO("Console logging " + std::string(enable ? "enabled" : "disabled"));
}

void ErrorHandler::EnableFileLogging(bool enable) {
    m_fileLogging = enable;
    REPORT_INFO("File logging " + std::string(enable ? "enabled" : "disabled"));
}

void ErrorHandler::ResetErrorStatistics() {
    m_stats = ErrorStats{};
    m_errorHistory.clear();
    m_startTime = std::chrono::steady_clock::now();
    REPORT_INFO("Error statistics reset");
}

std::vector<ErrorInfo> ErrorHandler::GetErrors(ErrorSeverity minSeverity) const {
    std::vector<ErrorInfo> filteredErrors;
    for (const auto& error : m_errorHistory) {
        if (static_cast<int>(error.severity) >= static_cast<int>(minSeverity)) {
            filteredErrors.push_back(error);
        }
    }
    return filteredErrors;
}

std::vector<ErrorInfo> ErrorHandler::GetErrorsByCategory(ErrorCategory category) const {
    std::vector<ErrorInfo> filteredErrors;
    for (const auto& error : m_errorHistory) {
        if (error.category == category) {
            filteredErrors.push_back(error);
        }
    }
    return filteredErrors;
}

ErrorInfo ErrorHandler::GetLastError() const {
    return m_errorHistory.empty() ? ErrorInfo(ErrorSeverity::DEBUG, ErrorCategory::GENERAL, "No errors") : m_errorHistory.back();
}

bool ErrorHandler::IsSystemHealthy() const {
    // System is healthy if no critical or fatal errors in the last 5 minutes
    auto fiveMinutesAgo = std::chrono::steady_clock::now() - std::chrono::minutes(5);

    for (const auto& error : m_errorHistory) {
        if (error.severity >= ErrorSeverity::CRITICAL) {
            // Parse timestamp to check if error is recent
            // For now, assume all errors in history are recent
            return false;
        }
    }

    return GetSystemHealthScore() > 0.7f;
}

float ErrorHandler::GetSystemHealthScore() const {
    if (m_errorHistory.empty()) return 1.0f;

    int criticalErrors = m_stats.errorsBySeverity[static_cast<int>(ErrorSeverity::CRITICAL)] +
                        m_stats.errorsBySeverity[static_cast<int>(ErrorSeverity::FATAL)];

    if (criticalErrors > 0) return 0.0f;

    int warningErrors = m_stats.errorsBySeverity[static_cast<int>(ErrorSeverity::WARNING)];
    int totalErrors = m_stats.totalErrors;

    if (totalErrors == 0) return 1.0f;

    // Health score based on error ratio and recovery rate
    float errorRatio = static_cast<float>(warningErrors) / totalErrors;
    float recoveryRate = m_stats.recoveredErrors / static_cast<float>(m_stats.totalErrors);

    return std::max(0.0f, 1.0f - errorRatio * 0.5f + recoveryRate * 0.3f);
}

std::string ErrorHandler::GetHealthReport() const {
    std::stringstream report;
    report << "System Health Report:\n";
    report << "Health Score: " << std::fixed << std::setprecision(2) << (GetSystemHealthScore() * 100) << "%\n";
    report << "Total Errors: " << m_stats.totalErrors << "\n";
    report << "Critical/Fatal: " << (m_stats.errorsBySeverity[4] + m_stats.errorsBySeverity[5]) << "\n";
    report << "Recovery Rate: " << std::fixed << std::setprecision(1)
           << (m_stats.totalErrors > 0 ? (m_stats.recoveredErrors * 100.0f / m_stats.totalErrors) : 100.0f) << "%\n";
    report << "Error Rate: " << std::fixed << std::setprecision(2) << m_stats.errorRate << " errors/minute\n";

    return report.str();
}

void ErrorHandler::ProcessError(const ErrorInfo& error) {
    // Add to history
    m_errorHistory.push_back(error);

    // Limit history size
    if (m_errorHistory.size() > MAX_ERROR_HISTORY) {
        m_errorHistory.erase(m_errorHistory.begin());
    }

    // Update statistics
    UpdateStatistics(error);

    // Log the error
    LogError(error);

    // Attempt recovery for errors and above
    if (error.severity >= ErrorSeverity::ERROR) {
        if (!AttemptRecovery(error)) {
            m_stats.unhandledErrors++;

            // For fatal errors, we might want to trigger emergency shutdown
            if (error.severity == ErrorSeverity::FATAL) {
                REPORT_CRITICAL("Fatal error encountered, system may be unstable: " + error.message);
            }
        }
    }
}

void ErrorHandler::LogError(const ErrorInfo& error) {
    if (static_cast<int>(error.severity) < static_cast<int>(m_logLevel)) {
        return; // Below log level threshold
    }

    std::string logMessage = FormatLogMessage(error);

    // Console logging
    if (m_consoleLogging) {
        std::cout << logMessage << std::endl;
    }

    // File logging
    if (m_fileLogging && !m_logFile.empty()) {
        try {
            std::ofstream logFile(m_logFile, std::ios::app);
            if (logFile.is_open()) {
                logFile << logMessage << std::endl;
            }
        } catch (const std::exception& e) {
            // Fallback to console if file logging fails
            std::cerr << "Failed to write to log file: " << e.what() << std::endl;
        }
    }

    // Also use raylib logging for consistency
    switch (error.severity) {
        case ErrorSeverity::DEBUG:
            TraceLog(LOG_DEBUG, "%s", logMessage.c_str());
            break;
        case ErrorSeverity::INFO:
            TraceLog(LOG_INFO, "%s", logMessage.c_str());
            break;
        case ErrorSeverity::WARNING:
            TraceLog(LOG_WARNING, "%s", logMessage.c_str());
            break;
        case ErrorSeverity::ERROR:
        case ErrorSeverity::CRITICAL:
            TraceLog(LOG_ERROR, "%s", logMessage.c_str());
            break;
        case ErrorSeverity::FATAL:
            TraceLog(LOG_FATAL, "%s", logMessage.c_str());
            break;
    }
}

std::string ErrorHandler::FormatLogMessage(const ErrorInfo& error) {
    std::stringstream ss;
    ss << "[" << error.timestamp << "] ";

    switch (error.severity) {
        case ErrorSeverity::DEBUG: ss << "DEBUG"; break;
        case ErrorSeverity::INFO: ss << "INFO"; break;
        case ErrorSeverity::WARNING: ss << "WARNING"; break;
        case ErrorSeverity::ERROR: ss << "ERROR"; break;
        case ErrorSeverity::CRITICAL: ss << "CRITICAL"; break;
        case ErrorSeverity::FATAL: ss << "FATAL"; break;
    }

    ss << " [" << GetCategoryName(error.category) << "] ";
    ss << error.message;

    if (!error.file.empty() && !error.function.empty()) {
        ss << " (at " << error.file << ":" << error.function << ":" << error.line << ")";
    }

    return ss.str();
}

std::string ErrorHandler::GetCategoryName(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::GENERAL: return "GENERAL";
        case ErrorCategory::RENDERING: return "RENDERING";
        case ErrorCategory::AUDIO: return "AUDIO";
        case ErrorCategory::COLLISION: return "COLLISION";
        case ErrorCategory::PHYSICS: return "PHYSICS";
        case ErrorCategory::ASSET_LOADING: return "ASSET_LOADING";
        case ErrorCategory::FILE_SYSTEM: return "FILE_SYSTEM";
        case ErrorCategory::MEMORY: return "MEMORY";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::INPUT: return "INPUT";
        default: return "UNKNOWN";
    }
}

void ErrorHandler::UpdateStatistics(const ErrorInfo& error) {
    m_stats.totalErrors++;
    m_stats.errorsBySeverity[static_cast<int>(error.severity)]++;
    m_stats.errorsByCategory[static_cast<int>(error.category)]++;

    // Update error rate (errors per minute)
    auto now = std::chrono::steady_clock::now();
    auto minutesElapsed = std::chrono::duration<float, std::ratio<60>>(now - m_startTime).count();
    if (minutesElapsed > 0) {
        m_stats.errorRate = m_stats.totalErrors / minutesElapsed;
    }
}

std::string ErrorHandler::GenerateStackTrace() {
    // Simplified stack trace generation
    // In a real implementation, you would use platform-specific APIs
    return "Stack trace not available";
}

std::string ErrorHandler::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

GameException::GameException(const std::string& message, ErrorCategory category,
                           const std::string& file, const std::string& function, int line)
    : m_message(message), m_category(category), m_file(file), m_function(function), m_line(line) {

    // Report this exception to the error handler
    ErrorHandler::GetInstance().HandleException(*this, "GameException thrown");
}