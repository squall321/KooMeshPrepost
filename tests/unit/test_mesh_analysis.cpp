#include <gtest/gtest.h>
#include "core/MeshValidator.h"
#include "core/MeshStatistics.h"
#include "helpers/MeshBuilder.h"
#include "helpers/TestUtils.h"

using namespace koomesh::core;
using namespace koomesh::test;

// ======================================================================
// MeshValidator Tests
// ======================================================================

class MeshValidatorTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;
};

TEST_F(MeshValidatorTest, ValidMesh) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;
    auto result = validator.validate(*mesh, validationOpts);

    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(0, result.errorCount());
}

TEST_F(MeshValidatorTest, MissingNodeReference) {
    mesh = std::make_unique<Mesh>();

    // Add element with non-existent nodes
    auto elem = std::make_unique<TetrahedronElement>(1, 1, std::vector<NodeId>{1, 2, 3, 4});

    // This should fail - nodes don't exist
    EXPECT_THROW(mesh->addElement(std::move(elem)), std::exception);
}

TEST_F(MeshValidatorTest, IsolatedNodes) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addNode(5, 10, 10, 10)  // Isolated node
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;
    auto result = validator.validate(*mesh, validationOpts);

    // Should have warning about isolated node
    EXPECT_GT(result.warningCount(), 0);
}

TEST_F(MeshValidatorTest, DegenerateElement) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 0, 0, 0)  // Same position as node 1
        .addNode(3, 0, 0, 0)  // Same position as node 1
        .addNode(4, 0, 0, 0)  // Same position as node 1
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;
    auto result = validator.validate(*mesh, validationOpts);

    // Should detect degenerate element
    EXPECT_FALSE(result.isValid);
    EXPECT_GT(result.errorCount(), 0);
}

TEST_F(MeshValidatorTest, LowQualityElements) {
    // Create a very skewed tetrahedron
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 100, 0, 0)  // Very long edge
        .addNode(3, 0, 0.1, 0)   // Very short edges
        .addNode(4, 0, 0, 0.1)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    MeshValidator::ValidationOptions options;
    options.minQualityThreshold = 0.5;

    MeshValidator validator;
    auto result = validator.validate(*mesh, options);

    // Should have quality warning
    EXPECT_GT(result.warningCount(), 0);
}

TEST_F(MeshValidatorTest, EmptyMesh) {
    mesh = std::make_unique<Mesh>();

    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;
    auto result = validator.validate(*mesh, validationOpts);

    EXPECT_TRUE(result.isValid);  // Empty is valid
    EXPECT_GT(result.warningCount(), 0);  // But should warn
}

TEST_F(MeshValidatorTest, ValidationResultMethods) {
    MeshValidator::ValidationResult result;

    result.addError("Error 1");
    result.addError("Error 2");
    result.addWarning("Warning 1");
    result.addInfo("Info 1");

    EXPECT_EQ(2, result.errorCount());
    EXPECT_EQ(1, result.warningCount());
    EXPECT_EQ(1, result.infoCount());
    EXPECT_EQ(3, result.totalIssueCount());
    EXPECT_FALSE(result.isValid);
}

// ======================================================================
// MeshStatistics Tests
// ======================================================================

class MeshStatisticsTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;
};

TEST_F(MeshStatisticsTest, BasicCounts) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(4, stats.totalNodes);
    EXPECT_EQ(1, stats.totalElements);
}

TEST_F(MeshStatisticsTest, ElementTypeCounts) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addNode(5, 1, 1, 0)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .addTriangle(2, 1, {1, 2, 3})
        .addTriangle(3, 1, {2, 3, 5})
        .build();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(3, stats.totalElements);
    EXPECT_EQ(1, stats.getElementCount(ElementType::TETRAHEDRON));
    EXPECT_EQ(2, stats.getElementCount(ElementType::TRIANGLE));
}

TEST_F(MeshStatisticsTest, QualityStatistics) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_GT(stats.avgElementQuality, 0.0);
    EXPECT_LE(stats.avgElementQuality, 1.0);
    EXPECT_GE(stats.minElementQuality, 0.0);
    EXPECT_LE(stats.maxElementQuality, 1.0);
}

TEST_F(MeshStatisticsTest, VolumeStatistics) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_GT(stats.totalVolume, 0.0);
    EXPECT_GT(stats.avgElementVolume, 0.0);
    EXPECT_EQ(stats.minElementVolume, stats.maxElementVolume);  // Only one element
}

TEST_F(MeshStatisticsTest, BoundingBox) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 10, 0, 0)
        .addNode(3, 0, 20, 0)
        .addNode(4, 0, 0, 30)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_DOUBLE_EQ(0.0, stats.globalBounds.minX);
    EXPECT_DOUBLE_EQ(10.0, stats.globalBounds.maxX);
    EXPECT_DOUBLE_EQ(0.0, stats.globalBounds.minY);
    EXPECT_DOUBLE_EQ(20.0, stats.globalBounds.maxY);
    EXPECT_DOUBLE_EQ(0.0, stats.globalBounds.minZ);
    EXPECT_DOUBLE_EQ(30.0, stats.globalBounds.maxZ);
}

TEST_F(MeshStatisticsTest, ConnectivityStatistics) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addNode(5, 1, 1, 1)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .addTetrahedron(2, 1, {2, 3, 4, 5})
        .build();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    // Nodes 2, 3, 4 are connected to 2 elements
    EXPECT_EQ(2, stats.maxNodeConnections);
    // Node 1 and 5 are connected to 1 element
    EXPECT_EQ(1, stats.minNodeConnections);
}

TEST_F(MeshStatisticsTest, PartStatistics) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addNode(5, 1, 1, 1)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .addTetrahedron(2, 1, {2, 3, 4, 5})
        .addTetrahedron(3, 2, {1, 2, 4, 5})
        .build();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(2, stats.partElementCounts[1]);
    EXPECT_EQ(1, stats.partElementCounts[2]);
}

TEST_F(MeshStatisticsTest, EmptyMesh) {
    mesh = std::make_unique<Mesh>();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(0, stats.totalNodes);
    EXPECT_EQ(0, stats.totalElements);
    EXPECT_EQ(0, stats.totalParts);
}

TEST_F(MeshStatisticsTest, ReportGeneration) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);
    std::string report = stats.generateReport();

    // Should contain key sections
    EXPECT_STRING_CONTAINS(report, "MESH STATISTICS REPORT");
    EXPECT_STRING_CONTAINS(report, "BASIC COUNTS");
    EXPECT_STRING_CONTAINS(report, "ELEMENT TYPES");
    EXPECT_STRING_CONTAINS(report, "BOUNDING BOX");
}

TEST_F(MeshStatisticsTest, SummaryString) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);
    std::string summary = stats.summary();

    EXPECT_STRING_CONTAINS(summary, "4 nodes");
    EXPECT_STRING_CONTAINS(summary, "1 elements");
}

TEST_F(MeshStatisticsTest, ElementTypeName) {
    EXPECT_EQ("Tetrahedron", MeshStatistics::Statistics::getElementTypeName(ElementType::TETRAHEDRON));
    EXPECT_EQ("Hexahedron", MeshStatistics::Statistics::getElementTypeName(ElementType::HEXAHEDRON));
    EXPECT_EQ("Triangle Shell", MeshStatistics::Statistics::getElementTypeName(ElementType::TRIANGLE));
    EXPECT_EQ("Beam", MeshStatistics::Statistics::getElementTypeName(ElementType::BEAM));
}

TEST_F(MeshStatisticsTest, StatisticsOptions) {
    mesh = SimpleMeshes::singleTetrahedron();

    MeshStatistics::Options options;
    options.computeQuality = false;
    options.computeVolume = false;
    options.computeConnectivity = false;
    options.computePartStats = false;

    auto stats = MeshStatistics::compute(*mesh, options);

    // Quality should not be computed
    EXPECT_DOUBLE_EQ(0.0, stats.avgElementQuality);
    EXPECT_DOUBLE_EQ(0.0, stats.totalVolume);
    EXPECT_DOUBLE_EQ(0.0, stats.avgNodeConnections);
    EXPECT_TRUE(stats.partElementCounts.empty());
}

// ======================================================================
// Integration Tests
// ======================================================================

class MeshAnalysisIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;
};

TEST_F(MeshAnalysisIntegrationTest, ValidateAndAnalyze) {
    mesh = SimpleMeshes::singleHexahedron();

    // Validate first
    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;
    auto validation = validator.validate(*mesh, validationOpts);

    EXPECT_TRUE(validation.isValid);

    // Then analyze
    MeshStatistics::Options statsOpts;
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(8, stats.totalNodes);
    EXPECT_EQ(1, stats.totalElements);
    EXPECT_GT(stats.avgElementQuality, 0.0);
}

TEST_F(MeshAnalysisIntegrationTest, ComplexMesh) {
    mesh = MeshBuilder()
        .addGridNodes(5, 5, 1.0)
        .build();

    // Add some elements
    for (size_t i = 1; i <= 16; ++i) {
        mesh->addElement(std::make_unique<TetrahedronElement>(
            i, 1, std::vector<NodeId>{i, i+1, i+5, i+6}));
    }

    // Validate
    MeshValidator validator;
    MeshValidator::ValidationOptions validationOpts;  // Use default options
    auto validation = validator.validate(*mesh, validationOpts);

    // Analyze
    MeshStatistics::Options statsOpts;  // Use default options
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(25, stats.totalNodes);
    EXPECT_EQ(16, stats.totalElements);

    // Print report
    std::string report = stats.generateReport();
    // std::cout << report << std::endl;  // Uncomment to see report
}

TEST_F(MeshAnalysisIntegrationTest, MixedElementTypes) {
    mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addNode(5, 2, 0, 0)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .addTriangle(2, 2, {1, 2, 3})
        .addBeam(3, 3, {2, 5})
        .build();

    MeshStatistics::Options statsOpts;  // Use default options
    auto stats = MeshStatistics::compute(*mesh, statsOpts);

    EXPECT_EQ(3, stats.totalElements);
    EXPECT_EQ(1, stats.getElementCount(ElementType::TETRAHEDRON));
    EXPECT_EQ(1, stats.getElementCount(ElementType::TRIANGLE));
    EXPECT_EQ(1, stats.getElementCount(ElementType::BEAM));

    EXPECT_EQ(3, stats.partElementCounts.size());
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
