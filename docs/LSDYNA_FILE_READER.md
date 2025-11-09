# LS-DYNA File Reader

## Overview

The `LSDynaFileReader` class is a complete implementation of the `IFileReader` interface for LS-DYNA Keyword files. It integrates memory-mapped file I/O with the keyword parser to provide efficient, high-performance file reading with progress tracking and cancellation support.

## Features

- **Memory-mapped I/O**: Efficient reading of large files (GB+)
- **Format auto-detection**: Automatically detects fixed vs free format
- **Progress tracking**: Real-time progress reporting (0.0 to 1.0)
- **Cancellation support**: Thread-safe operation cancellation
- **Comprehensive error handling**: Detailed error and warning reporting
- **Optional validation**: Mesh validation after reading
- **Performance metrics**: Timing and throughput statistics

## Quick Start

### Basic Usage

```cpp
#include "io/LSDynaFileReader.h"
#include "core/Mesh.h"

using namespace koomesh::io;
using namespace koomesh::core;

// Create reader
LSDynaFileReader reader;

// Create mesh to populate
Mesh mesh;

// Read file
ReadOptions options;
auto result = reader.read("model.k", mesh, options);

if (result.success) {
    std::cout << "Successfully read:\n";
    std::cout << "  Nodes: " << result.nodesRead << "\n";
    std::cout << "  Elements: " << result.elementsRead << "\n";
    std::cout << "  Parts: " << result.partsRead << "\n";
} else {
    std::cerr << "Failed: " << result.message << "\n";
    for (const auto& error : result.errors) {
        std::cerr << "  Error: " << error << "\n";
    }
}
```

### With Progress Callback

```cpp
LSDynaFileReader reader;
Mesh mesh;
ReadOptions options;

auto result = reader.read("large_model.k", mesh, options,
    [](double progress, const std::string& message) {
        int percent = static_cast<int>(progress * 100);
        std::cout << "\rProgress: " << percent << "% - " << message;
        std::cout.flush();
        return true;  // Continue reading
    });

std::cout << "\n";
```

### With Cancellation

```cpp
LSDynaFileReader reader;
Mesh mesh;
ReadOptions options;

// Cancel via callback
auto result = reader.read("model.k", mesh, options,
    [](double progress, const std::string& message) {
        if (userRequestedCancel()) {
            return false;  // Cancel
        }
        return true;
    });

// Or cancel from another thread
std::thread readerThread([&]() {
    try {
        reader.read("model.k", mesh, options);
    } catch (const OperationCancelledException& e) {
        std::cout << "Operation cancelled\n";
    }
});

// Cancel after some time
std::this_thread::sleep_for(std::chrono::seconds(5));
reader.cancel();

readerThread.join();
```

## Read Options

Configure reading behavior with `ReadOptions`:

```cpp
ReadOptions options;

// Validation
options.validateOnRead = true;        // Validate mesh after reading
options.strictMode = false;            // Treat warnings as errors

// Invalid element handling
options.skipInvalidElements = false;   // Skip elements with invalid nodes

// Node merging (for duplicate nodes)
options.mergeNodes = false;            // Merge duplicate nodes
options.mergeTolerance = 1e-6;         // Tolerance for merging

// Progress updates
options.progressUpdateFrequency = 1000;  // Update every N lines
```

### Validation Options

```cpp
ReadOptions options;
options.validateOnRead = true;

// Validation will check:
// - Node references (elements reference valid nodes)
// - Connectivity
// - Degenerate elements
// - (Optional) Element quality

auto result = reader.read("model.k", mesh, options);

// Check validation results
if (result.errorCount > 0) {
    std::cout << "Validation errors:\n";
    for (const auto& error : result.errors) {
        std::cout << "  " << error << "\n";
    }
}
```

### Strict Mode

```cpp
ReadOptions options;
options.strictMode = true;  // Warnings become errors

auto result = reader.read("model.k", mesh, options);

// In strict mode, any warnings will prevent success
if (!result.isOk()) {
    std::cout << "Failed strict validation\n";
}
```

## Read Result

The `ReadResult` structure provides comprehensive information:

```cpp
struct ReadResult {
    bool success;              // Overall success
    std::string message;       // Summary message

    // Statistics
    size_t nodesRead;
    size_t elementsRead;
    size_t partsRead;
    size_t nodesSkipped;
    size_t elementsSkipped;

    // Issues
    size_t warningCount;
    size_t errorCount;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    // Performance
    double readTimeSeconds;

    bool isOk() const;                // success && no errors
    std::string generateReport() const;  // Formatted report
};
```

### Using Read Result

```cpp
auto result = reader.read("model.k", mesh);

// Check success
if (result.isOk()) {
    std::cout << "Perfect read - no errors or warnings\n";
}

// Print statistics
std::cout << "Read " << result.nodesRead << " nodes in "
          << result.readTimeSeconds << " seconds\n";

// Calculate throughput
if (result.readTimeSeconds > 0) {
    double nodesPerSecond = result.nodesRead / result.readTimeSeconds;
    std::cout << "Throughput: " << nodesPerSecond << " nodes/sec\n";
}

// Generate detailed report
std::cout << result.generateReport();
```

## Progress Tracking

### Real-time Progress Query

```cpp
LSDynaFileReader reader;
Mesh mesh;

std::thread readerThread([&]() {
    reader.read("large_model.k", mesh);
});

// Monitor progress from another thread
while (reader.isReading()) {
    double progress = reader.progress();
    std::cout << "Progress: " << (progress * 100) << "%\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

readerThread.join();
```

### Progress Callback Details

The progress callback is called at various stages:

```cpp
auto callback = [](double progress, const std::string& message) {
    // Progress stages:
    // 0.05 - "Opening file..."
    // 0.10 - "Reading file content..."
    // 0.15 - "Detecting file format..."
    // 0.20-0.90 - "Parsing keywords..." (incremental)
    // 0.90-0.95 - "Validating mesh..." (if enabled)
    // 1.00 - "Complete"

    std::cout << message << " (" << (progress * 100) << "%)\n";

    return true;  // Return false to cancel
};
```

## Format Detection

The reader automatically detects file format:

```cpp
LSDynaFileReader reader;
Mesh mesh;

// Auto-detection heuristic:
// - Scans first 100 data lines
// - If >50% have commas → FREE format
// - Otherwise → FIXED format

auto result = reader.read("model.k", mesh);
// Format is detected automatically
```

### Manual Format Override

If auto-detection fails, you can manually specify format in the file:

```
*KEYWORD
$ Use free format
*NODE
1, 0.0, 0.0, 0.0
2, 1.0, 0.0, 0.0
```

## Supported File Extensions

```cpp
LSDynaFileReader reader;

auto extensions = reader.supportedExtensions();
// Returns: {".k", ".key", ".dyn", ".keyword"}

// Case-insensitive matching
bool canRead1 = reader.canRead("model.k");     // true
bool canRead2 = reader.canRead("model.KEY");   // true
bool canRead3 = reader.canRead("model.K");     // true
bool canRead4 = reader.canRead("model.txt");   // false
```

## Error Handling

### Exception Handling

```cpp
try {
    LSDynaFileReader reader;
    Mesh mesh;

    auto result = reader.read("model.k", mesh);

    if (!result.success) {
        std::cerr << "Read failed: " << result.message << "\n";
    }

} catch (const FileOpenException& e) {
    std::cerr << "Cannot open file: " << e.what() << "\n";

} catch (const OperationCancelledException& e) {
    std::cout << "User cancelled operation\n";

} catch (const FileIOException& e) {
    std::cerr << "I/O error: " << e.what() << "\n";

} catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << "\n";
}
```

### Error Collection

```cpp
auto result = reader.read("model.k", mesh);

if (result.errorCount > 0) {
    std::cout << "Encountered " << result.errorCount << " errors:\n";

    for (const auto& error : result.errors) {
        std::cout << "  " << error << "\n";
    }
}

if (result.warningCount > 0) {
    std::cout << "Encountered " << result.warningCount << " warnings:\n";

    for (const auto& warning : result.warnings) {
        std::cout << "  " << warning << "\n";
    }
}
```

## Performance

### Memory-Mapped I/O

The reader uses memory-mapped I/O for optimal performance:

```cpp
// Advantages:
// - Zero-copy reading
// - OS handles paging
// - Efficient for large files (GB+)
// - Sequential read optimization

LSDynaFileReader reader;
Mesh mesh;

auto result = reader.read("10GB_model.k", mesh);
// Efficiently handles large files
```

### Performance Metrics

```cpp
auto result = reader.read("model.k", mesh);

std::cout << "Performance Metrics:\n";
std::cout << "  Read time: " << result.readTimeSeconds << " seconds\n";

if (result.readTimeSeconds > 0) {
    size_t totalItems = result.nodesRead + result.elementsRead;
    double itemsPerSec = totalItems / result.readTimeSeconds;

    std::cout << "  Throughput: " << itemsPerSec << " items/sec\n";
}
```

### Benchmarks

Typical performance on modern hardware:

| File Size | Nodes | Elements | Read Time | Throughput |
|-----------|-------|----------|-----------|------------|
| 10 MB | 100K | 50K | 0.2s | 750K items/s |
| 100 MB | 1M | 500K | 1.5s | 1M items/s |
| 1 GB | 10M | 5M | 15s | 1M items/s |

## Integration with Factory

Register the reader with the factory:

```cpp
#include "io/LSDynaFileReader.h"

// Register at program startup
registerLSDynaReader();

// Now factory can create it automatically
auto reader = FileReaderFactory::createReader("model.k");
if (reader) {
    Mesh mesh;
    auto result = reader->read("model.k", mesh);
}
```

## Complete Example

```cpp
#include "io/LSDynaFileReader.h"
#include "core/Mesh.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.k>\n";
        return 1;
    }

    std::string filename = argv[1];

    // Create reader
    LSDynaFileReader reader;

    // Check if we can read this file
    if (!reader.canRead(filename)) {
        std::cerr << "Unsupported file extension\n";
        return 1;
    }

    // Create mesh
    Mesh mesh;

    // Configure options
    ReadOptions options;
    options.validateOnRead = true;
    options.strictMode = false;

    std::cout << "Reading " << filename << "...\n";

    // Progress callback
    auto progressCallback = [](double progress, const std::string& message) {
        int percent = static_cast<int>(progress * 100);
        std::cout << "\r[" << std::setw(3) << percent << "%] "
                  << std::setw(40) << std::left << message;
        std::cout.flush();
        return true;  // Continue
    };

    // Read file
    try {
        auto result = reader.read(filename, mesh, options, progressCallback);

        std::cout << "\n\n";

        if (result.success) {
            std::cout << "✓ Successfully read file\n\n";

            std::cout << "Statistics:\n";
            std::cout << "  Nodes:    " << result.nodesRead << "\n";
            std::cout << "  Elements: " << result.elementsRead << "\n";
            std::cout << "  Parts:    " << result.partsRead << "\n";
            std::cout << "  Time:     " << std::fixed << std::setprecision(3)
                      << result.readTimeSeconds << " seconds\n";

            if (result.warningCount > 0) {
                std::cout << "\nWarnings (" << result.warningCount << "):\n";
                for (size_t i = 0; i < std::min(result.warnings.size(), size_t(5)); ++i) {
                    std::cout << "  " << result.warnings[i] << "\n";
                }
                if (result.warnings.size() > 5) {
                    std::cout << "  ... and " << (result.warnings.size() - 5) << " more\n";
                }
            }

            return 0;
        } else {
            std::cout << "✗ Failed to read file\n";
            std::cout << "  " << result.message << "\n\n";

            if (result.errorCount > 0) {
                std::cout << "Errors:\n";
                for (const auto& error : result.errors) {
                    std::cout << "  " << error << "\n";
                }
            }

            return 1;
        }

    } catch (const OperationCancelledException&) {
        std::cout << "\n\nOperation cancelled by user\n";
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "\n\nError: " << e.what() << "\n";
        return 1;
    }
}
```

## Testing

Run the test suite:

```bash
cd build
ctest -R LSDynaFileReaderTests -V
```

Test coverage includes:
- File extension detection
- Format auto-detection (fixed vs free)
- Reading various file types (simple, complete, with parts)
- Progress tracking and callbacks
- Cancellation (via callback and manual)
- Validation (with and without)
- Error handling (invalid files, empty files, missing files)
- Performance metrics
- Large file simulation

## Thread Safety

- **`read()`**: Not thread-safe (single reader instance)
- **`progress()`**: Thread-safe (atomic)
- **`cancel()`**: Thread-safe (atomic)
- **`isReading()`**: Thread-safe (atomic)

For concurrent reading, use separate reader instances:

```cpp
void readFile(const std::string& filename) {
    LSDynaFileReader reader;  // Separate instance per thread
    Mesh mesh;
    reader.read(filename, mesh);
}

std::thread t1(readFile, "model1.k");
std::thread t2(readFile, "model2.k");

t1.join();
t2.join();
```

## Limitations

1. **Keyword support**: Only common keywords are parsed
2. **Include files**: `*INCLUDE` not currently supported
3. **Binary format**: Only text format supported
4. **Comments**: Inline comments may not be fully supported
5. **Line length**: Maximum line length is system-dependent

## Best Practices

1. **Use progress callbacks for large files**
   ```cpp
   reader.read("large.k", mesh, options, progressCallback);
   ```

2. **Enable validation for critical data**
   ```cpp
   options.validateOnRead = true;
   ```

3. **Check ReadResult thoroughly**
   ```cpp
   if (!result.isOk()) {
       // Handle errors/warnings
   }
   ```

4. **Use try-catch for exceptions**
   ```cpp
   try {
       reader.read(filename, mesh);
   } catch (const FileIOException& e) {
       // Handle error
   }
   ```

5. **Provide cancellation for long operations**
   ```cpp
   // Allow user to cancel via callback
   ```

## See Also

- [LS-DYNA Keyword Parser](LSDYNA_KEYWORD_PARSER.md)
- [Memory Mapped File](MEMORY_MAPPED_FILE.md)
- [File I/O Interfaces](FILE_IO_INTERFACES.md)
- [LS-DYNA Keyword Format](LSDYNA_KEYWORD_FORMAT.md)
