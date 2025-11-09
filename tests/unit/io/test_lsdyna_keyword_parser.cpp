#include <gtest/gtest.h>
#include "io/LSDynaKeywordParser.h"
#include "core/Mesh.h"

using namespace koomesh::io;
using namespace koomesh::core;

// ======================================================================
// LineParser Tests
// ======================================================================

TEST(LineParserTest, IsKeyword) {
    EXPECT_TRUE(LineParser::isKeyword("*NODE"));
    EXPECT_TRUE(LineParser::isKeyword("*ELEMENT_SOLID"));
    EXPECT_TRUE(LineParser::isKeyword("  *PART  "));
    EXPECT_FALSE(LineParser::isKeyword("NODE"));
    EXPECT_FALSE(LineParser::isKeyword("$*NODE"));
    EXPECT_FALSE(LineParser::isKeyword(""));
}

TEST(LineParserTest, IsComment) {
    EXPECT_TRUE(LineParser::isComment("$ This is a comment"));
    EXPECT_TRUE(LineParser::isComment("# Another comment"));
    EXPECT_TRUE(LineParser::isComment("  $ Comment with leading space"));
    EXPECT_FALSE(LineParser::isComment("*NODE"));
    EXPECT_FALSE(LineParser::isComment("1234"));
    EXPECT_FALSE(LineParser::isComment(""));
}

TEST(LineParserTest, IsBlank) {
    EXPECT_TRUE(LineParser::isBlank(""));
    EXPECT_TRUE(LineParser::isBlank("   "));
    EXPECT_TRUE(LineParser::isBlank("\t\t"));
    EXPECT_FALSE(LineParser::isBlank("*NODE"));
    EXPECT_FALSE(LineParser::isBlank("  data  "));
}

TEST(LineParserTest, ExtractKeyword) {
    EXPECT_EQ("NODE", LineParser::extractKeyword("*NODE"));
    EXPECT_EQ("ELEMENT_SOLID", LineParser::extractKeyword("*ELEMENT_SOLID"));
    EXPECT_EQ("PART", LineParser::extractKeyword("  *PART  "));
    EXPECT_EQ("NODE", LineParser::extractKeyword("*NODE $ comment"));
    EXPECT_EQ("", LineParser::extractKeyword("NODE"));
    EXPECT_EQ("", LineParser::extractKeyword(""));
}

TEST(LineParserTest, ParseFixed) {
    std::string line = "       1      0.0000      1.0000      2.0000";

    auto fields = LineParser::parseFixed(line, 10);

    ASSERT_GE(fields.size(), 4);
    EXPECT_EQ("1", fields[0].raw);
    EXPECT_EQ("0.0000", fields[1].raw);
    EXPECT_EQ("1.0000", fields[2].raw);
    EXPECT_EQ("2.0000", fields[3].raw);
}

TEST(LineParserTest, ParseFixedWithEmpty) {
    std::string line = "       1          1.0          2.0";

    auto fields = LineParser::parseFixed(line, 10);

    ASSERT_GE(fields.size(), 3);
    EXPECT_EQ("1", fields[0].raw);
    EXPECT_TRUE(fields[1].isEmpty);
    EXPECT_EQ("1.0", fields[2].raw);
}

TEST(LineParserTest, ParseFreeComma) {
    std::string line = "1, 0.0, 1.0, 2.0";

    auto fields = LineParser::parseFree(line);

    ASSERT_EQ(4, fields.size());
    EXPECT_EQ("1", fields[0].raw);
    EXPECT_EQ("0.0", fields[1].raw);
    EXPECT_EQ("1.0", fields[2].raw);
    EXPECT_EQ("2.0", fields[3].raw);
}

TEST(LineParserTest, ParseFreeWhitespace) {
    std::string line = "1  0.0  1.0  2.0";

    auto fields = LineParser::parseFree(line);

    ASSERT_EQ(4, fields.size());
    EXPECT_EQ("1", fields[0].raw);
    EXPECT_EQ("0.0", fields[1].raw);
    EXPECT_EQ("1.0", fields[2].raw);
    EXPECT_EQ("2.0", fields[3].raw);
}

TEST(LineParserTest, Trim) {
    EXPECT_EQ("text", LineParser::trim("  text  "));
    EXPECT_EQ("text", LineParser::trim("text"));
    EXPECT_EQ("text", LineParser::trim("\t\ttext\n"));
    EXPECT_EQ("", LineParser::trim("   "));
    EXPECT_EQ("", LineParser::trim(""));
}

TEST(LineParserTest, Split) {
    auto tokens = LineParser::split("a,b,c,d", ',');

    ASSERT_EQ(4, tokens.size());
    EXPECT_EQ("a", tokens[0]);
    EXPECT_EQ("b", tokens[1]);
    EXPECT_EQ("c", tokens[2]);
    EXPECT_EQ("d", tokens[3]);
}

// ======================================================================
// ParsedField Tests
// ======================================================================

TEST(ParsedFieldTest, ToInt) {
    ParsedField field1;
    field1.raw = "42";
    field1.isEmpty = false;
    EXPECT_EQ(42, field1.toInt());

    ParsedField field2;
    field2.raw = "-123";
    field2.isEmpty = false;
    EXPECT_EQ(-123, field2.toInt());

    ParsedField field3;
    field3.isEmpty = true;
    EXPECT_EQ(0, field3.toInt());
}

TEST(ParsedFieldTest, ToDouble) {
    ParsedField field1;
    field1.raw = "3.14159";
    field1.isEmpty = false;
    EXPECT_DOUBLE_EQ(3.14159, field1.toDouble());

    ParsedField field2;
    field2.raw = "-2.5";
    field2.isEmpty = false;
    EXPECT_DOUBLE_EQ(-2.5, field2.toDouble());

    ParsedField field3;
    field3.isEmpty = true;
    EXPECT_DOUBLE_EQ(0.0, field3.toDouble());
}

TEST(ParsedFieldTest, ToUnsigned) {
    ParsedField field1;
    field1.raw = "100";
    field1.isEmpty = false;
    EXPECT_EQ(100u, field1.toUnsigned());

    ParsedField field2;
    field2.isEmpty = true;
    EXPECT_EQ(0u, field2.toUnsigned());
}

// ======================================================================
// NodeParser Tests
// ======================================================================

class NodeParserTest : public ::testing::Test {
protected:
    Mesh mesh;
    ParseContext context;
    NodeParser parser;
};

TEST_F(NodeParserTest, ParseSingleNode) {
    std::vector<std::string> lines = {
        "       1      0.0000      1.0000      2.0000"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.nodesProcessed);
    EXPECT_EQ(1, mesh.nodeCount());

    const Node* node = mesh.getNode(1);
    ASSERT_NE(nullptr, node);
    EXPECT_DOUBLE_EQ(0.0, node->x());
    EXPECT_DOUBLE_EQ(1.0, node->y());
    EXPECT_DOUBLE_EQ(2.0, node->z());
}

TEST_F(NodeParserTest, ParseMultipleNodes) {
    std::vector<std::string> lines = {
        "       1      0.0000      0.0000      0.0000",
        "       2      1.0000      0.0000      0.0000",
        "       3      0.0000      1.0000      0.0000"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(3, consumed);
    EXPECT_EQ(3, context.nodesProcessed);
    EXPECT_EQ(3, mesh.nodeCount());
}

TEST_F(NodeParserTest, SkipComments) {
    std::vector<std::string> lines = {
        "$ This is a comment",
        "       1      0.0000      0.0000      0.0000",
        "# Another comment",
        "       2      1.0000      0.0000      0.0000"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(4, consumed);
    EXPECT_EQ(2, context.nodesProcessed);
    EXPECT_EQ(2, mesh.nodeCount());
}

TEST_F(NodeParserTest, StopAtKeyword) {
    std::vector<std::string> lines = {
        "       1      0.0000      0.0000      0.0000",
        "       2      1.0000      0.0000      0.0000",
        "*ELEMENT_SOLID",
        "      10       1       1       2       3       4"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(2, consumed);
    EXPECT_EQ(2, context.nodesProcessed);
}

// ======================================================================
// ElementSolidParser Tests
// ======================================================================

class ElementSolidParserTest : public ::testing::Test {
protected:
    Mesh mesh;
    ParseContext context;
    ElementSolidParser parser;

    void SetUp() override {
        // Add nodes for elements
        mesh.addNode(std::make_unique<Node>(1, 0, 0, 0));
        mesh.addNode(std::make_unique<Node>(2, 1, 0, 0));
        mesh.addNode(std::make_unique<Node>(3, 0, 1, 0));
        mesh.addNode(std::make_unique<Node>(4, 0, 0, 1));
        mesh.addNode(std::make_unique<Node>(5, 1, 1, 0));
        mesh.addNode(std::make_unique<Node>(6, 1, 0, 1));
        mesh.addNode(std::make_unique<Node>(7, 0, 1, 1));
        mesh.addNode(std::make_unique<Node>(8, 1, 1, 1));
    }
};

TEST_F(ElementSolidParserTest, ParseTetrahedron) {
    std::vector<std::string> lines = {
        "      10       1       1       2       3       4"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.elementsProcessed);
    EXPECT_EQ(1, mesh.elementCount());

    const Element* elem = mesh.getElement(10);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TETRAHEDRON, elem->type());
    EXPECT_EQ(4, elem->nodeCount());
}

TEST_F(ElementSolidParserTest, ParseHexahedron) {
    std::vector<std::string> lines = {
        "      20       1       1       2       5       3       4       6       8       7"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.elementsProcessed);

    const Element* elem = mesh.getElement(20);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::HEXAHEDRON, elem->type());
    EXPECT_EQ(8, elem->nodeCount());
}

// ======================================================================
// ElementShellParser Tests
// ======================================================================

class ElementShellParserTest : public ::testing::Test {
protected:
    Mesh mesh;
    ParseContext context;
    ElementShellParser parser;

    void SetUp() override {
        mesh.addNode(std::make_unique<Node>(1, 0, 0, 0));
        mesh.addNode(std::make_unique<Node>(2, 1, 0, 0));
        mesh.addNode(std::make_unique<Node>(3, 1, 1, 0));
        mesh.addNode(std::make_unique<Node>(4, 0, 1, 0));
    }
};

TEST_F(ElementShellParserTest, ParseTriangle) {
    std::vector<std::string> lines = {
        "     100       2       1       2       3"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.elementsProcessed);

    const Element* elem = mesh.getElement(100);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TRIANGLE, elem->type());
    EXPECT_EQ(3, elem->nodeCount());
}

TEST_F(ElementShellParserTest, ParseQuadrilateral) {
    std::vector<std::string> lines = {
        "     200       2       1       2       3       4"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.elementsProcessed);

    const Element* elem = mesh.getElement(200);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::QUADRILATERAL, elem->type());
    EXPECT_EQ(4, elem->nodeCount());
}

// ======================================================================
// ElementBeamParser Tests
// ======================================================================

class ElementBeamParserTest : public ::testing::Test {
protected:
    Mesh mesh;
    ParseContext context;
    ElementBeamParser parser;

    void SetUp() override {
        mesh.addNode(std::make_unique<Node>(1, 0, 0, 0));
        mesh.addNode(std::make_unique<Node>(2, 1, 0, 0));
    }
};

TEST_F(ElementBeamParserTest, ParseBeam) {
    std::vector<std::string> lines = {
        "     300       3       1       2"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(1, consumed);
    EXPECT_EQ(1, context.elementsProcessed);

    const Element* elem = mesh.getElement(300);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::BEAM, elem->type());
    EXPECT_EQ(2, elem->nodeCount());
}

// ======================================================================
// PartParser Tests
// ======================================================================

class PartParserTest : public ::testing::Test {
protected:
    Mesh mesh;
    ParseContext context;
    PartParser parser;
};

TEST_F(PartParserTest, ParsePart) {
    std::vector<std::string> lines = {
        "Steel Plate",
        "         1         1         1"
    };

    size_t consumed = parser.parse(lines, mesh, context);

    EXPECT_EQ(2, consumed);
    EXPECT_EQ(1, context.partsProcessed);
    EXPECT_EQ(1, mesh.partCount());

    const Part* part = mesh.getPart(1);
    ASSERT_NE(nullptr, part);
    EXPECT_EQ("Steel Plate", part->name());
}

// ======================================================================
// KeywordParserRegistry Tests
// ======================================================================

TEST(KeywordParserRegistryTest, CreateDefault) {
    auto registry = KeywordParserRegistry::createDefault();

    EXPECT_TRUE(registry->hasParser("NODE"));
    EXPECT_TRUE(registry->hasParser("ELEMENT_SOLID"));
    EXPECT_TRUE(registry->hasParser("ELEMENT_SHELL"));
    EXPECT_TRUE(registry->hasParser("ELEMENT_BEAM"));
    EXPECT_TRUE(registry->hasParser("PART"));
    EXPECT_FALSE(registry->hasParser("UNKNOWN_KEYWORD"));
}

TEST(KeywordParserRegistryTest, GetParser) {
    auto registry = KeywordParserRegistry::createDefault();

    IKeywordParser* nodeParser = registry->getParser("NODE");
    ASSERT_NE(nullptr, nodeParser);
    EXPECT_EQ("NODE", nodeParser->keywordName());

    IKeywordParser* unknownParser = registry->getParser("UNKNOWN");
    EXPECT_EQ(nullptr, unknownParser);
}

TEST(KeywordParserRegistryTest, RegisterCustomParser) {
    auto registry = std::make_unique<KeywordParserRegistry>();

    auto customParser = std::make_unique<NodeParser>();
    registry->registerParser(std::move(customParser));

    EXPECT_TRUE(registry->hasParser("NODE"));
}

TEST(KeywordParserRegistryTest, GetRegisteredKeywords) {
    auto registry = KeywordParserRegistry::createDefault();

    auto keywords = registry->getRegisteredKeywords();

    EXPECT_GE(keywords.size(), 5);
    EXPECT_NE(std::find(keywords.begin(), keywords.end(), "NODE"), keywords.end());
    EXPECT_NE(std::find(keywords.begin(), keywords.end(), "ELEMENT_SOLID"), keywords.end());
}

// ======================================================================
// ParseContext Tests
// ======================================================================

TEST(ParseContextTest, AddError) {
    ParseContext context;
    context.lineNumber = 42;

    context.addError("Test error");

    ASSERT_EQ(1, context.errors.size());
    EXPECT_NE(std::string::npos, context.errors[0].find("Line 42"));
    EXPECT_NE(std::string::npos, context.errors[0].find("Test error"));
}

TEST(ParseContextTest, AddWarning) {
    ParseContext context;
    context.lineNumber = 10;

    context.addWarning("Test warning");

    ASSERT_EQ(1, context.warnings.size());
    EXPECT_NE(std::string::npos, context.warnings[0].find("Line 10"));
    EXPECT_NE(std::string::npos, context.warnings[0].find("Test warning"));
}

// ======================================================================
// Integration Tests
// ======================================================================

TEST(IntegrationTest, ParseCompleteKeywordFile) {
    std::vector<std::string> fileLines = {
        "*KEYWORD",
        "$",
        "$ Simple test mesh",
        "$",
        "*NODE",
        "       1      0.0000      0.0000      0.0000",
        "       2      1.0000      0.0000      0.0000",
        "       3      0.0000      1.0000      0.0000",
        "       4      0.0000      0.0000      1.0000",
        "*ELEMENT_SOLID",
        "      10       1       1       2       3       4",
        "*ELEMENT_SHELL",
        "     100       2       1       2       3",
        "*END"
    };

    Mesh mesh;
    ParseContext context;
    auto registry = KeywordParserRegistry::createDefault();

    size_t currentLine = 0;
    while (currentLine < fileLines.size()) {
        const std::string& line = fileLines[currentLine];

        if (LineParser::isKeyword(line)) {
            std::string keyword = LineParser::extractKeyword(line);

            IKeywordParser* parser = registry->getParser(keyword);
            if (parser) {
                // Gather remaining lines for this keyword
                std::vector<std::string> keywordLines(
                    fileLines.begin() + currentLine + 1,
                    fileLines.end());

                size_t consumed = parser->parse(keywordLines, mesh, context);
                currentLine += 1 + consumed;
                continue;
            }
        }

        ++currentLine;
    }

    // Verify results
    EXPECT_EQ(4, mesh.nodeCount());
    EXPECT_EQ(2, mesh.elementCount());
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
