# LS-DYNA Keyword Parser

## Overview

The LS-DYNA Keyword Parser provides a modular, extensible framework for parsing LS-DYNA Keyword files. It supports both fixed-format and free-format files, with specialized parsers for different keyword types.

## Features

- **Modular design**: Separate parsers for each keyword type
- **Format detection**: Auto-detect fixed vs free format
- **Error handling**: Comprehensive error and warning reporting
- **Extensible**: Easy to add new keyword parsers
- **Registry pattern**: Centralized parser management
- **Line-level parsing**: Efficient field extraction utilities

## Quick Start

### Basic Usage

```cpp
#include "io/LSDynaKeywordParser.h"
#include "core/Mesh.h"

using namespace koomesh::io;
using namespace koomesh::core;

// Create mesh and parse context
Mesh mesh;
ParseContext context;
context.filename = "model.k";

// Create parser registry with standard parsers
auto registry = KeywordParserRegistry::createDefault();

// Parse file line by line
std::vector<std::string> lines = readFileLines("model.k");

for (size_t i = 0; i < lines.size(); ++i) {
    const std::string& line = lines[i];
    context.lineNumber = i + 1;

    if (LineParser::isKeyword(line)) {
        std::string keyword = LineParser::extractKeyword(line);

        IKeywordParser* parser = registry->getParser(keyword);
        if (parser) {
            // Get remaining lines for this keyword
            std::vector<std::string> keywordLines(
                lines.begin() + i + 1,
                lines.end());

            // Parse keyword data
            size_t consumed = parser->parse(keywordLines, mesh, context);
            i += consumed;
        }
    }
}

// Check for errors
if (!context.errors.empty()) {
    for (const auto& error : context.errors) {
        std::cerr << "Error: " << error << "\n";
    }
}
```

## Supported Keywords

The parser currently supports these LS-DYNA keywords:

| Keyword | Parser Class | Description |
|---------|--------------|-------------|
| `*NODE` | `NodeParser` | Node definitions |
| `*ELEMENT_SOLID` | `ElementSolidParser` | Solid elements (tet, hex, etc.) |
| `*ELEMENT_SHELL` | `ElementShellParser` | Shell elements (tri, quad) |
| `*ELEMENT_BEAM` | `ElementBeamParser` | Beam elements |
| `*PART` | `PartParser` | Part definitions |

## Line Parser Utilities

### Checking Line Types

```cpp
std::string line = "*NODE";

if (LineParser::isKeyword(line)) {
    std::string keyword = LineParser::extractKeyword(line);
    // keyword = "NODE"
}

if (LineParser::isComment(line)) {
    // Line starts with $ or #
}

if (LineParser::isBlank(line)) {
    // Line is empty or whitespace only
}
```

### Parsing Fixed Format

LS-DYNA uses fixed-width fields (typically 8 or 10 characters):

```cpp
std::string line = "       1      0.0000      1.0000      2.0000";

// Parse with 10-character fields
auto fields = LineParser::parseFixed(line, 10);

for (const auto& field : fields) {
    if (!field.isEmpty) {
        int intValue = field.toInt();
        double doubleValue = field.toDouble();
        size_t unsignedValue = field.toUnsigned();
    }
}
```

### Parsing Free Format

Free format uses commas or whitespace as delimiters:

```cpp
// Comma-separated
std::string line1 = "1, 0.0, 1.0, 2.0";
auto fields1 = LineParser::parseFree(line1);

// Whitespace-separated
std::string line2 = "1  0.0  1.0  2.0";
auto fields2 = LineParser::parseFree(line2);
```

### Auto-Detection

```cpp
std::string line = "1, 0.0, 1.0, 2.0";  // Has comma, will use free format

auto fields = LineParser::parseAuto(line);
```

## Individual Keyword Parsers

### NODE Parser

Parses `*NODE` keyword:

```
*NODE
$#   nid               x               y               z      tc      rc
       1      0.00000000      0.00000000      0.00000000       0       0
       2      1.00000000      0.00000000      0.00000000       0       0
       3      0.00000000      1.00000000      0.00000000       0       0
```

```cpp
NodeParser parser;
std::vector<std::string> lines = {
    "       1      0.0000      0.0000      0.0000",
    "       2      1.0000      0.0000      0.0000"
};

ParseContext context;
Mesh mesh;

size_t consumed = parser.parse(lines, mesh, context);
// mesh now contains 2 nodes
```

### ELEMENT_SOLID Parser

Parses solid elements (tetrahedron, hexahedron, etc.):

```
*ELEMENT_SOLID
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
      10       1       1       2       3       4       5       6       7       8
```

```cpp
ElementSolidParser parser;
// Detects element type based on number of nodes:
// 4 nodes = Tetrahedron
// 5 nodes = Pyramid
// 6 nodes = Pentahedron (Wedge)
// 8 nodes = Hexahedron
```

### ELEMENT_SHELL Parser

Parses shell elements (triangle, quadrilateral):

```
*ELEMENT_SHELL
$#   eid     pid      n1      n2      n3      n4
     100       2       1       2       3       4
```

```cpp
ElementShellParser parser;
// Detects:
// 3 nodes = Triangle
// 4 nodes = Quadrilateral
```

### ELEMENT_BEAM Parser

Parses beam elements (2 nodes):

```
*ELEMENT_BEAM
$#   eid     pid      n1      n2
     200       3       1       2
```

### PART Parser

Parses part definitions:

```
*PART
Steel Plate
$#     pid     secid       mid     eosid      hgid      grav    adpopt      tmid
         1         1         1         0         0         0         0         0
```

## Parse Context

The `ParseContext` tracks parsing state and collects statistics:

```cpp
ParseContext context;
context.filename = "model.k";
context.lineNumber = 42;
context.format = KeywordFormat::AUTO;

// Add errors and warnings
context.addError("Invalid node ID");
context.addWarning("Missing part reference");

// Access statistics
size_t nodesCount = context.nodesProcessed;
size_t elemsCount = context.elementsProcessed;
size_t partsCount = context.partsProcessed;

// Check for issues
if (!context.errors.empty()) {
    for (const auto& error : context.errors) {
        std::cerr << error << "\n";
    }
}
```

## Keyword Parser Registry

Centralized management of keyword parsers:

```cpp
// Create with default parsers
auto registry = KeywordParserRegistry::createDefault();

// Check if keyword is supported
if (registry->hasParser("NODE")) {
    IKeywordParser* parser = registry->getParser("NODE");
    // Use parser...
}

// Get all registered keywords
auto keywords = registry->getRegisteredKeywords();
for (const auto& keyword : keywords) {
    std::cout << keyword << "\n";
}
```

### Registering Custom Parsers

```cpp
class CustomParser : public IKeywordParser {
public:
    std::string keywordName() const override {
        return "CUSTOM_KEYWORD";
    }

    size_t parse(
        const std::vector<std::string>& lines,
        Mesh& mesh,
        ParseContext& context) override
    {
        // Custom parsing logic
        return linesConsumed;
    }
};

auto registry = std::make_unique<KeywordParserRegistry>();
registry->registerParser(std::make_unique<CustomParser>());
```

## Format Types

The parser supports three format detection modes:

```cpp
enum class KeywordFormat {
    FIXED,  // Fixed-width fields (8-10 characters)
    FREE,   // Comma or whitespace separated
    AUTO    // Auto-detect based on content
};

ParseContext context;
context.format = KeywordFormat::AUTO;  // Default
```

## Error Handling

Parsers report errors and warnings through the `ParseContext`:

```cpp
ParseContext context;

// Parse file...
parser.parse(lines, mesh, context);

// Check for errors
if (!context.errors.empty()) {
    std::cerr << "Parsing failed with " << context.errors.size()
              << " errors:\n";
    for (const auto& error : context.errors) {
        std::cerr << "  " << error << "\n";
    }
}

// Check for warnings
if (!context.warnings.empty()) {
    std::cout << context.warnings.size() << " warnings:\n";
    for (const auto& warning : context.warnings) {
        std::cout << "  " << warning << "\n";
    }
}
```

## Complete Example

```cpp
#include "io/LSDynaKeywordParser.h"
#include "io/MemoryMappedFile.h"
#include "core/Mesh.h"
#include <fstream>
#include <sstream>

std::vector<std::string> readLines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

int main() {
    // Load file
    std::vector<std::string> lines = readLines("model.k");

    // Setup
    Mesh mesh;
    ParseContext context;
    context.filename = "model.k";

    auto registry = KeywordParserRegistry::createDefault();

    // Parse
    for (size_t i = 0; i < lines.size(); ++i) {
        context.lineNumber = i + 1;
        const std::string& line = lines[i];

        // Skip comments and blank lines
        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            continue;
        }

        // Handle keywords
        if (LineParser::isKeyword(line)) {
            std::string keyword = LineParser::extractKeyword(line);

            if (keyword == "END") {
                break;
            }

            IKeywordParser* parser = registry->getParser(keyword);
            if (parser) {
                std::vector<std::string> remainingLines(
                    lines.begin() + i + 1,
                    lines.end());

                size_t consumed = parser->parse(remainingLines, mesh, context);
                i += consumed;
            } else {
                context.addWarning("Unsupported keyword: " + keyword);
            }
        }
    }

    // Report results
    std::cout << "Parsed successfully:\n";
    std::cout << "  Nodes: " << context.nodesProcessed << "\n";
    std::cout << "  Elements: " << context.elementsProcessed << "\n";
    std::cout << "  Parts: " << context.partsProcessed << "\n";

    if (!context.errors.empty()) {
        std::cerr << "\nErrors (" << context.errors.size() << "):\n";
        for (const auto& error : context.errors) {
            std::cerr << "  " << error << "\n";
        }
    }

    return context.errors.empty() ? 0 : 1;
}
```

## Testing

The parser includes comprehensive unit tests:

```bash
cd build
ctest -R LSDynaKeywordParserTests -V
```

Test coverage includes:
- Line parser utilities (keywords, comments, blanks)
- Field parsing (fixed and free format)
- Individual keyword parsers
- Parse context error/warning tracking
- Parser registry
- Integration tests

## Performance Considerations

- **Line-by-line parsing**: Efficient for large files
- **Field extraction**: Optimized string operations
- **Registry lookup**: O(1) hash map lookup
- **Memory**: Minimal overhead, nodes/elements created on-demand

## Limitations

1. **Keyword support**: Only common keywords currently supported
2. **Format detection**: May need manual override for ambiguous cases
3. **Validation**: Limited semantic validation (relies on Mesh validators)
4. **Comments**: Inline comments after data not fully supported

## Future Enhancements

- Additional keyword parsers (*SECTION, *MAT, *SET, etc.)
- Multi-line keyword support
- Include file handling (*INCLUDE)
- Binary format support
- Parallel parsing for large files

## See Also

- [LS-DYNA Keyword Format](LSDYNA_KEYWORD_FORMAT.md)
- [Memory Mapped File](MEMORY_MAPPED_FILE.md)
- [File I/O Interfaces](FILE_IO_INTERFACES.md)
- LS-DYNA Keyword User's Manual
