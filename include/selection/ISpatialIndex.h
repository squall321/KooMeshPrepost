#pragma once

#include "core/Element.h"
#include <vector>

namespace koomesh {
namespace core {

// Forward declaration
class BoundingBox;

/**
 * @brief Interface for spatial indexing structures
 *
 * Placeholder interface for future spatial indexing implementation (Phase 41+)
 */
class ISpatialIndex {
public:
    virtual ~ISpatialIndex() = default;

    /**
     * @brief Build the spatial index from elements
     */
    virtual void build(const std::unordered_map<ElementId, Element>& elements) = 0;

    /**
     * @brief Find elements in a bounding box
     */
    virtual std::vector<ElementId> findInBox(const BoundingBox& box) const = 0;

    /**
     * @brief Clear the index
     */
    virtual void clear() = 0;
};

} // namespace core
} // namespace koomesh
