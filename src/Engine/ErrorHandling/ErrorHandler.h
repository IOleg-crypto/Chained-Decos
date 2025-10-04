#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <functional>
#include <exception>
#include <raylib.h>

enum class ErrorSeverity {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4,
    FATAL = 5
};

enum class ErrorCategory {
    GENERAL = 0,
    RENDERING = 1,
    AUDIO = 2,
    COLLISION = 3,
    PHYSICS = 4,
    ASSET_LOADING = 5,
    FILE_SYSTEM = 6,
    MEMORY = 7,
    NETWORK = 8,
    INPUT = 9
};

struct ErrorInfo {
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::string file;
    std::string function;
    int line;
    std::string timestamp;
    std::string stackTrace;
    bool handled;

    ErrorInfo(ErrorSeverity sev, ErrorCategory cat, const std::string& msg,
              const std::string& srcFile, const std::string& srcFunc, int srcLine);
};

class ErrorHandler {
public:
    static ErrorHandler& GetInstance();

    // Error reporting
    void ReportError(ErrorSeverity severity, ErrorCategory category,
                    const std::string& message, const std::string& file = "",
                    const std::string& function = "", int line = 0);

    // Exception handling
    void HandleException(const std::exception& e, const std::string& context = "");
    void SetExceptionHandler(std::function<void(const std::exception&)> handler);

    // Error recovery
    bool AttemptRecovery(const ErrorInfo& error);
    void RegisterRecoveryFunction(ErrorCategory category, std::function<bool()> recoveryFunc);

    // Logging configuration
    void SetLogLevel(ErrorSeverity level);
    void SetLogFile(const std::string& filename);
    void EnableConsoleLogging(bool enable);
    void EnableFileLogging(bool enable);

    // Error statistics
    struct ErrorStats {
        int totalErrors;
        int errorsBySeverity[6]; // Index by ErrorSeverity enum
        int errorsByCategory[10]; // Index by ErrorCategory enum
        int recoveredErrors;
        int unhandledErrors;
        float errorRate; // Errors per minute
    };

    ErrorStats GetErrorStatistics() const { return m_stats; }
    void ResetErrorStatistics();

    // Error querying
    std::vector<ErrorInfo> GetErrors(ErrorSeverity minSeverity = ErrorSeverity::DEBUG) const;
    std::vector<ErrorInfo> GetErrorsByCategory(ErrorCategory category) const;
    ErrorInfo GetLastError() const;

    // System health monitoring
    bool IsSystemHealthy() const;
    float GetSystemHealthScore() const; // 0.0 to 1.0
    std::string GetHealthReport() const;

private:
    ErrorHandler();
    ~ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    // Internal methods
    void ProcessError(const ErrorInfo& error);
    void LogError(const ErrorInfo& error);
    void UpdateStatistics(const ErrorInfo& error);
    std::string GenerateStackTrace();
    std::string GetCurrentTimestamp();

    // Member variables
    std::vector<ErrorInfo> m_errorHistory;
    std::function<void(const std::exception&)> m_exceptionHandler;
    std::unordered_map<ErrorCategory, std::function<bool()>> m_recoveryFunctions;

    ErrorSeverity m_logLevel;
    std::string m_logFile;
    bool m_consoleLogging;
    bool m_fileLogging;

    ErrorStats m_stats;
    std::chrono::steady_clock::time_point m_startTime;

    // Constants
    static const size_t MAX_ERROR_HISTORY = 1000;
    static const int MAX_RECOVERY_ATTEMPTS = 3;
};

class GameException : public std::exception {
public:
    GameException(const std::string& message, ErrorCategory category = ErrorCategory::GENERAL,
                  const std::string& file = "", const std::string& function = "", int line = 0);
    virtual ~GameException() throw() {}

    virtual const char* what() const throw() { return m_message.c_str(); }
    ErrorCategory GetCategory() const { return m_category; }
    std::string GetFile() const { return m_file; }
    std::string GetFunction() const { return m_function; }
    int GetLine() const { return m_line; }

private:
    std::string m_message;
    ErrorCategory m_category;
    std::string m_file;
    std::string m_function;
    int m_line;
};

// Macros for easy error reporting
#define REPORT_ERROR(severity, category, message) \
    ErrorHandler::GetInstance().ReportError(severity, category, message, __FILE__, __FUNCTION__, __LINE__)

#define REPORT_DEBUG(message) REPORT_ERROR(ErrorSeverity::DEBUG, ErrorCategory::GENERAL, message)
#define REPORT_INFO(message) REPORT_ERROR(ErrorSeverity::INFO, ErrorCategory::GENERAL, message)
#define REPORT_WARNING(message) REPORT_ERROR(ErrorSeverity::WARNING, ErrorCategory::GENERAL, message)
#define REPORT_ERROR_MSG(message) REPORT_ERROR(ErrorSeverity::ERROR, ErrorCategory::GENERAL, message)
#define REPORT_CRITICAL(message) REPORT_ERROR(ErrorSeverity::CRITICAL, ErrorCategory::GENERAL, message)
#define REPORT_FATAL(message) REPORT_ERROR(ErrorSeverity::FATAL, ErrorCategory::GENERAL, message)

// Macros for exception throwing
#define THROW_GAME_EXCEPTION(message, category) \
    throw GameException(message, category, __FILE__, __FUNCTION__, __LINE__)

#define THROW_ASSET_EXCEPTION(message) THROW_GAME_EXCEPTION(message, ErrorCategory::ASSET_LOADING)
#define THROW_RENDER_EXCEPTION(message) THROW_GAME_EXCEPTION(message, ErrorCategory::RENDERING)
#define THROW_AUDIO_EXCEPTION(message) THROW_GAME_EXCEPTION(message, ErrorCategory::AUDIO)

#endif // ERROR_HANDLER_H