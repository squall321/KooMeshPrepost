#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace koomesh {
namespace io {

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
    INFO,       ///< Informational message
    WARNING,    ///< Warning - parsing can continue
    ERROR,      ///< Error - data may be invalid
    FATAL       ///< Fatal error - parsing cannot continue
};

/**
 * @brief Error recovery action
 */
enum class RecoveryAction {
    CONTINUE,       ///< Continue parsing
    SKIP_LINE,      ///< Skip current line
    SKIP_KEYWORD,   ///< Skip entire keyword section
    ABORT           ///< Abort parsing
};

/**
 * @brief Detailed error information
 */
struct ErrorInfo {
    ErrorSeverity severity;         ///< Error severity
    std::string message;            ///< Error message
    std::string filename;           ///< Source filename
    size_t lineNumber;              ///< Line number where error occurred
    std::string keyword;            ///< Current keyword being parsed
    std::string context;            ///< Additional context
    RecoveryAction suggestedAction; ///< Suggested recovery action

    std::chrono::system_clock::time_point timestamp;  ///< When error occurred

    /**
     * @brief Constructor
     */
    ErrorInfo(
        ErrorSeverity sev,
        const std::string& msg,
        const std::string& file = "",
        size_t line = 0,
        const std::string& kw = "",
        const std::string& ctx = "")
        : severity(sev)
        , message(msg)
        , filename(file)
        , lineNumber(line)
        , keyword(kw)
        , context(ctx)
        , suggestedAction(RecoveryAction::CONTINUE)
        , timestamp(std::chrono::system_clock::now())
    {}

    /**
     * @brief Get severity name
     */
    std::string getSeverityName() const;

    /**
     * @brief Format error as string
     */
    std::string format() const;
};

/**
 * @brief Error handling policy
 */
struct ErrorPolicy {
    bool continueOnError = true;        ///< Continue parsing after errors
    bool continueOnWarning = true;      ///< Continue parsing after warnings

    size_t maxErrors = 1000;            ///< Max errors before aborting
    size_t maxWarnings = 10000;         ///< Max warnings before aborting

    bool treatWarningsAsErrors = false; ///< Treat warnings as errors
    bool abortOnFatal = true;           ///< Abort on first fatal error

    bool collectAllErrors = true;       ///< Collect all errors (vs stop at max)
    bool verbose = false;               ///< Verbose error messages
};

/**
 * @brief Error handler callback
 *
 * @param error The error information
 * @return Recovery action to take
 */
using ErrorCallback = std::function<RecoveryAction(const ErrorInfo& error)>;

/**
 * @brief File parsing error handler
 *
 * Centralized error handling and recovery for file parsing operations.
 *
 * Features:
 * - Hierarchical error severity (INFO, WARNING, ERROR, FATAL)
 * - Configurable error policies
 * - Error recovery strategies
 * - Error statistics and reporting
 * - Custom error callbacks
 *
 * Example:
 * @code
 * ErrorHandler handler;
 * ErrorPolicy policy;
 * policy.maxErrors = 100;
 * handler.setPolicy(policy);
 *
 * handler.setCallback([](const ErrorInfo& error) {
 *     std::cerr << error.format() << "\n";
 *     return RecoveryAction::CONTINUE;
 * });
 *
 * if (handler.handleError(ErrorSeverity::ERROR, "Invalid node ID",
 *                         "model.k", 42) == RecoveryAction::ABORT) {
 *     // Stop parsing
 * }
 * @endcode
 */
class ErrorHandler {
public:
    /**
     * @brief Constructor
     */
    ErrorHandler();

    /**
     * @brief Constructor with policy
     */
    explicit ErrorHandler(const ErrorPolicy& policy);

    /**
     * @brief Set error policy
     */
    void setPolicy(const ErrorPolicy& policy);

    /**
     * @brief Get error policy
     */
    const ErrorPolicy& getPolicy() const { return m_policy; }

    /**
     * @brief Set error callback
     */
    void setCallback(ErrorCallback callback);

    /**
     * @brief Handle an error
     *
     * @return Recovery action to take
     */
    RecoveryAction handleError(
        ErrorSeverity severity,
        const std::string& message,
        const std::string& filename = "",
        size_t lineNumber = 0,
        const std::string& keyword = "",
        const std::string& context = "");

    /**
     * @brief Handle an error with full info
     */
    RecoveryAction handleError(const ErrorInfo& error);

    /**
     * @brief Clear all errors
     */
    void clear();

    /**
     * @brief Reset statistics
     */
    void reset();

    /**
     * @brief Get all errors
     */
    const std::vector<ErrorInfo>& getErrors() const { return m_errors; }

    /**
     * @brief Get errors by severity
     */
    std::vector<ErrorInfo> getErrorsBySeverity(ErrorSeverity severity) const;

    /**
     * @brief Get error count
     */
    size_t getErrorCount() const { return m_errorCount; }

    /**
     * @brief Get warning count
     */
    size_t getWarningCount() const { return m_warningCount; }

    /**
     * @brief Get info count
     */
    size_t getInfoCount() const { return m_infoCount; }

    /**
     * @brief Get fatal error count
     */
    size_t getFatalCount() const { return m_fatalCount; }

    /**
     * @brief Check if parsing should abort
     */
    bool shouldAbort() const { return m_shouldAbort; }

    /**
     * @brief Check if there are any errors
     */
    bool hasErrors() const { return m_errorCount > 0 || m_fatalCount > 0; }

    /**
     * @brief Check if there are any warnings
     */
    bool hasWarnings() const { return m_warningCount > 0; }

    /**
     * @brief Generate error report
     */
    std::string generateReport() const;

    /**
     * @brief Generate summary report
     */
    std::string generateSummary() const;

private:
    ErrorPolicy m_policy;
    ErrorCallback m_callback;

    std::vector<ErrorInfo> m_errors;

    size_t m_infoCount = 0;
    size_t m_warningCount = 0;
    size_t m_errorCount = 0;
    size_t m_fatalCount = 0;

    bool m_shouldAbort = false;

    /**
     * @brief Determine recovery action based on policy
     */
    RecoveryAction determineAction(const ErrorInfo& error);

    /**
     * @brief Update statistics
     */
    void updateStatistics(ErrorSeverity severity);

    /**
     * @brief Check if should abort based on counts
     */
    bool checkAbortConditions();
};

/**
 * @brief RAII helper for scoped error handling
 */
class ScopedErrorContext {
public:
    ScopedErrorContext(
        ErrorHandler& handler,
        const std::string& context)
        : m_handler(handler)
        , m_context(context)
    {}

    ~ScopedErrorContext() = default;

    /**
     * @brief Report error in this context
     */
    RecoveryAction error(
        const std::string& message,
        size_t lineNumber = 0,
        const std::string& keyword = "");

    /**
     * @brief Report warning in this context
     */
    RecoveryAction warning(
        const std::string& message,
        size_t lineNumber = 0,
        const std::string& keyword = "");

private:
    ErrorHandler& m_handler;
    std::string m_context;
};

} // namespace io
} // namespace koomesh
