#pragma once

#include "core/ISpatialIndex.h"
#include <Eigen/Dense>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

namespace koomesh {
namespace core {

// Forward declaration
class Mesh;

/**
 * @brief Uniform Grid spatial index implementation
 *
 * Uniform Grid divides 3D space into regular-sized cells.
 * Elements are assigned to cells based on their bounding boxes.
 *
 * Properties:
 * - O(1) point query time (best case)
 * - Very fast for uniformly distributed data
 * - Simple and memory-efficient
 * - Easy to parallelize
 *
 * Trade-offs:
 * - May use more memory than tree structures
 * - Performance degrades with non-uniform data
 * - Empty cells waste memory
 *
 * Example:
 * @code
 * UniformGrid grid;
 * grid.setCellSize(1.0);  // 1x1x1 cells
 * grid.build(mesh);
 *
 * Eigen::Vector3d point(5.5, 3.2, 1.8);
 * ElementId nearest = grid.findNearest(point);
 * @endcode
 */
class UniformGrid : public ISpatialIndex {
public:
    /**
     * @brief Constructor with default parameters
     */
    UniformGrid();

    /**
     * @brief Constructor with custom cell size
     *
     * @param cellSize Size of each grid cell (uniform in all dimensions)
     */
    explicit UniformGrid(double cellSize);

    /**
     * @brief Constructor with custom cell count
     *
     * @param cellsX Number of cells in X dimension
     * @param cellsY Number of cells in Y dimension
     * @param cellsZ Number of cells in Z dimension
     */
    UniformGrid(size_t cellsX, size_t cellsY, size_t cellsZ);

    /**
     * @brief Destructor
     */
    ~UniformGrid() override = default;

    // ISpatialIndex interface implementation
    void build(const Mesh& mesh) override;
    std::vector<ElementId> query(const BoundingBox& box) const override;
    std::vector<ElementId> queryPoint(const Eigen::Vector3d& point) const override;
    ElementId findNearest(const Eigen::Vector3d& point) const override;
    std::vector<ElementId> findKNearest(const Eigen::Vector3d& point, size_t k) const override;
    std::vector<ElementId> findWithinRadius(const Eigen::Vector3d& point, double radius) const override;
    std::vector<ElementId> rayIntersect(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction) const override;
    void clear() override;
    bool isBuilt() const override;
    size_t elementCount() const override;
    size_t memoryUsage() const override;
    BoundingBox boundingBox() const override;
    std::string getStatistics() const override;

    // Uniform Grid specific configuration
    void setCellSize(double size);
    void setCellCount(size_t cellsX, size_t cellsY, size_t cellsZ);
    double getCellSize() const { return m_cellSize; }
    std::array<size_t, 3> getCellCount() const { return {m_cellsX, m_cellsY, m_cellsZ}; }

private:
    /**
     * @brief Cell index (3D integer coordinates)
     */
    struct CellIndex {
        int x, y, z;

        bool operator==(const CellIndex& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    /**
     * @brief Hash function for CellIndex
     */
    struct CellIndexHash {
        size_t operator()(const CellIndex& idx) const {
            // Cantor pairing function for 3D
            size_t h1 = std::hash<int>{}(idx.x);
            size_t h2 = std::hash<int>{}(idx.y);
            size_t h3 = std::hash<int>{}(idx.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    /**
     * @brief Grid cell containing element IDs
     */
    struct GridCell {
        std::vector<ElementId> elements;
    };

    /**
     * @brief Get cell index from world position
     */
    CellIndex getCellIndex(const Eigen::Vector3d& point) const;

    /**
     * @brief Get cells intersecting a bounding box
     */
    std::vector<CellIndex> getCellsInBox(const BoundingBox& box) const;

    /**
     * @brief Check if cell index is valid
     */
    bool isValidCellIndex(const CellIndex& idx) const;

    /**
     * @brief Get cell at index (returns nullptr if empty)
     */
    const GridCell* getCell(const CellIndex& idx) const;

    /**
     * @brief Add element to cell
     */
    void addElementToCell(const CellIndex& idx, ElementId elemId);

    /**
     * @brief Spiral search for nearest neighbor
     */
    void spiralSearch(
        const Eigen::Vector3d& point,
        ElementId& nearestId,
        double& nearestDistSq,
        int maxRadius) const;

    /**
     * @brief Get neighbor cells in expanding radius
     */
    std::vector<CellIndex> getNeighborCells(const CellIndex& center, int radius) const;

    /**
     * @brief Ray-box intersection test
     */
    bool rayBoxIntersect(
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        const BoundingBox& box) const;

    /**
     * @brief DDA-based ray traversal through grid
     */
    void traverseRay(
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        std::vector<ElementId>& results) const;

    // Grid structure
    std::unordered_map<CellIndex, GridCell, CellIndexHash> m_cells;
    BoundingBox m_bounds;

    // Element bounds cache
    std::unordered_map<ElementId, BoundingBox> m_elementBounds;

    // Grid configuration
    double m_cellSize;
    size_t m_cellsX, m_cellsY, m_cellsZ;
    bool m_useCellCount;  // true = use cell count, false = use cell size

    // Statistics
    size_t m_elementCount;
    bool m_isBuilt;
};

} // namespace core
} // namespace koomesh
