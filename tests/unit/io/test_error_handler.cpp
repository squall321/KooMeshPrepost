#include <gtest/gtest.h>
#include "io/ErrorHandler.h"
#include <chrono>
#include <thread>

using namespace koomesh::io;

class ErrorHandlerTest : public ::testing::Test {
protected:
    ErrorHandler handler;
};

// ======================================================================
// ErrorInfo Tests
// ======================================================================

TEST_F(ErrorHandlerTest, ErrorInfoCreation) {
    ErrorInfo error(
        ErrorSeverity::ERROR,
        "Test error",
        "test.k",
        42,
        "NODE",
        "Additional context"
    );

    EXPECT_EQ(error.severity, ErrorSeverity::ERROR);
    EXPECT_EQ(error.message, "Test error");
    EXPECT_EQ(error.filename, "test.k");
    EXPECT_EQ(error.lineNumber, 42);
    EXPECT_EQ(error.keyword, "NODE");
    EXPECT_EQ(error.context, "Additional context");
}

TEST_F(ErrorHandlerTest, ErrorInfoSeverityName) {
    EXPECT_EQ(ErrorInfo(ErrorSeverity::INFO, "").getSeverityName(), "INFO");
    EXPECT_EQ(ErrorInfo(ErrorSeverity::WARNING, "").getSeverityName(), "WARNING");
    EXPECT_EQ(ErrorInfo(ErrorSeverity::ERROR, "").getSeverityName(), "ERROR");
    EXPECT_EQ(ErrorInfo(ErrorSeverity::FATAL, "").getSeverityName(), "FATAL");
}

TEST_F(ErrorHandlerTest, ErrorInfoFormat) {
    ErrorInfo error(
        ErrorSeverity::ERROR,
        "Invalid node ID",
        "model.k",
        100,
        "NODE",
        "Expected positive integer"
    );

    std::string formatted = error.format();

    EXPECT_NE(formatted.find("[ERROR]"), std::string::npos);
    EXPECT_NE(formatted.find("model.k:100"), std::string::npos);
    EXPECT_NE(formatted.find("NODE"), std::string::npos);
    EXPECT_NE(formatted.find("Invalid node ID"), std::string::npos);
    EXPECT_NE(formatted.find("Expected positive integer"), std::string::npos);
}

// ======================================================================
// ErrorPolicy Tests
// ======================================================================

TEST_F(ErrorHandlerTest, DefaultPolicy) {
    ErrorPolicy policy;

    EXPECT_TRUE(policy.continueOnError);
    EXPECT_TRUE(policy.continueOnWarning);
    EXPECT_EQ(policy.maxErrors, 1000);
    EXPECT_EQ(policy.maxWarnings, 10000);
    EXPECT_FALSE(policy.treatWarningsAsErrors);
    EXPECT_TRUE(policy.abortOnFatal);
}

TEST_F(ErrorHandlerTest, CustomPolicy) {
    ErrorPolicy policy;
    policy.maxErrors = 10;
    policy.continueOnError = false;
    policy.treatWarningsAsErrors = true;

    handler.setPolicy(policy);

    EXPECT_EQ(handler.getPolicy().maxErrors, 10);
    EXPECT_FALSE(handler.getPolicy().continueOnError);
    EXPECT_TRUE(handler.getPolicy().treatWarningsAsErrors);
}

// ======================================================================
// Error Handling Tests
// ======================================================================

TEST_F(ErrorHandlerTest, HandleSingleError) {
    auto action = handler.handleError(
        ErrorSeverity::ERROR,
        "Test error",
        "test.k",
        10
    );

    EXPECT_EQ(handler.getErrorCount(), 1);
    EXPECT_EQ(handler.getWarningCount(), 0);
    EXPECT_FALSE(handler.shouldAbort());
}

TEST_F(ErrorHandlerTest, HandleMultipleErrors) {
    handler.handleError(ErrorSeverity::ERROR, "Error 1");
    handler.handleError(ErrorSeverity::ERROR, "Error 2");
    handler.handleError(ErrorSeverity::WARNING, "Warning 1");
    handler.handleError(ErrorSeverity::INFO, "Info 1");

    EXPECT_EQ(handler.getErrorCount(), 2);
    EXPECT_EQ(handler.getWarningCount(), 1);
    EXPECT_EQ(handler.getInfoCount(), 1);
    EXPECT_TRUE(handler.hasErrors());
    EXPECT_TRUE(handler.hasWarnings());
}

TEST_F(ErrorHandlerTest, FatalErrorAbortsImmediately) {
    ErrorPolicy policy;
    policy.abortOnFatal = true;
    handler.setPolicy(policy);

    auto action = handler.handleError(
        ErrorSeverity::FATAL,
        "Fatal error"
    );

    EXPECT_EQ(action, RecoveryAction::ABORT);
    EXPECT_TRUE(handler.shouldAbort());
    EXPECT_EQ(handler.getFatalCount(), 1);
}

TEST_F(ErrorHandlerTest, MaxErrorsThreshold) {
    ErrorPolicy policy;
    policy.maxErrors = 5;
    handler.setPolicy(policy);

    // Add 4 errors - should not abort
    for (int i = 0; i < 4; ++i) {
        handler.handleError(ErrorSeverity::ERROR, "Error " + std::to_string(i));
    }
    EXPECT_FALSE(handler.shouldAbort());

    // Add 5th error - should abort
    handler.handleError(ErrorSeverity::ERROR, "Error 5");
    EXPECT_TRUE(handler.shouldAbort());
}

TEST_F(ErrorHandlerTest, MaxWarningsThreshold) {
    ErrorPolicy policy;
    policy.maxWarnings = 3;
    handler.setPolicy(policy);

    for (int i = 0; i < 3; ++i) {
        handler.handleError(ErrorSeverity::WARNING, "Warning " + std::to_string(i));
    }

    EXPECT_TRUE(handler.shouldAbort());
}

TEST_F(ErrorHandlerTest, TreatWarningsAsErrors) {
    ErrorPolicy policy;
    policy.treatWarningsAsErrors = true;
    policy.maxErrors = 5;
    handler.setPolicy(policy);

    // Add 3 warnings and 2 errors = 5 total
    handler.handleError(ErrorSeverity::WARNING, "W1");
    handler.handleError(ErrorSeverity::WARNING, "W2");
    handler.handleError(ErrorSeverity::WARNING, "W3");
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::ERROR, "E2");

    EXPECT_TRUE(handler.shouldAbort());
}

// ======================================================================
// Error Callback Tests
// ======================================================================

TEST_F(ErrorHandlerTest, CustomCallback) {
    int callbackCount = 0;
    ErrorSeverity lastSeverity = ErrorSeverity::INFO;

    handler.setCallback([&](const ErrorInfo& error) {
        ++callbackCount;
        lastSeverity = error.severity;
        return RecoveryAction::CONTINUE;
    });

    handler.handleError(ErrorSeverity::ERROR, "Test");
    handler.handleError(ErrorSeverity::WARNING, "Test 2");

    EXPECT_EQ(callbackCount, 2);
    EXPECT_EQ(lastSeverity, ErrorSeverity::WARNING);
}

TEST_F(ErrorHandlerTest, CallbackCanAbort) {
    handler.setCallback([](const ErrorInfo& error) {
        if (error.message == "Critical") {
            return RecoveryAction::ABORT;
        }
        return RecoveryAction::CONTINUE;
    });

    auto action1 = handler.handleError(ErrorSeverity::ERROR, "Normal");
    EXPECT_EQ(action1, RecoveryAction::CONTINUE);
    EXPECT_FALSE(handler.shouldAbort());

    auto action2 = handler.handleError(ErrorSeverity::ERROR, "Critical");
    EXPECT_EQ(action2, RecoveryAction::ABORT);
    EXPECT_TRUE(handler.shouldAbort());
}

// ======================================================================
// Error Retrieval Tests
// ======================================================================

TEST_F(ErrorHandlerTest, GetAllErrors) {
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::WARNING, "W1");
    handler.handleError(ErrorSeverity::ERROR, "E2");

    const auto& errors = handler.getErrors();
    EXPECT_EQ(errors.size(), 3);
}

TEST_F(ErrorHandlerTest, GetErrorsBySeverity) {
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::WARNING, "W1");
    handler.handleError(ErrorSeverity::ERROR, "E2");
    handler.handleError(ErrorSeverity::WARNING, "W2");

    auto errors = handler.getErrorsBySeverity(ErrorSeverity::ERROR);
    auto warnings = handler.getErrorsBySeverity(ErrorSeverity::WARNING);

    EXPECT_EQ(errors.size(), 2);
    EXPECT_EQ(warnings.size(), 2);
    EXPECT_EQ(errors[0].message, "E1");
    EXPECT_EQ(warnings[0].message, "W1");
}

// ======================================================================
// Reset and Clear Tests
// ======================================================================

TEST_F(ErrorHandlerTest, ClearErrors) {
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::WARNING, "W1");

    EXPECT_EQ(handler.getErrors().size(), 2);

    handler.clear();

    EXPECT_EQ(handler.getErrors().size(), 0);
    // Counts are NOT reset by clear()
    EXPECT_EQ(handler.getErrorCount(), 1);
}

TEST_F(ErrorHandlerTest, ResetHandler) {
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::WARNING, "W1");

    handler.reset();

    EXPECT_EQ(handler.getErrors().size(), 0);
    EXPECT_EQ(handler.getErrorCount(), 0);
    EXPECT_EQ(handler.getWarningCount(), 0);
    EXPECT_FALSE(handler.shouldAbort());
}

// ======================================================================
// Report Generation Tests
// ======================================================================

TEST_F(ErrorHandlerTest, GenerateReport) {
    handler.handleError(ErrorSeverity::ERROR, "Error 1", "test.k", 10);
    handler.handleError(ErrorSeverity::WARNING, "Warning 1", "test.k", 20);

    std::string report = handler.generateReport();

    EXPECT_NE(report.find("ERROR REPORT"), std::string::npos);
    EXPECT_NE(report.find("Errors:   1"), std::string::npos);
    EXPECT_NE(report.find("Warnings: 1"), std::string::npos);
    EXPECT_NE(report.find("Error 1"), std::string::npos);
    EXPECT_NE(report.find("Warning 1"), std::string::npos);
}

TEST_F(ErrorHandlerTest, GenerateSummary) {
    handler.handleError(ErrorSeverity::ERROR, "E1");
    handler.handleError(ErrorSeverity::ERROR, "E2");
    handler.handleError(ErrorSeverity::WARNING, "W1");

    std::string summary = handler.generateSummary();

    EXPECT_NE(summary.find("2 errors"), std::string::npos);
    EXPECT_NE(summary.find("1 warning"), std::string::npos);
}

TEST_F(ErrorHandlerTest, EmptySummary) {
    std::string summary = handler.generateSummary();

    EXPECT_NE(summary.find("No errors"), std::string::npos);
}

// ======================================================================
// Recovery Action Tests
// ======================================================================

TEST_F(ErrorHandlerTest, RecoveryActionForInfo) {
    auto action = handler.handleError(ErrorSeverity::INFO, "Info");
    EXPECT_EQ(action, RecoveryAction::CONTINUE);
}

TEST_F(ErrorHandlerTest, RecoveryActionForWarning) {
    ErrorPolicy policy;
    policy.continueOnWarning = true;
    handler.setPolicy(policy);

    auto action = handler.handleError(ErrorSeverity::WARNING, "Warning");
    EXPECT_EQ(action, RecoveryAction::CONTINUE);

    handler.reset();
    policy.continueOnWarning = false;
    handler.setPolicy(policy);

    action = handler.handleError(ErrorSeverity::WARNING, "Warning");
    EXPECT_EQ(action, RecoveryAction::SKIP_LINE);
}

TEST_F(ErrorHandlerTest, RecoveryActionForError) {
    ErrorPolicy policy;
    policy.continueOnError = true;
    handler.setPolicy(policy);

    auto action = handler.handleError(ErrorSeverity::ERROR, "Error");
    EXPECT_EQ(action, RecoveryAction::SKIP_LINE);

    handler.reset();
    policy.continueOnError = false;
    handler.setPolicy(policy);

    action = handler.handleError(ErrorSeverity::ERROR, "Error");
    EXPECT_EQ(action, RecoveryAction::ABORT);
}

// ======================================================================
// Scoped Error Context Tests
// ======================================================================

TEST_F(ErrorHandlerTest, ScopedErrorContext) {
    ScopedErrorContext context(handler, "test_file.k");

    context.error("Error message", 10, "NODE");
    context.warning("Warning message", 20, "ELEMENT");

    EXPECT_EQ(handler.getErrorCount(), 1);
    EXPECT_EQ(handler.getWarningCount(), 1);

    const auto& errors = handler.getErrors();
    EXPECT_EQ(errors[0].filename, "test_file.k");
    EXPECT_EQ(errors[0].lineNumber, 10);
    EXPECT_EQ(errors[0].keyword, "NODE");
}

// ======================================================================
// Stress Tests
// ======================================================================

TEST_F(ErrorHandlerTest, ManyErrors) {
    ErrorPolicy policy;
    policy.maxErrors = 100000;
    policy.collectAllErrors = false;  // Don't collect all to save memory
    handler.setPolicy(policy);

    for (int i = 0; i < 10000; ++i) {
        handler.handleError(ErrorSeverity::ERROR, "Error " + std::to_string(i));
    }

    EXPECT_EQ(handler.getErrorCount(), 10000);
    EXPECT_FALSE(handler.shouldAbort());
}

TEST_F(ErrorHandlerTest, LargeErrorMessages) {
    std::string largeMessage(10000, 'X');

    handler.handleError(ErrorSeverity::ERROR, largeMessage);

    EXPECT_EQ(handler.getErrorCount(), 1);
    EXPECT_EQ(handler.getErrors()[0].message, largeMessage);
}

// ======================================================================
// Integration Tests
// ======================================================================

TEST_F(ErrorHandlerTest, RealWorldScenario) {
    ErrorPolicy policy;
    policy.maxErrors = 100;
    policy.continueOnError = true;
    handler.setPolicy(policy);

    bool aborted = false;
    handler.setCallback([&](const ErrorInfo& error) {
        std::cout << error.format() << "\n";
        return RecoveryAction::CONTINUE;
    });

    // Simulate parsing a file with various errors
    for (int line = 1; line <= 1000; ++line) {
        if (line % 100 == 0) {
            // Invalid data every 100 lines
            auto action = handler.handleError(
                ErrorSeverity::ERROR,
                "Invalid node ID",
                "large_file.k",
                line,
                "NODE"
            );

            if (action == RecoveryAction::ABORT) {
                aborted = true;
                break;
            }
        }

        if (line % 50 == 0) {
            // Warning every 50 lines
            handler.handleError(
                ErrorSeverity::WARNING,
                "Duplicate node",
                "large_file.k",
                line,
                "NODE"
            );
        }
    }

    EXPECT_FALSE(aborted);
    EXPECT_EQ(handler.getErrorCount(), 10);  // 1000/100
    EXPECT_EQ(handler.getWarningCount(), 20);  // 1000/50
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
