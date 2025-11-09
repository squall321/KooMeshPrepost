#pragma once

#include "core/Node.h"
#include "core/Element.h"
#include "core/Mesh.h"
#include "core/Part.h"
#include <memory>
#include <vector>

namespace koomesh {
namespace test {

/**
 * @brief 테스트용 Mesh 빌더
 *
 * 테스트에서 빠르고 쉽게 메시를 생성할 수 있도록 돕는 유틸리티입니다.
 *
 * Example usage:
 * @code
 * auto mesh = MeshBuilder()
 *     .addNode(1, 0, 0, 0)
 *     .addNode(2, 1, 0, 0)
 *     .addTetrahedron(1, 1, {1, 2, 3, 4})
 *     .build();
 * @endcode
 */
class MeshBuilder {
public:
    MeshBuilder() : m_mesh(std::make_unique<core::Mesh>()) {}

    /**
     * @brief 노드 추가
     */
    MeshBuilder& addNode(core::NodeId id, double x, double y, double z) {
        m_mesh->addNode(core::Node(id, x, y, z));
        return *this;
    }

    /**
     * @brief 여러 노드 한번에 추가
     */
    MeshBuilder& addNodes(const std::vector<core::Node>& nodes) {
        m_mesh->addNodesBatch(nodes);
        return *this;
    }

    /**
     * @brief 정사각형 그리드 노드 생성
     */
    MeshBuilder& addGridNodes(int nx, int ny, double spacing = 1.0) {
        core::NodeId id = 1;
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                double x = i * spacing;
                double y = j * spacing;
                m_mesh->addNode(core::Node(id++, x, y, 0.0));
            }
        }
        return *this;
    }

    /**
     * @brief 사면체 요소 추가
     */
    MeshBuilder& addTetrahedron(core::ElementId id, core::PartId partId,
                                const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::TetrahedronElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 육면체 요소 추가
     */
    MeshBuilder& addHexahedron(core::ElementId id, core::PartId partId,
                               const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::HexahedronElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 오각기둥 요소 추가
     */
    MeshBuilder& addPentahedron(core::ElementId id, core::PartId partId,
                                const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::PentahedronElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 피라미드 요소 추가
     */
    MeshBuilder& addPyramid(core::ElementId id, core::PartId partId,
                            const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::PyramidElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 삼각형 쉘 요소 추가
     */
    MeshBuilder& addTriangle(core::ElementId id, core::PartId partId,
                             const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::TriangleElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 사각형 쉘 요소 추가
     */
    MeshBuilder& addQuadrilateral(core::ElementId id, core::PartId partId,
                                  const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::QuadrilateralElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief 빔 요소 추가
     */
    MeshBuilder& addBeam(core::ElementId id, core::PartId partId,
                         const std::vector<core::NodeId>& nodeIds) {
        auto element = std::make_unique<core::BeamElement>(id, partId, nodeIds);
        m_mesh->addElement(std::move(element));
        return *this;
    }

    /**
     * @brief Part 추가
     */
    MeshBuilder& addPart(core::PartId id, const std::string& name = "") {
        m_mesh->addPart(core::Part(id, name));
        return *this;
    }

    /**
     * @brief 생성된 메시 반환
     */
    std::unique_ptr<core::Mesh> build() {
        return std::move(m_mesh);
    }

    /**
     * @brief 메시 참조 반환 (계속 빌더 사용 가능)
     */
    core::Mesh& get() {
        return *m_mesh;
    }

private:
    std::unique_ptr<core::Mesh> m_mesh;
};

/**
 * @brief 미리 정의된 간단한 메시들
 */
class SimpleMeshes {
public:
    /**
     * @brief 단일 사면체 메시
     */
    static std::unique_ptr<core::Mesh> singleTetrahedron() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 0.0, 1.0, 0.0)
            .addNode(4, 0.0, 0.0, 1.0)
            .addTetrahedron(1, 1, {1, 2, 3, 4})
            .build();
    }

    /**
     * @brief 단일 육면체 메시
     */
    static std::unique_ptr<core::Mesh> singleHexahedron() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 1.0, 1.0, 0.0)
            .addNode(4, 0.0, 1.0, 0.0)
            .addNode(5, 0.0, 0.0, 1.0)
            .addNode(6, 1.0, 0.0, 1.0)
            .addNode(7, 1.0, 1.0, 1.0)
            .addNode(8, 0.0, 1.0, 1.0)
            .addHexahedron(1, 1, {1, 2, 3, 4, 5, 6, 7, 8})
            .build();
    }

    /**
     * @brief 단일 오각기둥 메시 (wedge)
     */
    static std::unique_ptr<core::Mesh> singlePentahedron() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 0.5, 1.0, 0.0)
            .addNode(4, 0.0, 0.0, 1.0)
            .addNode(5, 1.0, 0.0, 1.0)
            .addNode(6, 0.5, 1.0, 1.0)
            .addPentahedron(1, 1, {1, 2, 3, 4, 5, 6})
            .build();
    }

    /**
     * @brief 단일 피라미드 메시
     */
    static std::unique_ptr<core::Mesh> singlePyramid() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 1.0, 1.0, 0.0)
            .addNode(4, 0.0, 1.0, 0.0)
            .addNode(5, 0.5, 0.5, 1.0)
            .addPyramid(1, 1, {1, 2, 3, 4, 5})
            .build();
    }

    /**
     * @brief 단일 삼각형 쉘 메시
     */
    static std::unique_ptr<core::Mesh> singleTriangle() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 0.5, 1.0, 0.0)
            .addTriangle(1, 1, {1, 2, 3})
            .build();
    }

    /**
     * @brief 단일 사각형 쉘 메시
     */
    static std::unique_ptr<core::Mesh> singleQuadrilateral() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addNode(3, 1.0, 1.0, 0.0)
            .addNode(4, 0.0, 1.0, 0.0)
            .addQuadrilateral(1, 1, {1, 2, 3, 4})
            .build();
    }

    /**
     * @brief 단일 빔 메시
     */
    static std::unique_ptr<core::Mesh> singleBeam() {
        return MeshBuilder()
            .addNode(1, 0.0, 0.0, 0.0)
            .addNode(2, 1.0, 0.0, 0.0)
            .addBeam(1, 1, {1, 2})
            .build();
    }

    /**
     * @brief 2x2 그리드 메시
     */
    static std::unique_ptr<core::Mesh> grid2x2() {
        auto builder = MeshBuilder();
        builder.addGridNodes(3, 3);

        // 사면체 요소들 추가 (2x2 그리드 = 8개 삼각형)
        // 간단한 구현을 위해 몇 개만 추가
        builder.addTetrahedron(1, 1, {1, 2, 4, 5});

        return builder.build();
    }

    /**
     * @brief 빈 메시
     */
    static std::unique_ptr<core::Mesh> empty() {
        return std::make_unique<core::Mesh>();
    }
};

} // namespace test
} // namespace koomesh
