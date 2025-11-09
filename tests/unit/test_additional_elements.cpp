#include <gtest/gtest.h>
#include "core/Element.h"
#include "helpers/MeshBuilder.h"
#include "helpers/TestUtils.h"

using namespace koomesh::core;
using namespace koomesh::test;

// ======================================================================
// PentahedronElement Tests
// ======================================================================

class PentahedronElementTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = SimpleMeshes::singlePentahedron();
    }
};

TEST_F(PentahedronElementTest, Creation) {
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(6, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(PentahedronElementTest, Type) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::PENTAHEDRON, elem->type());
    EXPECT_EQ(6, elem->nodeCount());
}

TEST_F(PentahedronElementTest, VolumeCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double volume = elem->computeVolume(*mesh);
    EXPECT_GT(volume, 0.0);
}

TEST_F(PentahedronElementTest, QualityCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);
}

TEST_F(PentahedronElementTest, BoundingBox) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    BoundingBox bbox = elem->computeBoundingBox(*mesh);
    EXPECT_GE(bbox.maxX, bbox.minX);
    EXPECT_GE(bbox.maxY, bbox.minY);
    EXPECT_GE(bbox.maxZ, bbox.minZ);
}

TEST_F(PentahedronElementTest, Center) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    Eigen::Vector3d center = elem->computeCenter(*mesh);
    // Center should be within bounding box
    BoundingBox bbox = elem->computeBoundingBox(*mesh);
    EXPECT_GE(center.x(), bbox.minX);
    EXPECT_LE(center.x(), bbox.maxX);
}

TEST_F(PentahedronElementTest, InvalidNodeCount) {
    EXPECT_THROW(
        PentahedronElement(1, 1, {1, 2, 3, 4, 5}),  // Only 5 nodes
        std::exception
    );
}

// ======================================================================
// PyramidElement Tests
// ======================================================================

class PyramidElementTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = SimpleMeshes::singlePyramid();
    }
};

TEST_F(PyramidElementTest, Creation) {
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(5, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(PyramidElementTest, Type) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::PYRAMID, elem->type());
    EXPECT_EQ(5, elem->nodeCount());
}

TEST_F(PyramidElementTest, VolumeCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double volume = elem->computeVolume(*mesh);
    EXPECT_GT(volume, 0.0);

    // Pyramid with base (1x1) and height 1 should have volume ~= 1/3
    EXPECT_FLOAT_NEAR(volume, 1.0/3.0, 0.1);
}

TEST_F(PyramidElementTest, QualityCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);
}

TEST_F(PyramidElementTest, InvalidNodeCount) {
    EXPECT_THROW(
        PyramidElement(1, 1, {1, 2, 3, 4}),  // Only 4 nodes
        std::exception
    );
}

// ======================================================================
// TriangleElement Tests
// ======================================================================

class TriangleElementTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = SimpleMeshes::singleTriangle();
    }
};

TEST_F(TriangleElementTest, Creation) {
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(3, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(TriangleElementTest, Type) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TRIANGLE, elem->type());
    EXPECT_EQ(3, elem->nodeCount());
}

TEST_F(TriangleElementTest, AreaCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    // Triangle with vertices (0,0,0), (1,0,0), (0.5,1,0)
    // Area = 0.5 * base * height = 0.5 * 1 * 1 = 0.5
    double area = elem->computeVolume(*mesh);  // For shell, volume returns area
    EXPECT_FLOAT_NEAR(area, 0.5, 1e-6);
}

TEST_F(TriangleElementTest, AreaCalculationWithTriangleElement) {
    const TriangleElement* elem = dynamic_cast<const TriangleElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    double area = elem->computeArea(*mesh);
    EXPECT_FLOAT_NEAR(area, 0.5, 1e-6);
}

TEST_F(TriangleElementTest, NormalCalculation) {
    const TriangleElement* elem = dynamic_cast<const TriangleElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    Eigen::Vector3d normal = elem->computeNormal(*mesh);

    // Normal should be unit vector
    EXPECT_FLOAT_NEAR(normal.norm(), 1.0, 1e-6);

    // For triangle in XY plane, normal should be (0,0,±1)
    EXPECT_FLOAT_NEAR(std::abs(normal.z()), 1.0, 1e-6);
    EXPECT_FLOAT_NEAR(std::abs(normal.x()), 0.0, 1e-6);
    EXPECT_FLOAT_NEAR(std::abs(normal.y()), 0.0, 1e-6);
}

TEST_F(TriangleElementTest, QualityCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);
}

TEST_F(TriangleElementTest, InvalidNodeCount) {
    EXPECT_THROW(
        TriangleElement(1, 1, {1, 2}),  // Only 2 nodes
        std::exception
    );
}

TEST_F(TriangleElementTest, EquilateralTriangleQuality) {
    // Create equilateral triangle
    auto equiMesh = MeshBuilder()
        .addNode(1, 0.0, 0.0, 0.0)
        .addNode(2, 1.0, 0.0, 0.0)
        .addNode(3, 0.5, 0.866025, 0.0)  // sqrt(3)/2
        .addTriangle(1, 1, {1, 2, 3})
        .build();

    const Element* elem = equiMesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*equiMesh);
    // Equilateral triangle should have high quality
    EXPECT_GT(quality, 0.8);
}

// ======================================================================
// QuadrilateralElement Tests
// ======================================================================

class QuadrilateralElementTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = SimpleMeshes::singleQuadrilateral();
    }
};

TEST_F(QuadrilateralElementTest, Creation) {
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(4, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(QuadrilateralElementTest, Type) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::QUADRILATERAL, elem->type());
    EXPECT_EQ(4, elem->nodeCount());
}

TEST_F(QuadrilateralElementTest, AreaCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    // Square with side 1, area should be 1.0
    double area = elem->computeVolume(*mesh);  // For shell, volume returns area
    EXPECT_FLOAT_NEAR(area, 1.0, 1e-6);
}

TEST_F(QuadrilateralElementTest, AreaCalculationWithQuadElement) {
    const QuadrilateralElement* elem = dynamic_cast<const QuadrilateralElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    double area = elem->computeArea(*mesh);
    EXPECT_FLOAT_NEAR(area, 1.0, 1e-6);
}

TEST_F(QuadrilateralElementTest, NormalCalculation) {
    const QuadrilateralElement* elem = dynamic_cast<const QuadrilateralElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    Eigen::Vector3d normal = elem->computeNormal(*mesh);

    // Normal should be unit vector
    EXPECT_FLOAT_NEAR(normal.norm(), 1.0, 1e-6);

    // For quadrilateral in XY plane, normal should be (0,0,±1)
    EXPECT_FLOAT_NEAR(std::abs(normal.z()), 1.0, 1e-6);
}

TEST_F(QuadrilateralElementTest, QualityCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);
}

TEST_F(QuadrilateralElementTest, SquareQuality) {
    // Square should have quality 1.0 (all edges equal)
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);
    EXPECT_FLOAT_NEAR(quality, 1.0, 1e-6);
}

TEST_F(QuadrilateralElementTest, InvalidNodeCount) {
    EXPECT_THROW(
        QuadrilateralElement(1, 1, {1, 2, 3}),  // Only 3 nodes
        std::exception
    );
}

TEST_F(QuadrilateralElementTest, SkewedQuadQuality) {
    // Create skewed quadrilateral
    auto skewedMesh = MeshBuilder()
        .addNode(1, 0.0, 0.0, 0.0)
        .addNode(2, 2.0, 0.0, 0.0)  // Longer edge
        .addNode(3, 2.0, 1.0, 0.0)
        .addNode(4, 0.0, 1.0, 0.0)
        .addQuadrilateral(1, 1, {1, 2, 3, 4})
        .build();

    const Element* elem = skewedMesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*skewedMesh);
    // Quality should be min_edge/max_edge = 1.0/2.0 = 0.5
    EXPECT_FLOAT_NEAR(quality, 0.5, 1e-6);
}

// ======================================================================
// BeamElement Tests
// ======================================================================

class BeamElementTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = SimpleMeshes::singleBeam();
    }
};

TEST_F(BeamElementTest, Creation) {
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(2, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(BeamElementTest, Type) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::BEAM, elem->type());
    EXPECT_EQ(2, elem->nodeCount());
}

TEST_F(BeamElementTest, LengthCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    // Beam from (0,0,0) to (1,0,0), length = 1.0
    double length = elem->computeVolume(*mesh);  // For beam, volume returns length
    EXPECT_FLOAT_NEAR(length, 1.0, 1e-6);
}

TEST_F(BeamElementTest, LengthCalculationWithBeamElement) {
    const BeamElement* elem = dynamic_cast<const BeamElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    double length = elem->computeLength(*mesh);
    EXPECT_FLOAT_NEAR(length, 1.0, 1e-6);
}

TEST_F(BeamElementTest, DirectionCalculation) {
    const BeamElement* elem = dynamic_cast<const BeamElement*>(mesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    Eigen::Vector3d direction = elem->computeDirection(*mesh);

    // Direction should be unit vector
    EXPECT_FLOAT_NEAR(direction.norm(), 1.0, 1e-6);

    // Direction along X-axis
    EXPECT_FLOAT_NEAR(direction.x(), 1.0, 1e-6);
    EXPECT_FLOAT_NEAR(direction.y(), 0.0, 1e-6);
    EXPECT_FLOAT_NEAR(direction.z(), 0.0, 1e-6);
}

TEST_F(BeamElementTest, QualityCalculation) {
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    // Beam always has quality 1.0
    double quality = elem->computeQuality(*mesh);
    EXPECT_FLOAT_NEAR(quality, 1.0, 1e-6);
}

TEST_F(BeamElementTest, InvalidNodeCount) {
    EXPECT_THROW(
        BeamElement(1, 1, {1}),  // Only 1 node
        std::exception
    );

    EXPECT_THROW(
        BeamElement(1, 1, {1, 2, 3}),  // 3 nodes
        std::exception
    );
}

TEST_F(BeamElementTest, DiagonalBeam) {
    // Create diagonal beam
    auto diagMesh = MeshBuilder()
        .addNode(1, 0.0, 0.0, 0.0)
        .addNode(2, 1.0, 1.0, 1.0)
        .addBeam(1, 1, {1, 2})
        .build();

    const BeamElement* elem = dynamic_cast<const BeamElement*>(diagMesh->getElement(1));
    ASSERT_NE(nullptr, elem);

    double length = elem->computeLength(*diagMesh);
    EXPECT_FLOAT_NEAR(length, std::sqrt(3.0), 1e-6);

    Eigen::Vector3d direction = elem->computeDirection(*diagMesh);
    EXPECT_FLOAT_NEAR(direction.norm(), 1.0, 1e-6);

    // Direction should be (1,1,1) normalized
    double expected = 1.0 / std::sqrt(3.0);
    EXPECT_FLOAT_NEAR(direction.x(), expected, 1e-6);
    EXPECT_FLOAT_NEAR(direction.y(), expected, 1e-6);
    EXPECT_FLOAT_NEAR(direction.z(), expected, 1e-6);
}

// ======================================================================
// ElementFactory Tests for New Types
// ======================================================================

class ElementFactoryAdditionalTest : public ::testing::Test {};

TEST_F(ElementFactoryAdditionalTest, CreatePentahedron) {
    auto elem = ElementFactory::create(
        ElementType::PENTAHEDRON,
        1, 1,
        {1, 2, 3, 4, 5, 6}
    );

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::PENTAHEDRON, elem->type());
    EXPECT_EQ(6, elem->nodeCount());
}

TEST_F(ElementFactoryAdditionalTest, CreatePyramid) {
    auto elem = ElementFactory::create(
        ElementType::PYRAMID,
        1, 1,
        {1, 2, 3, 4, 5}
    );

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::PYRAMID, elem->type());
    EXPECT_EQ(5, elem->nodeCount());
}

TEST_F(ElementFactoryAdditionalTest, CreateTriangle) {
    auto elem = ElementFactory::create(
        ElementType::TRIANGLE,
        1, 1,
        {1, 2, 3}
    );

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TRIANGLE, elem->type());
    EXPECT_EQ(3, elem->nodeCount());
}

TEST_F(ElementFactoryAdditionalTest, CreateQuadrilateral) {
    auto elem = ElementFactory::create(
        ElementType::QUADRILATERAL,
        1, 1,
        {1, 2, 3, 4}
    );

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::QUADRILATERAL, elem->type());
    EXPECT_EQ(4, elem->nodeCount());
}

TEST_F(ElementFactoryAdditionalTest, CreateBeam) {
    auto elem = ElementFactory::create(
        ElementType::BEAM,
        1, 1,
        {1, 2}
    );

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::BEAM, elem->type());
    EXPECT_EQ(2, elem->nodeCount());
}

TEST_F(ElementFactoryAdditionalTest, CreateUnknownType) {
    EXPECT_THROW(
        ElementFactory::create(
            ElementType::UNKNOWN,
            1, 1,
            {1, 2, 3, 4}
        ),
        std::exception
    );
}

// ======================================================================
// Integration Tests - Mixed Element Types
// ======================================================================

class MixedElementMeshTest : public ::testing::Test {
protected:
    std::unique_ptr<Mesh> mesh;

    void SetUp() override {
        mesh = std::make_unique<Mesh>();

        // Add nodes
        mesh->addNode(Node(1, 0.0, 0.0, 0.0));
        mesh->addNode(Node(2, 1.0, 0.0, 0.0));
        mesh->addNode(Node(3, 1.0, 1.0, 0.0));
        mesh->addNode(Node(4, 0.0, 1.0, 0.0));
        mesh->addNode(Node(5, 0.5, 0.5, 1.0));
        mesh->addNode(Node(6, 2.0, 0.0, 0.0));
        mesh->addNode(Node(7, 2.0, 1.0, 0.0));

        // Add different element types
        mesh->addElement(std::make_unique<TriangleElement>(1, 1, std::vector<NodeId>{1, 2, 4}));
        mesh->addElement(std::make_unique<QuadrilateralElement>(2, 1, std::vector<NodeId>{1, 2, 3, 4}));
        mesh->addElement(std::make_unique<PyramidElement>(3, 2, std::vector<NodeId>{1, 2, 3, 4, 5}));
        mesh->addElement(std::make_unique<BeamElement>(4, 3, std::vector<NodeId>{2, 6}));
        mesh->addElement(std::make_unique<BeamElement>(5, 3, std::vector<NodeId>{3, 7}));
    }
};

TEST_F(MixedElementMeshTest, ElementCounts) {
    EXPECT_EQ(7, mesh->nodeCount());
    EXPECT_EQ(5, mesh->elementCount());
}

TEST_F(MixedElementMeshTest, ElementTypes) {
    EXPECT_EQ(ElementType::TRIANGLE, mesh->getElement(1)->type());
    EXPECT_EQ(ElementType::QUADRILATERAL, mesh->getElement(2)->type());
    EXPECT_EQ(ElementType::PYRAMID, mesh->getElement(3)->type());
    EXPECT_EQ(ElementType::BEAM, mesh->getElement(4)->type());
    EXPECT_EQ(ElementType::BEAM, mesh->getElement(5)->type());
}

TEST_F(MixedElementMeshTest, AllElementsHaveValidQuality) {
    for (size_t i = 1; i <= 5; ++i) {
        const Element* elem = mesh->getElement(i);
        ASSERT_NE(nullptr, elem);

        double quality = elem->computeQuality(*mesh);
        EXPECT_GE(quality, 0.0) << "Element " << i << " has negative quality";
        EXPECT_LE(quality, 1.0) << "Element " << i << " has quality > 1.0";
    }
}

TEST_F(MixedElementMeshTest, AllElementsHaveValidVolume) {
    for (size_t i = 1; i <= 5; ++i) {
        const Element* elem = mesh->getElement(i);
        ASSERT_NE(nullptr, elem);

        double volume = elem->computeVolume(*mesh);
        EXPECT_GE(volume, 0.0) << "Element " << i << " has negative volume/area/length";
    }
}

TEST_F(MixedElementMeshTest, MeshBoundingBox) {
    BoundingBox bbox = mesh->boundingBox();

    EXPECT_FLOAT_NEAR(bbox.minX, 0.0, 1e-6);
    EXPECT_FLOAT_NEAR(bbox.maxX, 2.0, 1e-6);
    EXPECT_FLOAT_NEAR(bbox.minY, 0.0, 1e-6);
    EXPECT_FLOAT_NEAR(bbox.maxY, 1.0, 1e-6);
    EXPECT_FLOAT_NEAR(bbox.minZ, 0.0, 1e-6);
    EXPECT_FLOAT_NEAR(bbox.maxZ, 1.0, 1e-6);
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
