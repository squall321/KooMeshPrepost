/**
 * @file test_mesh_to_vtk.cpp
 * @brief Unit tests for MeshToVTK conversion
 */

#include <gtest/gtest.h>
#include "visualization/MeshToVTK.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::visualization;
using namespace koomesh::core;

/**
 * @brief Test fixture for MeshToVTK tests
 */
class MeshToVTKTest : public ::testing::Test {
protected:
    void SetUp() override {
        createTestMesh();
    }

    void createTestMesh() {
        mesh = std::make_unique<Mesh>();

        // Create a simple 2x2x2 hex mesh
        NodeId nodeId = 1;
        for (int k = 0; k < 3; ++k) {
            for (int j = 0; j < 3; ++j) {
                for (int i = 0; i < 3; ++i) {
                    Node node(nodeId++, Eigen::Vector3d(
                        static_cast<double>(i),
                        static_cast<double>(j),
                        static_cast<double>(k)
                    ));
                    mesh->addNode(node);
                }
            }
        }

        // Create 8 hexahedral elements
        ElementId elemId = 1;
        for (int k = 0; k < 2; ++k) {
            for (int j = 0; j < 2; ++j) {
                for (int i = 0; i < 2; ++i) {
                    NodeId n000 = 1 + i + j * 3 + k * 9;
                    NodeId n100 = n000 + 1;
                    NodeId n010 = n000 + 3;
                    NodeId n110 = n010 + 1;
                    NodeId n001 = n000 + 9;
                    NodeId n101 = n001 + 1;
                    NodeId n011 = n001 + 3;
                    NodeId n111 = n011 + 1;

                    std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                    HexahedronElement elem(elemId++, 1, nodes);
                    mesh->addElement(std::make_unique<HexahedronElement>(elem));
                }
            }
        }
    }

    std::unique_ptr<Mesh> mesh;
};

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// VTK Available Tests
// ============================================================================

TEST_F(MeshToVTKTest, ConvertToUnstructuredGrid) {
    ConversionStats stats;
    auto grid = MeshToVTK::convert(*mesh, ConversionOptions(), &stats);

    ASSERT_NE(grid, nullptr);
    EXPECT_EQ(stats.nodesConverted, mesh->nodeCount());
    EXPECT_EQ(stats.elementsConverted, mesh->elementCount());
    EXPECT_GT(stats.conversionTime, 0.0);

    // Verify VTK grid properties
    EXPECT_EQ(grid->GetNumberOfPoints(), static_cast<vtkIdType>(mesh->nodeCount()));
    EXPECT_EQ(grid->GetNumberOfCells(), static_cast<vtkIdType>(mesh->elementCount()));
}

TEST_F(MeshToVTKTest, ConvertToPolyData) {
    ConversionOptions options;
    options.computeNormals = true;

    auto polyData = MeshToVTK::convertToPolyData(*mesh, options);

    ASSERT_NE(polyData, nullptr);
    EXPECT_GT(polyData->GetNumberOfPoints(), 0);
}

TEST_F(MeshToVTKTest, ConvertToPolyDataWithoutNormals) {
    ConversionOptions options;
    options.computeNormals = false;

    auto polyData = MeshToVTK::convertToPolyData(*mesh, options);

    ASSERT_NE(polyData, nullptr);
    EXPECT_GT(polyData->GetNumberOfPoints(), 0);
}

TEST_F(MeshToVTKTest, ConvertToActor) {
    auto actor = MeshToVTK::convertToActor(*mesh);

    ASSERT_NE(actor, nullptr);
    EXPECT_NE(actor->GetMapper(), nullptr);
}

TEST_F(MeshToVTKTest, ConvertGroupToActor) {
    std::vector<ElementId> selectedIds = {1, 3, 5};

    auto actor = MeshToVTK::convertGroupToActor(*mesh, selectedIds);

    ASSERT_NE(actor, nullptr);
    EXPECT_NE(actor->GetMapper(), nullptr);
}

TEST_F(MeshToVTKTest, ExtractSurface) {
    auto surface = MeshToVTK::extractSurface(*mesh);

    ASSERT_NE(surface, nullptr);
    EXPECT_GT(surface->GetNumberOfPoints(), 0);
    EXPECT_GT(surface->GetNumberOfCells(), 0);
}

TEST_F(MeshToVTKTest, ConvertEdges) {
    auto edges = MeshToVTK::convertEdges(*mesh);

    ASSERT_NE(edges, nullptr);
    // Edges polydata should have points and lines
    EXPECT_GT(edges->GetNumberOfPoints(), 0);
}

TEST_F(MeshToVTKTest, ConversionWithOptions) {
    ConversionOptions options;
    options.includeInternalFaces = true;
    options.computeNormals = true;
    options.generateEdges = true;
    options.edgeAngle = 45.0;

    auto actor = MeshToVTK::convertToActor(*mesh, options);

    ASSERT_NE(actor, nullptr);
}

TEST_F(MeshToVTKTest, EmptyMeshConversion) {
    Mesh emptyMesh;

    auto grid = MeshToVTK::convert(emptyMesh);

    ASSERT_NE(grid, nullptr);
    EXPECT_EQ(grid->GetNumberOfPoints(), 0);
    EXPECT_EQ(grid->GetNumberOfCells(), 0);
}

TEST_F(MeshToVTKTest, SingleElementConversion) {
    Mesh singleElemMesh;

    // Add 8 nodes for one hexahedron
    for (NodeId i = 1; i <= 8; ++i) {
        Node node(i, Eigen::Vector3d(
            static_cast<double>(i % 2),
            static_cast<double>((i / 2) % 2),
            static_cast<double>(i / 4)
        ));
        singleElemMesh.addNode(node);
    }

    std::vector<NodeId> nodes = {1, 2, 3, 4, 5, 6, 7, 8};
    HexahedronElement elem(1, 1, nodes);
    singleElemMesh.addElement(std::make_unique<HexahedronElement>(elem));

    auto grid = MeshToVTK::convert(singleElemMesh);

    ASSERT_NE(grid, nullptr);
    EXPECT_EQ(grid->GetNumberOfPoints(), 8);
    EXPECT_EQ(grid->GetNumberOfCells(), 1);
}

TEST_F(MeshToVTKTest, ConversionStatistics) {
    ConversionStats stats;

    MeshToVTK::convert(*mesh, ConversionOptions(), &stats);

    EXPECT_EQ(stats.nodesConverted, mesh->nodeCount());
    EXPECT_EQ(stats.elementsConverted, mesh->elementCount());
    EXPECT_GT(stats.conversionTime, 0.0);
    EXPECT_LT(stats.conversionTime, 1.0);  // Should be fast
}

TEST_F(MeshToVTKTest, MultipleConversions) {
    // Convert same mesh multiple times
    for (int i = 0; i < 5; ++i) {
        auto grid = MeshToVTK::convert(*mesh);
        ASSERT_NE(grid, nullptr);
        EXPECT_EQ(grid->GetNumberOfCells(), static_cast<vtkIdType>(mesh->elementCount()));
    }
}

TEST_F(MeshToVTKTest, ActorProperties) {
    auto actor = MeshToVTK::convertToActor(*mesh);

    ASSERT_NE(actor, nullptr);

    auto property = actor->GetProperty();
    ASSERT_NE(property, nullptr);

    // Actor should have default color set
    double color[3];
    property->GetColor(color);
    EXPECT_GE(color[0], 0.0);
    EXPECT_LE(color[0], 1.0);
}

TEST_F(MeshToVTKTest, GroupActorProperties) {
    std::vector<ElementId> selectedIds = {1, 2};

    auto actor = MeshToVTK::convertGroupToActor(*mesh, selectedIds);

    ASSERT_NE(actor, nullptr);

    auto property = actor->GetProperty();
    ASSERT_NE(property, nullptr);

    // Group actor should have highlight color
    double color[3];
    property->GetColor(color);
    EXPECT_GE(color[0], 0.9);  // Should be orange-ish (1.0, 0.5, 0.0)
}

#else // !KOOMESH_HAS_VTK

// ============================================================================
// VTK Not Available Tests (Stub)
// ============================================================================

TEST_F(MeshToVTKTest, StubConversion) {
    auto result = MeshToVTK::convert(*mesh);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubPolyDataConversion) {
    auto result = MeshToVTK::convertToPolyData(*mesh);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubActorConversion) {
    auto result = MeshToVTK::convertToActor(*mesh);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubGroupConversion) {
    std::vector<ElementId> ids = {1, 2, 3};
    auto result = MeshToVTK::convertGroupToActor(*mesh, ids);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubSurfaceExtraction) {
    auto result = MeshToVTK::extractSurface(*mesh);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubEdgeConversion) {
    auto result = MeshToVTK::convertEdges(*mesh);
    EXPECT_EQ(result, nullptr);
}

TEST_F(MeshToVTKTest, StubNoVTKMessage) {
    std::cout << "Note: VTK is not available. MeshToVTK runs in stub mode.\n";
    EXPECT_TRUE(true);
}

#endif // KOOMESH_HAS_VTK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
