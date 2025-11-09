#include <gtest/gtest.h>
#include "core/Node.h"
#include "core/Element.h"
#include "core/Mesh.h"
#include "core/Part.h"
#include "utils/Exception.h"
#include <cmath>

using namespace koomesh::core;

// ======================================================================
// Node 테스트
// ======================================================================

class NodeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(NodeTest, Constructor_WithCoordinates) {
    Node node(1, 1.0, 2.0, 3.0);

    EXPECT_EQ(1, node.id());
    EXPECT_DOUBLE_EQ(1.0, node.x());
    EXPECT_DOUBLE_EQ(2.0, node.y());
    EXPECT_DOUBLE_EQ(3.0, node.z());
}

TEST_F(NodeTest, Constructor_WithVector) {
    Eigen::Vector3d coords(1.0, 2.0, 3.0);
    Node node(1, coords);

    EXPECT_EQ(1, node.id());
    EXPECT_DOUBLE_EQ(1.0, node.coordinates().x());
    EXPECT_DOUBLE_EQ(2.0, node.coordinates().y());
    EXPECT_DOUBLE_EQ(3.0, node.coordinates().z());
}

TEST_F(NodeTest, DistanceTo) {
    Node n1(1, 0.0, 0.0, 0.0);
    Node n2(2, 3.0, 4.0, 0.0);

    double distance = n1.distanceTo(n2);
    EXPECT_DOUBLE_EQ(5.0, distance);
}

TEST_F(NodeTest, Translate) {
    Node node(1, 1.0, 2.0, 3.0);
    Eigen::Vector3d offset(1.0, 1.0, 1.0);

    node.translate(offset);

    EXPECT_DOUBLE_EQ(2.0, node.x());
    EXPECT_DOUBLE_EQ(3.0, node.y());
    EXPECT_DOUBLE_EQ(4.0, node.z());
}

TEST_F(NodeTest, Scale) {
    Node node(1, 2.0, 4.0, 6.0);

    node.scale(0.5);

    EXPECT_DOUBLE_EQ(1.0, node.x());
    EXPECT_DOUBLE_EQ(2.0, node.y());
    EXPECT_DOUBLE_EQ(3.0, node.z());
}

TEST_F(NodeTest, ConnectedElements) {
    Node node(1, 0.0, 0.0, 0.0);

    EXPECT_TRUE(node.connectedElements().empty());

    node.addConnectedElement(100);
    node.addConnectedElement(200);

    EXPECT_EQ(2, node.connectedElements().size());

    // 중복 추가 방지 확인
    node.addConnectedElement(100);
    EXPECT_EQ(2, node.connectedElements().size());
}

// ======================================================================
// BoundingBox 테스트
// ======================================================================

class BoundingBoxTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BoundingBoxTest, Intersects) {
    BoundingBox box1;
    box1.minX = 0.0; box1.maxX = 10.0;
    box1.minY = 0.0; box1.maxY = 10.0;
    box1.minZ = 0.0; box1.maxZ = 10.0;

    BoundingBox box2;
    box2.minX = 5.0; box2.maxX = 15.0;
    box2.minY = 5.0; box2.maxY = 15.0;
    box2.minZ = 5.0; box2.maxZ = 15.0;

    EXPECT_TRUE(box1.intersects(box2));

    BoundingBox box3;
    box3.minX = 20.0; box3.maxX = 30.0;
    box3.minY = 20.0; box3.maxY = 30.0;
    box3.minZ = 20.0; box3.maxZ = 30.0;

    EXPECT_FALSE(box1.intersects(box3));
}

TEST_F(BoundingBoxTest, Contains) {
    BoundingBox box;
    box.minX = 0.0; box.maxX = 10.0;
    box.minY = 0.0; box.maxY = 10.0;
    box.minZ = 0.0; box.maxZ = 10.0;

    Eigen::Vector3d inside(5.0, 5.0, 5.0);
    Eigen::Vector3d outside(15.0, 5.0, 5.0);

    EXPECT_TRUE(box.contains(inside));
    EXPECT_FALSE(box.contains(outside));
}

TEST_F(BoundingBoxTest, Expand) {
    BoundingBox box;
    box.minX = box.maxX = 0.0;
    box.minY = box.maxY = 0.0;
    box.minZ = box.maxZ = 0.0;

    Eigen::Vector3d point1(5.0, 5.0, 5.0);
    Eigen::Vector3d point2(-3.0, 7.0, 2.0);

    box.expand(point1);
    EXPECT_DOUBLE_EQ(5.0, box.maxX);
    EXPECT_DOUBLE_EQ(5.0, box.maxY);
    EXPECT_DOUBLE_EQ(5.0, box.maxZ);

    box.expand(point2);
    EXPECT_DOUBLE_EQ(-3.0, box.minX);
    EXPECT_DOUBLE_EQ(7.0, box.maxY);
}

TEST_F(BoundingBoxTest, Center) {
    BoundingBox box;
    box.minX = 0.0; box.maxX = 10.0;
    box.minY = 0.0; box.maxY = 20.0;
    box.minZ = 0.0; box.maxZ = 30.0;

    Eigen::Vector3d center = box.center();

    EXPECT_DOUBLE_EQ(5.0, center.x());
    EXPECT_DOUBLE_EQ(10.0, center.y());
    EXPECT_DOUBLE_EQ(15.0, center.z());
}

TEST_F(BoundingBoxTest, Volume) {
    BoundingBox box;
    box.minX = 0.0; box.maxX = 2.0;
    box.minY = 0.0; box.maxY = 3.0;
    box.minZ = 0.0; box.maxZ = 4.0;

    EXPECT_DOUBLE_EQ(24.0, box.volume());
}

// ======================================================================
// Element 테스트
// ======================================================================

class ElementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트용 메시 생성
        mesh = std::make_unique<Mesh>();

        // 사면체용 노드 4개 추가
        mesh->addNode(Node(1, 0.0, 0.0, 0.0));
        mesh->addNode(Node(2, 1.0, 0.0, 0.0));
        mesh->addNode(Node(3, 0.0, 1.0, 0.0));
        mesh->addNode(Node(4, 0.0, 0.0, 1.0));

        // 육면체용 노드 추가
        mesh->addNode(Node(5, 1.0, 1.0, 0.0));
        mesh->addNode(Node(6, 0.0, 1.0, 1.0));
        mesh->addNode(Node(7, 1.0, 0.0, 1.0));
        mesh->addNode(Node(8, 1.0, 1.0, 1.0));
    }

    void TearDown() override {}

    std::unique_ptr<Mesh> mesh;
};

TEST_F(ElementTest, TetrahedronElement_Creation) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    EXPECT_EQ(100, elem->id());
    EXPECT_EQ(1, elem->partId());
    EXPECT_EQ(ElementType::TETRAHEDRON, elem->type());
    EXPECT_EQ(4, elem->nodeCount());
}

TEST_F(ElementTest, TetrahedronElement_InvalidNodeCount) {
    std::vector<NodeId> invalidNodeIds = {1, 2, 3};  // 3개 노드 (잘못됨)

    EXPECT_THROW(
        TetrahedronElement(100, 1, invalidNodeIds),
        koomesh::utils::InvalidDataException
    );
}

TEST_F(ElementTest, TetrahedronElement_Volume) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    double volume = elem->computeVolume(*mesh);

    // 정사면체 부피 = 1/6
    EXPECT_NEAR(1.0/6.0, volume, 1e-10);
}

TEST_F(ElementTest, TetrahedronElement_Quality) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    double quality = elem->computeQuality(*mesh);

    // 품질은 0.0 ~ 1.0 범위
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);
}

TEST_F(ElementTest, HexahedronElement_Creation) {
    std::vector<NodeId> nodeIds = {1, 2, 5, 3, 4, 7, 8, 6};
    auto elem = std::make_unique<HexahedronElement>(200, 2, nodeIds);

    EXPECT_EQ(200, elem->id());
    EXPECT_EQ(2, elem->partId());
    EXPECT_EQ(ElementType::HEXAHEDRON, elem->type());
    EXPECT_EQ(8, elem->nodeCount());
}

TEST_F(ElementTest, ElementFactory_CreateTetrahedron) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};

    auto elem = ElementFactory::create(ElementType::TETRAHEDRON, 100, 1, nodeIds);

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TETRAHEDRON, elem->type());
}

TEST_F(ElementTest, ElementFactory_CreateHexahedron) {
    std::vector<NodeId> nodeIds = {1, 2, 5, 3, 4, 7, 8, 6};

    auto elem = ElementFactory::create(ElementType::HEXAHEDRON, 200, 2, nodeIds);

    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::HEXAHEDRON, elem->type());
}

TEST_F(ElementTest, ElementFactory_NotImplemented) {
    std::vector<NodeId> nodeIds = {1, 2, 3};

    EXPECT_THROW(
        ElementFactory::create(ElementType::TRIANGLE, 300, 3, nodeIds),
        koomesh::utils::RuntimeException
    );
}

TEST_F(ElementTest, Element_ComputeCenter) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    Eigen::Vector3d center = elem->computeCenter(*mesh);

    // 사면체 중심은 네 꼭지점의 평균
    EXPECT_DOUBLE_EQ(0.25, center.x());
    EXPECT_DOUBLE_EQ(0.25, center.y());
    EXPECT_DOUBLE_EQ(0.25, center.z());
}

TEST_F(ElementTest, Element_BoundingBox) {
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    BoundingBox bbox = elem->computeBoundingBox(*mesh);

    EXPECT_DOUBLE_EQ(0.0, bbox.minX);
    EXPECT_DOUBLE_EQ(1.0, bbox.maxX);
    EXPECT_DOUBLE_EQ(0.0, bbox.minY);
    EXPECT_DOUBLE_EQ(1.0, bbox.maxY);
    EXPECT_DOUBLE_EQ(0.0, bbox.minZ);
    EXPECT_DOUBLE_EQ(1.0, bbox.maxZ);
}

// ======================================================================
// Part 테스트
// ======================================================================

class PartTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PartTest, Constructor) {
    Part part(1, "Test Part");

    EXPECT_EQ(1, part.id());
    EXPECT_EQ("Test Part", part.name());
    EXPECT_EQ(0, part.elementCount());
}

TEST_F(PartTest, AddElement) {
    Part part(1, "Test Part");

    part.addElement(100);
    part.addElement(200);

    EXPECT_EQ(2, part.elementCount());
    EXPECT_TRUE(part.containsElement(100));
    EXPECT_TRUE(part.containsElement(200));
    EXPECT_FALSE(part.containsElement(300));
}

TEST_F(PartTest, RemoveElement) {
    Part part(1, "Test Part");

    part.addElement(100);
    part.addElement(200);

    EXPECT_TRUE(part.removeElement(100));
    EXPECT_EQ(1, part.elementCount());
    EXPECT_FALSE(part.containsElement(100));

    EXPECT_FALSE(part.removeElement(300));
}

TEST_F(PartTest, Clear) {
    Part part(1, "Test Part");

    part.addElement(100);
    part.addElement(200);
    part.addElement(300);

    part.clear();

    EXPECT_EQ(0, part.elementCount());
}

TEST_F(PartTest, MaterialProperties) {
    Part part(1, "Steel Part");

    Part::MaterialProperties props;
    props.density = 7850.0;
    props.youngModulus = 200e9;
    props.poissonRatio = 0.3;

    part.setMaterialProperties(props);

    const auto& retrieved = part.materialProperties();
    EXPECT_DOUBLE_EQ(7850.0, retrieved.density);
    EXPECT_DOUBLE_EQ(200e9, retrieved.youngModulus);
    EXPECT_DOUBLE_EQ(0.3, retrieved.poissonRatio);
}

// ======================================================================
// Mesh 테스트
// ======================================================================

class MeshTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
    }

    void TearDown() override {}

    std::unique_ptr<Mesh> mesh;
};

TEST_F(MeshTest, AddNode) {
    Node node(1, 1.0, 2.0, 3.0);
    mesh->addNode(node);

    EXPECT_EQ(1, mesh->nodeCount());

    const Node* retrieved = mesh->getNode(1);
    ASSERT_NE(nullptr, retrieved);
    EXPECT_EQ(1, retrieved->id());
}

TEST_F(MeshTest, AddNode_Duplicate) {
    Node node1(1, 1.0, 2.0, 3.0);
    Node node2(1, 4.0, 5.0, 6.0);

    mesh->addNode(node1);

    EXPECT_THROW(
        mesh->addNode(node2),
        koomesh::utils::InvalidDataException
    );
}

TEST_F(MeshTest, AddNodesBatch) {
    std::vector<Node> nodes;
    nodes.push_back(Node(1, 0.0, 0.0, 0.0));
    nodes.push_back(Node(2, 1.0, 0.0, 0.0));
    nodes.push_back(Node(3, 0.0, 1.0, 0.0));

    mesh->addNodesBatch(nodes);

    EXPECT_EQ(3, mesh->nodeCount());
}

TEST_F(MeshTest, AddElement) {
    // 노드 추가
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    mesh->addNode(Node(3, 0.0, 1.0, 0.0));
    mesh->addNode(Node(4, 0.0, 0.0, 1.0));

    // 요소 추가
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);
    mesh->addElement(std::move(elem));

    EXPECT_EQ(1, mesh->elementCount());

    const Element* retrieved = mesh->getElement(100);
    ASSERT_NE(nullptr, retrieved);
    EXPECT_EQ(100, retrieved->id());
}

TEST_F(MeshTest, AddElement_InvalidNodeReference) {
    // 노드 없이 요소 추가 시도
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);

    EXPECT_THROW(
        mesh->addElement(std::move(elem)),
        koomesh::utils::InvalidDataException
    );
}

TEST_F(MeshTest, RemoveNode) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));

    EXPECT_TRUE(mesh->removeNode(1));
    EXPECT_EQ(0, mesh->nodeCount());

    EXPECT_FALSE(mesh->removeNode(999));
}

TEST_F(MeshTest, RemoveElement) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    mesh->addNode(Node(3, 0.0, 1.0, 0.0));
    mesh->addNode(Node(4, 0.0, 0.0, 1.0));

    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto elem = std::make_unique<TetrahedronElement>(100, 1, nodeIds);
    mesh->addElement(std::move(elem));

    EXPECT_TRUE(mesh->removeElement(100));
    EXPECT_EQ(0, mesh->elementCount());
}

TEST_F(MeshTest, BoundingBox) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 10.0, 0.0, 0.0));
    mesh->addNode(Node(3, 0.0, 20.0, 0.0));
    mesh->addNode(Node(4, 0.0, 0.0, 30.0));

    BoundingBox bbox = mesh->boundingBox();

    EXPECT_DOUBLE_EQ(0.0, bbox.minX);
    EXPECT_DOUBLE_EQ(10.0, bbox.maxX);
    EXPECT_DOUBLE_EQ(0.0, bbox.minY);
    EXPECT_DOUBLE_EQ(20.0, bbox.maxY);
    EXPECT_DOUBLE_EQ(0.0, bbox.minZ);
    EXPECT_DOUBLE_EQ(30.0, bbox.maxZ);
}

TEST_F(MeshTest, GetAllNodeIds) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    mesh->addNode(Node(3, 0.0, 1.0, 0.0));

    std::vector<NodeId> ids = mesh->getAllNodeIds();

    EXPECT_EQ(3, ids.size());
    EXPECT_NE(std::find(ids.begin(), ids.end(), 1), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), 2), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), 3), ids.end());
}

TEST_F(MeshTest, FindNearestNode) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 10.0, 0.0, 0.0));
    mesh->addNode(Node(3, 5.0, 5.0, 0.0));

    Eigen::Vector3d queryPoint(4.0, 4.0, 0.0);
    NodeId nearest = mesh->findNearestNode(queryPoint);

    EXPECT_EQ(3, nearest);
}

TEST_F(MeshTest, Clear) {
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));

    mesh->clear();

    EXPECT_EQ(0, mesh->nodeCount());
    EXPECT_EQ(0, mesh->elementCount());
}

// ======================================================================
// 메인 함수
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
