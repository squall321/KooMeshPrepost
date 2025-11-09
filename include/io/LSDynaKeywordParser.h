#pragma once

#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include "io/FileIOException.h"
#include "io/ErrorHandler.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace koomesh {
namespace io {

/**
 * @brief LS-DYNA keyword format type
 */
enum class KeywordFormat {
    FIXED,      ///< Fixed format (8-character fields)
    FREE,       ///< Free format (comma or whitespace separated)
    AUTO        ///< Auto-detect format
};

/**
 * @brief Parse context for tracking parser state
 */
struct ParseContext {
    std::string filename;
    size_t lineNumber = 0;
    std::string currentKeyword;
    KeywordFormat format = KeywordFormat::AUTO;

    // Statistics
    size_t nodesProcessed = 0;
    size_t elementsProcessed = 0;
    size_t partsProcessed = 0;

    // Error handling (optional)
    ErrorHandler* errorHandler = nullptr;

    // Errors and warnings (legacy - use errorHandler if available)
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void addError(const std::string& message) {
        if (errorHandler) {
            errorHandler->handleError(
                ErrorSeverity::ERROR,
                message,
                filename,
                lineNumber,
                currentKeyword
            );
        } else {
            errors.push_back("Line " + std::to_string(lineNumber) + ": " + message);
        }
    }

    void addWarning(const std::string& message) {
        if (errorHandler) {
            errorHandler->handleError(
                ErrorSeverity::WARNING,
                message,
                filename,
                lineNumber,
                currentKeyword
            );
        } else {
            warnings.push_back("Line " + std::to_string(lineNumber) + ": " + message);
        }
    }

    bool shouldAbort() const {
        return errorHandler && errorHandler->shouldAbort();
    }
};

/**
 * @brief Parsed field value from LS-DYNA file
 */
struct ParsedField {
    std::string raw;        ///< Raw string value
    bool isEmpty = false;   ///< True if field is empty/whitespace

    // Conversion methods
    int toInt() const;
    double toDouble() const;
    size_t toUnsigned() const;

    bool isValid() const { return !isEmpty; }
};

/**
 * @brief LS-DYNA line parser utilities
 */
class LineParser {
public:
    /**
     * @brief Check if line is a keyword (starts with *)
     */
    static bool isKeyword(const std::string& line);

    /**
     * @brief Check if line is a comment (starts with $ or #)
     */
    static bool isComment(const std::string& line);

    /**
     * @brief Check if line is blank
     */
    static bool isBlank(const std::string& line);

    /**
     * @brief Extract keyword name from line
     *
     * @param line Keyword line (e.g., "*NODE" or "*ELEMENT_SOLID")
     * @return Keyword name without asterisk
     */
    static std::string extractKeyword(const std::string& line);

    /**
     * @brief Parse fixed format line (8-character fields)
     *
     * @param line Input line
     * @param fieldWidth Width of each field (default: 10)
     * @return Vector of parsed fields
     */
    static std::vector<ParsedField> parseFixed(
        const std::string& line,
        size_t fieldWidth = 10);

    /**
     * @brief Parse free format line (comma or space separated)
     *
     * @param line Input line
     * @return Vector of parsed fields
     */
    static std::vector<ParsedField> parseFree(const std::string& line);

    /**
     * @brief Auto-detect format and parse line
     *
     * @param line Input line
     * @return Vector of parsed fields
     */
    static std::vector<ParsedField> parseAuto(const std::string& line);

    /**
     * @brief Trim whitespace from string
     */
    static std::string trim(const std::string& str);

    /**
     * @brief Split string by delimiter
     */
    static std::vector<std::string> split(
        const std::string& str,
        char delimiter = ',');
};

/**
 * @brief Base class for keyword parsers
 */
class IKeywordParser {
public:
    virtual ~IKeywordParser() = default;

    /**
     * @brief Get keyword name this parser handles
     */
    virtual std::string keywordName() const = 0;

    /**
     * @brief Parse keyword data
     *
     * @param lines Lines of keyword data (not including keyword line itself)
     * @param mesh Target mesh to populate
     * @param context Parse context
     * @return Number of lines consumed
     */
    virtual size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) = 0;
};

/**
 * @brief Parser for *NODE keyword
 */
class NodeParser : public IKeywordParser {
public:
    std::string keywordName() const override { return "NODE"; }

    size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) override;

private:
    bool parseNodeLine(
        const std::string& line,
        core::Mesh& mesh,
        ParseContext& context);
};

/**
 * @brief Parser for *ELEMENT_SOLID keyword
 */
class ElementSolidParser : public IKeywordParser {
public:
    std::string keywordName() const override { return "ELEMENT_SOLID"; }

    size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) override;

private:
    bool parseElementLine(
        const std::string& line,
        core::Mesh& mesh,
        ParseContext& context);

    core::ElementType detectElementType(const std::vector<core::NodeId>& nodeIds);
};

/**
 * @brief Parser for *ELEMENT_SHELL keyword
 */
class ElementShellParser : public IKeywordParser {
public:
    std::string keywordName() const override { return "ELEMENT_SHELL"; }

    size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) override;

private:
    bool parseElementLine(
        const std::string& line,
        core::Mesh& mesh,
        ParseContext& context);
};

/**
 * @brief Parser for *ELEMENT_BEAM keyword
 */
class ElementBeamParser : public IKeywordParser {
public:
    std::string keywordName() const override { return "ELEMENT_BEAM"; }

    size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) override;

private:
    bool parseElementLine(
        const std::string& line,
        core::Mesh& mesh,
        ParseContext& context);
};

/**
 * @brief Parser for *PART keyword
 */
class PartParser : public IKeywordParser {
public:
    std::string keywordName() const override { return "PART"; }

    size_t parse(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context) override;
};

/**
 * @brief Main LS-DYNA keyword parser registry
 */
class KeywordParserRegistry {
public:
    /**
     * @brief Register a keyword parser
     */
    void registerParser(std::unique_ptr<IKeywordParser> parser);

    /**
     * @brief Get parser for keyword
     *
     * @param keyword Keyword name (without asterisk)
     * @return Parser, or nullptr if not found
     */
    IKeywordParser* getParser(const std::string& keyword);

    /**
     * @brief Check if keyword is supported
     */
    bool hasParser(const std::string& keyword) const;

    /**
     * @brief Get all registered keyword names
     */
    std::vector<std::string> getRegisteredKeywords() const;

    /**
     * @brief Create default registry with standard parsers
     */
    static std::unique_ptr<KeywordParserRegistry> createDefault();

private:
    std::unordered_map<std::string, std::unique_ptr<IKeywordParser>> m_parsers;
};

} // namespace io
} // namespace koomesh
