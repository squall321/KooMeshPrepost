/**
 * @file main.cpp
 * @brief Core Data Types 사용 예제
 *
 * Node, Element, Mesh, Part 등 핵심 데이터 구조의 사용법을 시연합니다.
 */

#include "core/Node.h"
#include "core/Element.h"
#include "core/Mesh.h"
#include "core/Part.h"
#include "utils/Logger.h"
#include <iostream>
#include <iomanip>

using namespace koomesh::core;
using namespace koomesh::utils;

// ======================================================================
// 헬퍼 함수
// ======================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

// ======================================================================
// 예제 1: Node 클래스 사용
// ======================================================================

void example1_Node() {
    printSeparator("Example 1: Node 클래스");

    // Node 생성
    Node node1(1, 0.0, 0.0, 0.0);
    Node node2(2, 1.0, 0.0, 0.0);
    Node node3(3, 0.0, 1.0, 0.0);

    std::cout << "Node 1: ID=" << node1.id()
              << ", Position=(" << node1.x() << ", " << node1.y() << ", " << node1.z() << ")\n";

    std::cout << "Node 2: ID=" << node2.id()
              << ", Position=(" << node2.x() << ", " << node2.y() << ", " << node2.z() << ")\n";

    // 거리 계산
    double dist = node1.distanceTo(node2);
    std::cout << "\nDistance between Node 1 and Node 2: " << dist << "\n";

    // 이동
    node3.translate(Eigen::Vector3d(1.0, 1.0, 0.0));
    std::cout << "Node 3 after translation: ("
              << node3.x() << ", " << node3.y() << ", " << node3.z() << ")\n";

    // 스케일
    Node node4(4, 2.0, 4.0, 6.0);
    node4.scale(0.5);
    std::cout << "Node 4 after scaling by 0.5: ("
              << node4.x() << ", " << node4.y() << ", " << node4.z() << ")\n";
}

// ======================================================================
// 예제 2: BoundingBox 사용
// ======================================================================

void example2_BoundingBox() {
    printSeparator("Example 2: BoundingBox");

    BoundingBox bbox;
    bbox.minX = 0.0; bbox.maxX = 10.0;
    bbox.minY = 0.0; bbox.maxY = 10.0;
    bbox.minZ = 0.0; bbox.maxZ = 10.0;

    std::cout << "Bounding Box:\n";
    std::cout << "  Min: (" << bbox.minX << ", " << bbox.minY << ", " << bbox.minZ << ")\n";
    std::cout << "  Max: (" << bbox.maxX << ", " << bbox.maxY << ", " << bbox.maxZ << ")\n";
    std::cout << "  Center: ("
              << bbox.center().x() << ", "
              << bbox.center().y() << ", "
              << bbox.center().z() << ")\n";
    std::cout << "  Volume: " << bbox.volume() << "\n";

    // 포함 검사
    Eigen::Vector3d point1(5.0, 5.0, 5.0);
    Eigen::Vector3d point2(15.0, 5.0, 5.0);

    std::cout << "\nPoint (5,5,5) is " << (bbox.contains(point1) ? "inside" : "outside") << " the box\n";
    std::cout << "Point (15,5,5) is " << (bbox.contains(point2) ? "inside" : "outside") << " the box\n";

    // 확장
    bbox.expand(Eigen::Vector3d(20.0, 5.0, 5.0));
    std::cout << "\nAfter expanding to (20,5,5):\n";
    std::cout << "  Max: (" << bbox.maxX << ", " << bbox.maxY << ", " << bbox.maxZ << ")\n";
}

// ======================================================================
// 예제 3: Element 생성 및 사용
// ======================================================================

void example3_Element() {
    printSeparator("Example 3: Element 생성");

    // Mesh 생성
    Mesh mesh;

    // 사면체용 노드 추가
    mesh.addNode(Node(1, 0.0, 0.0, 0.0));
    mesh.addNode(Node(2, 1.0, 0.0, 0.0));
    mesh.addNode(Node(3, 0.0, 1.0, 0.0));
    mesh.addNode(Node(4, 0.0, 0.0, 1.0));

    std::cout << "Added 4 nodes for tetrahedron\n";

    // 사면체 요소 생성
    std::vector<NodeId> nodeIds = {1, 2, 3, 4};
    auto tetraElem = ElementFactory::create(ElementType::TETRAHEDRON, 100, 1, nodeIds);

    std::cout << "\nTetrahedron Element:\n";
    std::cout << "  ID: " << tetraElem->id() << "\n";
    std::cout << "  Part ID: " << tetraElem->partId() << "\n";
    std::cout << "  Type: " << (tetraElem->type() == ElementType::TETRAHEDRON ? "TETRAHEDRON" : "OTHER") << "\n";
    std::cout << "  Node Count: " << tetraElem->nodeCount() << "\n";

    // Mesh에 추가
    mesh.addElement(std::move(tetraElem));

    // 요소 정보 계산
    const Element* elem = mesh.getElement(100);
    if (elem) {
        Eigen::Vector3d center = elem->computeCenter(mesh);
        double volume = elem->computeVolume(mesh);
        double quality = elem->computeQuality(mesh);

        std::cout << "\nElement Properties:\n";
        std::cout << "  Center: (" << center.x() << ", " << center.y() << ", " << center.z() << ")\n";
        std::cout << "  Volume: " << volume << "\n";
        std::cout << "  Quality: " << quality << " (0.0 = worst, 1.0 = best)\n";
    }
}

// ======================================================================
// 예제 4: Part 사용
// ======================================================================

void example4_Part() {
    printSeparator("Example 4: Part 사용");

    Part steelPart(1, "Steel Structure");

    std::cout << "Part ID: " << steelPart.id() << "\n";
    std::cout << "Part Name: " << steelPart.name() << "\n";

    // Material 속성 설정
    Part::MaterialProperties steel;
    steel.density = 7850.0;  // kg/m^3
    steel.youngModulus = 200e9;  // Pa
    steel.poissonRatio = 0.3;

    steelPart.setMaterialProperties(steel);

    std::cout << "\nMaterial Properties:\n";
    std::cout << "  Density: " << steelPart.materialProperties().density << " kg/m³\n";
    std::cout << "  Young's Modulus: " << steelPart.materialProperties().youngModulus / 1e9 << " GPa\n";
    std::cout << "  Poisson's Ratio: " << steelPart.materialProperties().poissonRatio << "\n";

    // 요소 추가
    steelPart.addElement(100);
    steelPart.addElement(101);
    steelPart.addElement(102);

    std::cout << "\nElement Count: " << steelPart.elementCount() << "\n";
    std::cout << "Contains Element 100: " << (steelPart.containsElement(100) ? "Yes" : "No") << "\n";
    std::cout << "Contains Element 999: " << (steelPart.containsElement(999) ? "Yes" : "No") << "\n";
}

// ======================================================================
// 예제 5: Mesh - 노드 및 요소 관리
// ======================================================================

void example5_Mesh() {
    printSeparator("Example 5: Mesh 사용");

    Mesh mesh;
    Logger::getInstance().setLevel(LogLevel::INFO);

    // 노드 배치 추가
    std::vector<Node> nodes;
    for (int i = 1; i <= 8; ++i) {
        double x = (i - 1) % 2;
        double y = ((i - 1) / 2) % 2;
        double z = (i - 1) / 4;
        nodes.push_back(Node(i, x, y, z));
    }

    mesh.addNodesBatch(nodes);
    Logger::infof("Added {} nodes to mesh", mesh.nodeCount());

    // 육면체 요소 추가
    std::vector<NodeId> hexNodeIds = {1, 2, 4, 3, 5, 6, 8, 7};
    auto hexElem = ElementFactory::create(ElementType::HEXAHEDRON, 200, 2, hexNodeIds);
    mesh.addElement(std::move(hexElem));

    Logger::infof("Added {} elements to mesh", mesh.elementCount());

    // 경계 상자 계산
    BoundingBox bbox = mesh.boundingBox();
    std::cout << "\nMesh Bounding Box:\n";
    std::cout << "  Min: (" << bbox.minX << ", " << bbox.minY << ", " << bbox.minZ << ")\n";
    std::cout << "  Max: (" << bbox.maxX << ", " << bbox.maxY << ", " << bbox.maxZ << ")\n";

    // 가장 가까운 노드 찾기
    Eigen::Vector3d queryPoint(0.3, 0.3, 0.3);
    NodeId nearest = mesh.findNearestNode(queryPoint);

    std::cout << "\nNearest node to (0.3, 0.3, 0.3): Node " << nearest << "\n";

    // 노드 조회
    const Node* node = mesh.getNode(nearest);
    if (node) {
        std::cout << "  Position: (" << node->x() << ", " << node->y() << ", " << node->z() << ")\n";
    }
}

// ======================================================================
// 예제 6: 복잡한 메시 구축
// ======================================================================

void example6_ComplexMesh() {
    printSeparator("Example 6: 복잡한 메시 구축");

    Mesh mesh;
    Logger::getInstance().setLevel(LogLevel::INFO);

    // 10x10 그리드 노드 생성
    Logger::info("Creating 10x10 grid...");

    int nodeId = 1;
    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < 10; ++i) {
            double x = static_cast<double>(i);
            double y = static_cast<double>(j);
            double z = 0.0;
            mesh.addNode(Node(nodeId++, x, y, z));
        }
    }

    Logger::infof("Created {} nodes", mesh.nodeCount());

    // Part 추가
    Part gridPart(1, "Grid Structure");
    mesh.addPart(gridPart);

    // 사면체 요소들 생성 (간단한 예시로 몇 개만)
    int elemId = 1;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            // 정사각형을 2개의 삼각형으로 분할하는 대신,
            // 3D 사면체를 만들기 위해 추가 노드 필요
            // 여기서는 간단히 평면 노드만 사용하여 데모

            int n1 = j * 10 + i + 1;
            int n2 = j * 10 + i + 2;
            int n3 = (j + 1) * 10 + i + 1;
            int n4 = (j + 1) * 10 + i + 2;

            // 중심 노드 추가 (z=1)
            int centerNode = 100 + elemId;
            mesh.addNode(Node(centerNode, i + 0.5, j + 0.5, 1.0));

            // 사면체 요소 추가
            std::vector<NodeId> nodeIds = {
                static_cast<NodeId>(n1),
                static_cast<NodeId>(n2),
                static_cast<NodeId>(n3),
                static_cast<NodeId>(centerNode)
            };

            try {
                auto elem = ElementFactory::create(ElementType::TETRAHEDRON, elemId++, 1, nodeIds);
                mesh.addElement(std::move(elem));
            } catch (const std::exception& e) {
                Logger::errorf("Failed to create element: {}", e.what());
            }
        }
    }

    Logger::infof("Created {} elements", mesh.elementCount());

    // 통계
    std::cout << "\nMesh Statistics:\n";
    std::cout << "  Total Nodes: " << mesh.nodeCount() << "\n";
    std::cout << "  Total Elements: " << mesh.elementCount() << "\n";

    auto stats = mesh.getElementTypeStatistics();
    for (const auto& pair : stats) {
        std::string typeName;
        switch (pair.first) {
            case ElementType::TETRAHEDRON: typeName = "Tetrahedron"; break;
            case ElementType::HEXAHEDRON: typeName = "Hexahedron"; break;
            default: typeName = "Other"; break;
        }
        std::cout << "  " << typeName << ": " << pair.second << "\n";
    }

    BoundingBox bbox = mesh.boundingBox();
    std::cout << "\nBounding Box:\n";
    std::cout << "  Size: "
              << (bbox.maxX - bbox.minX) << " x "
              << (bbox.maxY - bbox.minY) << " x "
              << (bbox.maxZ - bbox.minZ) << "\n";
}

// ======================================================================
// 예제 7: 요소 품질 분석
// ======================================================================

void example7_QualityAnalysis() {
    printSeparator("Example 7: 요소 품질 분석");

    Mesh mesh;

    // 좋은 품질의 정사면체
    mesh.addNode(Node(1, 0.0, 0.0, 0.0));
    mesh.addNode(Node(2, 1.0, 0.0, 0.0));
    mesh.addNode(Node(3, 0.5, std::sqrt(3.0)/2.0, 0.0));
    mesh.addNode(Node(4, 0.5, std::sqrt(3.0)/6.0, std::sqrt(2.0/3.0)));

    std::vector<NodeId> goodTetraNodes = {1, 2, 3, 4};
    auto goodTetra = ElementFactory::create(ElementType::TETRAHEDRON, 1, 1, goodTetraNodes);

    std::cout << "Good Quality Tetrahedron:\n";
    std::cout << "  Volume: " << goodTetra->computeVolume(mesh) << "\n";
    std::cout << "  Quality: " << goodTetra->computeQuality(mesh) << "\n";

    mesh.addElement(std::move(goodTetra));

    // 나쁜 품질의 납작한 사면체
    mesh.addNode(Node(5, 0.0, 0.0, 0.0));
    mesh.addNode(Node(6, 1.0, 0.0, 0.0));
    mesh.addNode(Node(7, 0.0, 1.0, 0.0));
    mesh.addNode(Node(8, 0.0, 0.0, 0.01));  // 매우 작은 높이

    std::vector<NodeId> badTetraNodes = {5, 6, 7, 8};
    auto badTetra = ElementFactory::create(ElementType::TETRAHEDRON, 2, 1, badTetraNodes);

    std::cout << "\nBad Quality Tetrahedron (flat):\n";
    std::cout << "  Volume: " << badTetra->computeVolume(mesh) << "\n";
    std::cout << "  Quality: " << badTetra->computeQuality(mesh) << "\n";

    mesh.addElement(std::move(badTetra));
}

// ======================================================================
// 메인 함수
// ======================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  Core Data Types Examples\n";
    std::cout << "========================================\n";

    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().setLevel(LogLevel::WARNING);

    try {
        example1_Node();
        example2_BoundingBox();
        example3_Element();
        example4_Part();
        example5_Mesh();
        example6_ComplexMesh();
        example7_QualityAnalysis();

        printSeparator("모든 예제 완료");
        std::cout << "✓ 모든 예제가 성공적으로 실행되었습니다.\n";

    } catch (const std::exception& e) {
        std::cerr << "✗ 오류 발생: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
