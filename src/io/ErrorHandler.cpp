#include "io/ErrorHandler.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace koomesh {
namespace io {

// ======================================================================
// ErrorInfo Implementation
// ======================================================================

std::string ErrorInfo::getSeverityName() const {
    switch (severity) {
        case ErrorSeverity::INFO:    return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR:   return "ERROR";
        case ErrorSeverity::FATAL:   return "FATAL";
        default:                     return "UNKNOWN";
    }
}

std::string ErrorInfo::format() const {
    std::ostringstream oss;

    oss << "[" << getSeverityName() << "] ";

    if (!filename.empty()) {
        oss << filename;
        if (lineNumber > 0) {
            oss << ":" << lineNumber;
        }
        oss << ": ";
    }

    if (!keyword.empty()) {
        oss << "In keyword '" << keyword << "': ";
    }

    oss << message;

    if (!context.empty()) {
        oss << " (" << context << ")";
    }

    return oss.str();
}

// ======================================================================
// ErrorHandler Implementation
// ======================================================================

ErrorHandler::ErrorHandler()
    : m_policy()
{
}

ErrorHandler::ErrorHandler(const ErrorPolicy& policy)
    : m_policy(policy)
{
}

void ErrorHandler::setPolicy(const ErrorPolicy& policy) {
    m_policy = policy;
}

void ErrorHandler::setCallback(ErrorCallback callback) {
    m_callback = std::move(callback);
}

RecoveryAction ErrorHandler::handleError(
    ErrorSeverity severity,
    const std::string& message,
    const std::string& filename,
    size_t lineNumber,
    const std::string& keyword,
    const std::string& context)
{
    ErrorInfo error(severity, message, filename, lineNumber, keyword, context);
    return handleError(error);
}

RecoveryAction ErrorHandler::handleError(const ErrorInfo& error) {
    // Update statistics
    updateStatistics(error.severity);

    // Store error if collecting
    if (m_policy.collectAllErrors || m_errors.size() < m_policy.maxErrors) {
        m_errors.push_back(error);
    }

    // Check abort conditions
    if (checkAbortConditions()) {
        m_shouldAbort = true;
        return RecoveryAction::ABORT;
    }

    // Call user callback if set
    if (m_callback) {
        RecoveryAction action = m_callback(error);
        if (action == RecoveryAction::ABORT) {
            m_shouldAbort = true;
        }
        return action;
    }

    // Use policy to determine action
    return determineAction(error);
}

void ErrorHandler::clear() {
    m_errors.clear();
}

void ErrorHandler::reset() {
    m_errors.clear();
    m_infoCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;
    m_fatalCount = 0;
    m_shouldAbort = false;
}

std::vector<ErrorInfo> ErrorHandler::getErrorsBySeverity(ErrorSeverity severity) const {
    std::vector<ErrorInfo> result;
    std::copy_if(m_errors.begin(), m_errors.end(), std::back_inserter(result),
        [severity](const ErrorInfo& error) {
            return error.severity == severity;
        });
    return result;
}

std::string ErrorHandler::generateReport() const {
    std::ostringstream oss;

    oss << "=================================================\n";
    oss << "           ERROR REPORT\n";
    oss << "=================================================\n\n";

    oss << "Summary:\n";
    oss << "  Info:     " << m_infoCount << "\n";
    oss << "  Warnings: " << m_warningCount << "\n";
    oss << "  Errors:   " << m_errorCount << "\n";
    oss << "  Fatal:    " << m_fatalCount << "\n";
    oss << "  Total:    " << (m_infoCount + m_warningCount + m_errorCount + m_fatalCount) << "\n\n";

    if (!m_errors.empty()) {
        oss << "Detailed Errors:\n";
        oss << "-------------------------------------------------\n";

        size_t maxDisplay = std::min(m_errors.size(), size_t(100));
        for (size_t i = 0; i < maxDisplay; ++i) {
            oss << (i + 1) << ". " << m_errors[i].format() << "\n";
        }

        if (m_errors.size() > maxDisplay) {
            oss << "... and " << (m_errors.size() - maxDisplay) << " more errors\n";
        }
    }

    return oss.str();
}

std::string ErrorHandler::generateSummary() const {
    std::ostringstream oss;

    size_t total = m_infoCount + m_warningCount + m_errorCount + m_fatalCount;

    if (total == 0) {
        oss << "No errors or warnings";
    } else {
        std::vector<std::string> parts;

        if (m_fatalCount > 0) {
            parts.push_back(std::to_string(m_fatalCount) + " fatal");
        }
        if (m_errorCount > 0) {
            parts.push_back(std::to_string(m_errorCount) + " error" +
                          (m_errorCount != 1 ? "s" : ""));
        }
        if (m_warningCount > 0) {
            parts.push_back(std::to_string(m_warningCount) + " warning" +
                          (m_warningCount != 1 ? "s" : ""));
        }
        if (m_infoCount > 0 && m_policy.verbose) {
            parts.push_back(std::to_string(m_infoCount) + " info");
        }

        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                if (i == parts.size() - 1) {
                    oss << " and ";
                } else {
                    oss << ", ";
                }
            }
            oss << parts[i];
        }
    }

    return oss.str();
}

RecoveryAction ErrorHandler::determineAction(const ErrorInfo& error) {
    ErrorSeverity severity = error.severity;

    // Treat warnings as errors if policy says so
    if (m_policy.treatWarningsAsErrors && severity == ErrorSeverity::WARNING) {
        severity = ErrorSeverity::ERROR;
    }

    switch (severity) {
        case ErrorSeverity::INFO:
            return RecoveryAction::CONTINUE;

        case ErrorSeverity::WARNING:
            return m_policy.continueOnWarning ? RecoveryAction::CONTINUE : RecoveryAction::SKIP_LINE;

        case ErrorSeverity::ERROR:
            return m_policy.continueOnError ? RecoveryAction::SKIP_LINE : RecoveryAction::ABORT;

        case ErrorSeverity::FATAL:
            return m_policy.abortOnFatal ? RecoveryAction::ABORT : RecoveryAction::SKIP_KEYWORD;

        default:
            return RecoveryAction::CONTINUE;
    }
}

void ErrorHandler::updateStatistics(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::INFO:
            ++m_infoCount;
            break;
        case ErrorSeverity::WARNING:
            ++m_warningCount;
            break;
        case ErrorSeverity::ERROR:
            ++m_errorCount;
            break;
        case ErrorSeverity::FATAL:
            ++m_fatalCount;
            break;
    }
}

bool ErrorHandler::checkAbortConditions() {
    // Check fatal count
    if (m_fatalCount > 0 && m_policy.abortOnFatal) {
        return true;
    }

    // Check error threshold
    if (m_errorCount >= m_policy.maxErrors) {
        return true;
    }

    // Check warning threshold
    if (m_warningCount >= m_policy.maxWarnings) {
        return true;
    }

    // Check if treating warnings as errors
    if (m_policy.treatWarningsAsErrors) {
        if ((m_errorCount + m_warningCount) >= m_policy.maxErrors) {
            return true;
        }
    }

    return false;
}

// ======================================================================
// ScopedErrorContext Implementation
// ======================================================================

RecoveryAction ScopedErrorContext::error(
    const std::string& message,
    size_t lineNumber,
    const std::string& keyword)
{
    return m_handler.handleError(
        ErrorSeverity::ERROR,
        message,
        m_context,
        lineNumber,
        keyword
    );
}

RecoveryAction ScopedErrorContext::warning(
    const std::string& message,
    size_t lineNumber,
    const std::string& keyword)
{
    return m_handler.handleError(
        ErrorSeverity::WARNING,
        message,
        m_context,
        lineNumber,
        keyword
    );
}

} // namespace io
} // namespace koomesh
