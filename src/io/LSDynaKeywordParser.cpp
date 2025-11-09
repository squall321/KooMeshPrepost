#include "io/LSDynaKeywordParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace koomesh {
namespace io {

// ======================================================================
// ParsedField Implementation
// ======================================================================

int ParsedField::toInt() const {
    if (isEmpty) return 0;
    try {
        return std::stoi(raw);
    } catch (...) {
        return 0;
    }
}

double ParsedField::toDouble() const {
    if (isEmpty) return 0.0;
    try {
        return std::stod(raw);
    } catch (...) {
        return 0.0;
    }
}

size_t ParsedField::toUnsigned() const {
    if (isEmpty) return 0;
    try {
        return static_cast<size_t>(std::stoull(raw));
    } catch (...) {
        return 0;
    }
}

// ======================================================================
// LineParser Implementation
// ======================================================================

bool LineParser::isKeyword(const std::string& line) {
    std::string trimmed = trim(line);
    return !trimmed.empty() && trimmed[0] == '*';
}

bool LineParser::isComment(const std::string& line) {
    std::string trimmed = trim(line);
    return !trimmed.empty() && (trimmed[0] == '$' || trimmed[0] == '#');
}

bool LineParser::isBlank(const std::string& line) {
    return trim(line).empty();
}

std::string LineParser::extractKeyword(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] != '*') {
        return "";
    }

    // Remove asterisk
    std::string keyword = trimmed.substr(1);

    // Remove any trailing comments or whitespace
    size_t commentPos = keyword.find_first_of("$#");
    if (commentPos != std::string::npos) {
        keyword = keyword.substr(0, commentPos);
    }

    return trim(keyword);
}

std::vector<ParsedField> LineParser::parseFixed(
    const std::string& line,
    size_t fieldWidth)
{
    std::vector<ParsedField> fields;

    size_t pos = 0;
    while (pos < line.length()) {
        size_t endPos = std::min(pos + fieldWidth, line.length());
        std::string fieldStr = line.substr(pos, endPos - pos);

        ParsedField field;
        field.raw = trim(fieldStr);
        field.isEmpty = field.raw.empty();

        fields.push_back(field);
        pos = endPos;
    }

    return fields;
}

std::vector<ParsedField> LineParser::parseFree(const std::string& line) {
    std::vector<ParsedField> fields;

    // Check for comma-separated
    if (line.find(',') != std::string::npos) {
        std::vector<std::string> tokens = split(line, ',');
        for (const auto& token : tokens) {
            ParsedField field;
            field.raw = trim(token);
            field.isEmpty = field.raw.empty();
            fields.push_back(field);
        }
    } else {
        // Whitespace-separated
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            ParsedField field;
            field.raw = token;
            field.isEmpty = false;
            fields.push_back(field);
        }
    }

    return fields;
}

std::vector<ParsedField> LineParser::parseAuto(const std::string& line) {
    // Heuristic: if line contains comma, use free format
    if (line.find(',') != std::string::npos) {
        return parseFree(line);
    }

    // Check if it looks like fixed format (regular spacing)
    // For now, default to fixed format for LS-DYNA
    return parseFixed(line, 10);
}

std::string LineParser::trim(const std::string& str) {
    if (str.empty()) return str;

    size_t start = 0;
    while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    size_t end = str.length();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

std::vector<std::string> LineParser::split(
    const std::string& str,
    char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// ======================================================================
// NodeParser Implementation
// ======================================================================

size_t NodeParser::parse(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context)
{
    size_t linesConsumed = 0;

    for (const auto& line : lines) {
        // Stop at next keyword or end
        if (LineParser::isKeyword(line)) {
            break;
        }

        // Skip comments and blank lines
        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++linesConsumed;
            ++context.lineNumber;
            continue;
        }

        // Parse node
        if (!parseNodeLine(line, mesh, context)) {
            context.addError("Failed to parse node: " + line);
        }

        ++linesConsumed;
        ++context.lineNumber;
    }

    return linesConsumed;
}

bool NodeParser::parseNodeLine(
    const std::string& line,
    core::Mesh& mesh,
    ParseContext& context)
{
    // Parse fields
    auto fields = (context.format == KeywordFormat::FREE)
        ? LineParser::parseFree(line)
        : LineParser::parseFixed(line, 16);  // LS-DYNA uses 16-char fields for nodes

    if (fields.size() < 4) {
        return false;
    }

    // Extract node data
    core::NodeId nodeId = fields[0].toUnsigned();
    double x = fields[1].toDouble();
    double y = fields[2].toDouble();
    double z = fields[3].toDouble();

    if (nodeId == 0) {
        return false;
    }

    // Create and add node
    try {
        core::Node node(nodeId, x, y, z);
        mesh.addNode(node);
        ++context.nodesProcessed;
        return true;
    } catch (const std::exception& e) {
        context.addError(std::string("Failed to add node: ") + e.what());
        return false;
    }
}

// ======================================================================
// ElementSolidParser Implementation
// ======================================================================

size_t ElementSolidParser::parse(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context)
{
    size_t linesConsumed = 0;

    for (const auto& line : lines) {
        if (LineParser::isKeyword(line)) {
            break;
        }

        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++linesConsumed;
            ++context.lineNumber;
            continue;
        }

        if (!parseElementLine(line, mesh, context)) {
            context.addError("Failed to parse solid element: " + line);
        }

        ++linesConsumed;
        ++context.lineNumber;
    }

    return linesConsumed;
}

bool ElementSolidParser::parseElementLine(
    const std::string& line,
    core::Mesh& mesh,
    ParseContext& context)
{
    // Parse fields (8-char fields in LS-DYNA)
    auto fields = (context.format == KeywordFormat::FREE)
        ? LineParser::parseFree(line)
        : LineParser::parseFixed(line, 8);

    if (fields.size() < 2) {
        return false;
    }

    // Extract element data
    core::ElementId elemId = fields[0].toUnsigned();
    core::PartId partId = fields[1].toUnsigned();

    if (elemId == 0) {
        return false;
    }

    // Extract node IDs (up to 8 for hexahedron)
    std::vector<core::NodeId> nodeIds;
    for (size_t i = 2; i < std::min(fields.size(), size_t(10)); ++i) {
        core::NodeId nodeId = fields[i].toUnsigned();
        if (nodeId != 0) {
            nodeIds.push_back(nodeId);
        }
    }

    if (nodeIds.empty()) {
        return false;
    }

    // Detect element type
    core::ElementType elemType = detectElementType(nodeIds);

    try {
        auto element = core::ElementFactory::create(elemType, elemId, partId, nodeIds);
        mesh.addElement(std::move(element));
        ++context.elementsProcessed;
        return true;
    } catch (const std::exception& e) {
        context.addError(std::string("Failed to add element: ") + e.what());
        return false;
    }
}

core::ElementType ElementSolidParser::detectElementType(
    const std::vector<core::NodeId>& nodeIds)
{
    size_t nodeCount = nodeIds.size();

    switch (nodeCount) {
        case 4:  return core::ElementType::TETRAHEDRON;
        case 5:  return core::ElementType::PYRAMID;
        case 6:  return core::ElementType::PENTAHEDRON;
        case 8:  return core::ElementType::HEXAHEDRON;
        default: return core::ElementType::UNKNOWN;
    }
}

// ======================================================================
// ElementShellParser Implementation
// ======================================================================

size_t ElementShellParser::parse(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context)
{
    size_t linesConsumed = 0;

    for (const auto& line : lines) {
        if (LineParser::isKeyword(line)) {
            break;
        }

        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++linesConsumed;
            ++context.lineNumber;
            continue;
        }

        if (!parseElementLine(line, mesh, context)) {
            context.addError("Failed to parse shell element: " + line);
        }

        ++linesConsumed;
        ++context.lineNumber;
    }

    return linesConsumed;
}

bool ElementShellParser::parseElementLine(
    const std::string& line,
    core::Mesh& mesh,
    ParseContext& context)
{
    auto fields = (context.format == KeywordFormat::FREE)
        ? LineParser::parseFree(line)
        : LineParser::parseFixed(line, 8);

    if (fields.size() < 2) {
        return false;
    }

    core::ElementId elemId = fields[0].toUnsigned();
    core::PartId partId = fields[1].toUnsigned();

    if (elemId == 0) {
        return false;
    }

    // Extract node IDs (3 or 4 for shell)
    std::vector<core::NodeId> nodeIds;
    for (size_t i = 2; i < std::min(fields.size(), size_t(6)); ++i) {
        core::NodeId nodeId = fields[i].toUnsigned();
        if (nodeId != 0) {
            nodeIds.push_back(nodeId);
        }
    }

    if (nodeIds.size() < 3) {
        return false;
    }

    // Detect shell type
    core::ElementType elemType = (nodeIds.size() == 3)
        ? core::ElementType::TRIANGLE
        : core::ElementType::QUADRILATERAL;

    try {
        auto element = core::ElementFactory::create(elemType, elemId, partId, nodeIds);
        mesh.addElement(std::move(element));
        ++context.elementsProcessed;
        return true;
    } catch (const std::exception& e) {
        context.addError(std::string("Failed to add shell element: ") + e.what());
        return false;
    }
}

// ======================================================================
// ElementBeamParser Implementation
// ======================================================================

size_t ElementBeamParser::parse(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context)
{
    size_t linesConsumed = 0;

    for (const auto& line : lines) {
        if (LineParser::isKeyword(line)) {
            break;
        }

        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++linesConsumed;
            ++context.lineNumber;
            continue;
        }

        if (!parseElementLine(line, mesh, context)) {
            context.addError("Failed to parse beam element: " + line);
        }

        ++linesConsumed;
        ++context.lineNumber;
    }

    return linesConsumed;
}

bool ElementBeamParser::parseElementLine(
    const std::string& line,
    core::Mesh& mesh,
    ParseContext& context)
{
    auto fields = (context.format == KeywordFormat::FREE)
        ? LineParser::parseFree(line)
        : LineParser::parseFixed(line, 8);

    if (fields.size() < 4) {
        return false;
    }

    core::ElementId elemId = fields[0].toUnsigned();
    core::PartId partId = fields[1].toUnsigned();

    if (elemId == 0) {
        return false;
    }

    // Beam has 2 nodes
    std::vector<core::NodeId> nodeIds;
    nodeIds.push_back(fields[2].toUnsigned());
    nodeIds.push_back(fields[3].toUnsigned());

    if (nodeIds[0] == 0 || nodeIds[1] == 0) {
        return false;
    }

    try {
        auto element = std::make_unique<core::BeamElement>(
            elemId,
            partId,
            nodeIds);
        mesh.addElement(std::move(element));
        ++context.elementsProcessed;
        return true;
    } catch (const std::exception& e) {
        context.addError(std::string("Failed to add beam element: ") + e.what());
        return false;
    }
}

// ======================================================================
// PartParser Implementation
// ======================================================================

size_t PartParser::parse(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context)
{
    // *PART format:
    // Line 1: heading (part name)
    // Line 2: PID, SECID, MID, ...

    if (lines.empty()) {
        return 0;
    }

    size_t linesConsumed = 0;

    // First line is part heading (name)
    std::string partName;
    if (!LineParser::isKeyword(lines[0]) &&
        !LineParser::isComment(lines[0]) &&
        !LineParser::isBlank(lines[0])) {
        partName = LineParser::trim(lines[0]);
        ++linesConsumed;
        ++context.lineNumber;
    }

    // Second line contains part data
    if (linesConsumed < lines.size() &&
        !LineParser::isKeyword(lines[linesConsumed])) {

        auto fields = LineParser::parseFixed(lines[linesConsumed], 10);

        if (!fields.empty()) {
            core::PartId partId = fields[0].toUnsigned();

            if (partId != 0) {
                // Create part
                core::Part part(partId, partName);

                try {
                    mesh.addPart(part);
                    ++context.partsProcessed;
                } catch (const std::exception& e) {
                    context.addError(std::string("Failed to add part: ") + e.what());
                }
            }
        }

        ++linesConsumed;
        ++context.lineNumber;
    }

    return linesConsumed;
}

// ======================================================================
// KeywordParserRegistry Implementation
// ======================================================================

void KeywordParserRegistry::registerParser(std::unique_ptr<IKeywordParser> parser) {
    if (parser) {
        std::string keyword = parser->keywordName();
        m_parsers[keyword] = std::move(parser);
    }
}

IKeywordParser* KeywordParserRegistry::getParser(const std::string& keyword) {
    auto it = m_parsers.find(keyword);
    if (it != m_parsers.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool KeywordParserRegistry::hasParser(const std::string& keyword) const {
    return m_parsers.find(keyword) != m_parsers.end();
}

std::vector<std::string> KeywordParserRegistry::getRegisteredKeywords() const {
    std::vector<std::string> keywords;
    for (const auto& pair : m_parsers) {
        keywords.push_back(pair.first);
    }
    return keywords;
}

std::unique_ptr<KeywordParserRegistry> KeywordParserRegistry::createDefault() {
    auto registry = std::make_unique<KeywordParserRegistry>();

    // Register standard parsers
    registry->registerParser(std::make_unique<NodeParser>());
    registry->registerParser(std::make_unique<ElementSolidParser>());
    registry->registerParser(std::make_unique<ElementShellParser>());
    registry->registerParser(std::make_unique<ElementBeamParser>());
    registry->registerParser(std::make_unique<PartParser>());

    return registry;
}

} // namespace io
} // namespace koomesh
