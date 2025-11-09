# Error Handling System

## Overview

The ErrorHandler provides centralized error handling and recovery for file parsing operations. It supports hierarchical error severity levels, configurable policies, recovery strategies, and detailed error reporting.

## Features

- **Hierarchical severity**: INFO → WARNING → ERROR → FATAL
- **Configurable policies**: Max error thresholds, recovery actions
- **Error recovery**: Continue, skip line, skip keyword, or abort
- **Statistics tracking**: Count errors by severity
- **Custom callbacks**: User-defined error handling
- **Detailed reporting**: Full error reports and summaries

## Quick Start

### Basic Usage

```cpp
#include "io/ErrorHandler.h"

using namespace koomesh::io;

// Create handler
ErrorHandler handler;

// Handle an error
auto action = handler.handleError(
    ErrorSeverity::ERROR,
    "Invalid node ID",
    "model.k",  // filename
    42,         // line number
    "NODE"      // keyword
);

if (action == RecoveryAction::ABORT) {
    // Stop parsing
}

// Check status
std::cout << "Errors: " << handler.getErrorCount() << "\n";
std::cout << handler.generateSummary() << "\n";
```

### With Custom Policy

```cpp
ErrorPolicy policy;
policy.maxErrors = 100;              // Stop after 100 errors
policy.continueOnError = true;       // Try to continue after errors
policy.treatWarningsAsErrors = false;

ErrorHandler handler(policy);
```

## Error Severity Levels

| Level | Description | Default Action |
|-------|-------------|----------------|
| **INFO** | Informational message | CONTINUE |
| **WARNING** | Non-critical issue | CONTINUE |
| **ERROR** | Data may be invalid | SKIP_LINE |
| **FATAL** | Cannot continue | ABORT |

## Error Policy

### Configuration Options

```cpp
struct ErrorPolicy {
    bool continueOnError = true;        // Continue after errors
    bool continueOnWarning = true;      // Continue after warnings

    size_t maxErrors = 1000;            // Max errors before abort
    size_t maxWarnings = 10000;         // Max warnings before abort

    bool treatWarningsAsErrors = false; // Escalate warnings
    bool abortOnFatal = true;           // Stop on fatal error

    bool collectAllErrors = true;       // Store all errors
    bool verbose = false;               // Verbose messages
};
```

### Policy Examples

**Strict mode** (fail fast):
```cpp
ErrorPolicy strict;
strict.maxErrors = 0;
strict.continueOnError = false;
strict.treatWarningsAsErrors = true;
```

**Permissive mode** (collect all):
```cpp
ErrorPolicy permissive;
permissive.maxErrors = 1000000;
permissive.continueOnError = true;
permissive.treatWarningsAsErrors = false;
```

**Production mode** (balance):
```cpp
ErrorPolicy production;
production.maxErrors = 1000;
production.continueOnError = true;
production.abortOnFatal = true;
```

## Recovery Actions

| Action | Description | Use Case |
|--------|-------------|----------|
| **CONTINUE** | Keep parsing current line | Warnings, info |
| **SKIP_LINE** | Skip to next line | Invalid data in line |
| **SKIP_KEYWORD** | Skip entire keyword section | Malformed keyword |
| **ABORT** | Stop parsing immediately | Fatal errors |

## Custom Error Callbacks

### Basic Callback

```cpp
handler.setCallback([](const ErrorInfo& error) {
    std::cerr << error.format() << "\n";
    return RecoveryAction::CONTINUE;
});
```

### Conditional Abort

```cpp
handler.setCallback([](const ErrorInfo& error) {
    if (error.keyword == "NODE" && error.severity == ErrorSeverity::FATAL) {
        return RecoveryAction::ABORT;
    }
    return RecoveryAction::SKIP_LINE;
});
```

### Logging Integration

```cpp
handler.setCallback([&logger](const ErrorInfo& error) {
    switch (error.severity) {
        case ErrorSeverity::FATAL:
        case ErrorSeverity::ERROR:
            logger.error(error.format());
            break;
        case ErrorSeverity::WARNING:
            logger.warning(error.format());
            break;
        case ErrorSeverity::INFO:
            logger.info(error.format());
            break;
    }
    return RecoveryAction::CONTINUE;
});
```

## Integration with Parsers

### ParseContext Integration

```cpp
#include "io/LSDynaKeywordParser.h"
#include "io/ErrorHandler.h"

ErrorHandler errorHandler;
ParseContext context;
context.errorHandler = &errorHandler;  // Connect to context

// Parser will automatically use errorHandler
parser->parse(lines, mesh, context);

// Check results
if (errorHandler.hasErrors()) {
    std::cout << errorHandler.generateReport();
}
```

### Manual Error Reporting

```cpp
ParseContext context;
context.errorHandler = &errorHandler;
context.filename = "model.k";
context.lineNumber = 42;
context.currentKeyword = "NODE";

// Error is automatically formatted with context
context.addError("Invalid node ID");
context.addWarning("Duplicate node");
```

### Scoped Context

```cpp
ScopedErrorContext context(handler, "model.k");

context.error("Invalid data", 10, "NODE");
context.warning("Missing field", 20, "ELEMENT");
```

## Error Information

### ErrorInfo Structure

```cpp
struct ErrorInfo {
    ErrorSeverity severity;
    std::string message;
    std::string filename;
    size_t lineNumber;
    std::string keyword;
    std::string context;
    RecoveryAction suggestedAction;
    std::chrono::system_clock::time_point timestamp;
};
```

### Formatting

```cpp
ErrorInfo error(ErrorSeverity::ERROR, "Invalid node ID",
                "model.k", 100, "NODE", "Expected positive integer");

std::cout << error.format() << "\n";
// Output: [ERROR] model.k:100: In keyword 'NODE': Invalid node ID (Expected positive integer)
```

## Error Reporting

### Full Report

```cpp
std::string report = handler.generateReport();
// Includes:
// - Summary statistics
// - Up to 100 detailed errors
// - Indication if more errors exist
```

### Summary

```cpp
std::string summary = handler.generateSummary();
// Examples:
// "No errors or warnings"
// "2 errors and 1 warning"
// "1 fatal, 5 errors and 3 warnings"
```

### Filtering Errors

```cpp
// Get only errors
auto errors = handler.getErrorsBySeverity(ErrorSeverity::ERROR);

// Get only warnings
auto warnings = handler.getErrorsBySeverity(ErrorSeverity::WARNING);

// Process each error
for (const auto& error : errors) {
    std::cout << error.format() << "\n";
}
```

## Statistics

```cpp
// Get counts
size_t infoCount = handler.getInfoCount();
size_t warningCount = handler.getWarningCount();
size_t errorCount = handler.getErrorCount();
size_t fatalCount = handler.getFatalCount();

// Check status
bool hasErrors = handler.hasErrors();
bool hasWarnings = handler.hasWarnings();
bool shouldAbort = handler.shouldAbort();
```

## Complete Example

```cpp
#include "io/ErrorHandler.h"
#include "io/LSDynaFileReader.h"

int main() {
    // Configure error handling
    ErrorPolicy policy;
    policy.maxErrors = 100;
    policy.continueOnError = true;

    ErrorHandler errorHandler(policy);

    // Set callback for real-time reporting
    errorHandler.setCallback([](const ErrorInfo& error) {
        if (error.severity >= ErrorSeverity::ERROR) {
            std::cerr << error.format() << "\n";
        }
        return RecoveryAction::CONTINUE;
    });

    // Read file with error handling
    LSDynaFileReader reader;
    Mesh mesh;
    ReadOptions options;

    ParseContext context;
    context.errorHandler = &errorHandler;

    auto result = reader.read("model.k", mesh, options);

    // Check results
    if (errorHandler.hasErrors()) {
        std::cout << "\n" << errorHandler.generateReport();

        if (errorHandler.shouldAbort()) {
            std::cerr << "Too many errors - parsing aborted\n";
            return 1;
        }
    }

    std::cout << "Parsing complete: " << errorHandler.generateSummary() << "\n";
    return errorHandler.hasErrors() ? 1 : 0;
}
```

## Best Practices

1. **Set appropriate thresholds**: Balance between catching errors and allowing parsing to complete

2. **Use callbacks for immediate feedback**: Don't wait until end to report errors

3. **Clear/reset between operations**: Prevent error accumulation across multiple files

4. **Check shouldAbort() periodically**: Allow early termination on critical errors

5. **Provide context**: Always include filename, line number, keyword

6. **Categorize severity correctly**:
   - INFO: Informational only
   - WARNING: Recoverable issues
   - ERROR: Data problems
   - FATAL: Cannot continue

7. **Test error paths**: Verify recovery works correctly

## Thread Safety

- **`handleError()`**: Not thread-safe (serialize access)
- **`getErrorCount()`**: Thread-safe (atomic updates)
- **`shouldAbort()`**: Thread-safe (atomic check)

For multi-threaded parsing, use per-thread handlers and merge results.

## See Also

- [File I/O Interfaces](FILE_IO_INTERFACES.md)
- [LS-DYNA File Reader](LSDYNA_FILE_READER.md)
- [LS-DYNA Keyword Parser](LSDYNA_KEYWORD_PARSER.md)
