/**
 * @file MeshToVTK.h
 * @brief Conversion utilities for Mesh to VTK data structures
 */

#pragma once

#include "core/Mesh.h"
#include <vector>

#ifdef KOOMESH_HAS_VTK

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <memory>
#include <vector>

namespace koomesh {
namespace visualization {

/**
 * @brief Conversion options
 */
struct ConversionOptions {
    bool includeInternalFaces = false;
    bool computeNormals = true;
    bool generateEdges = false;
    double edgeAngle = 30.0;  // Feature edge angle in degrees
};

/**
 * @brief Conversion statistics
 */
struct ConversionStats {
    size_t nodesConverted = 0;
    size_t elementsConverted = 0;
    size_t facesGenerated = 0;
    size_t edgesGenerated = 0;
    double conversionTime = 0.0;
};

/**
 * @brief Utility class for converting Mesh to VTK data structures
 *
 * This class provides static methods to convert KooMesh data structures
 * to VTK formats for visualization. Supports:
 * - Unstructured grid for volume elements
 * - PolyData for surface meshes
 * - Automatic actor creation
 * - Normal computation
 * - Edge detection
 */
class MeshToVTK {
public:
    /**
     * @brief Convert mesh to VTK unstructured grid
     * @param mesh Source mesh
     * @param options Conversion options
     * @param stats Optional pointer to receive statistics
     * @return VTK unstructured grid
     */
    static vtkSmartPointer<vtkUnstructuredGrid> convert(
        const core::Mesh& mesh,
        const ConversionOptions& options = ConversionOptions(),
        ConversionStats* stats = nullptr
    );

    /**
     * @brief Convert mesh surface to VTK polydata
     * @param mesh Source mesh
     * @param options Conversion options
     * @param stats Optional pointer to receive statistics
     * @return VTK polydata
     */
    static vtkSmartPointer<vtkPolyData> convertToPolyData(
        const core::Mesh& mesh,
        const ConversionOptions& options = ConversionOptions(),
        ConversionStats* stats = nullptr
    );

    /**
     * @brief Convert mesh to VTK actor (ready for rendering)
     * @param mesh Source mesh
     * @param options Conversion options
     * @return VTK actor
     */
    static vtkSmartPointer<vtkActor> convertToActor(
        const core::Mesh& mesh,
        const ConversionOptions& options = ConversionOptions()
    );

    /**
     * @brief Convert group of elements to VTK actor
     * @param mesh Source mesh
     * @param elementIds Element IDs to include
     * @param options Conversion options
     * @return VTK actor
     */
    static vtkSmartPointer<vtkActor> convertGroupToActor(
        const core::Mesh& mesh,
        const std::vector<core::ElementId>& elementIds,
        const ConversionOptions& options = ConversionOptions()
    );

    /**
     * @brief Extract surface mesh from volume mesh
     * @param mesh Source volume mesh
     * @return VTK polydata containing only surface
     */
    static vtkSmartPointer<vtkPolyData> extractSurface(
        const core::Mesh& mesh
    );

    /**
     * @brief Convert edges to VTK polydata for wireframe rendering
     * @param mesh Source mesh
     * @return VTK polydata of edges
     */
    static vtkSmartPointer<vtkPolyData> convertEdges(
        const core::Mesh& mesh
    );

private:
    /**
     * @brief Add nodes to VTK points
     */
    static void addNodes(
        vtkUnstructuredGrid* grid,
        const core::Mesh& mesh
    );

    /**
     * @brief Add elements to VTK grid
     */
    static void addElements(
        vtkUnstructuredGrid* grid,
        const core::Mesh& mesh
    );

    /**
     * @brief Add specific elements to VTK grid
     */
    static void addSelectedElements(
        vtkUnstructuredGrid* grid,
        const core::Mesh& mesh,
        const std::vector<core::ElementId>& elementIds
    );

    /**
     * @brief Convert element type to VTK cell type
     */
    static int getVTKCellType(const core::Element* element);

    /**
     * @brief Get element face nodes for surface extraction
     */
    static std::vector<std::vector<core::NodeId>> getElementFaces(
        const core::Element* element,
        const core::Mesh& mesh
    );

    /**
     * @brief Check if face is on surface (has no neighbor)
     */
    static bool isSurfaceFace(
        const std::vector<core::NodeId>& face,
        const core::Mesh& mesh,
        const core::Element* element
    );
};

} // namespace visualization
} // namespace koomesh

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
namespace koomesh {
namespace visualization {

struct ConversionOptions {};
struct ConversionStats { double conversionTime = 0.0; };

class MeshToVTK {
public:
    // All methods return nullptr or do nothing
    static void* convert(const core::Mesh&, const ConversionOptions& = ConversionOptions(), ConversionStats* = nullptr) { return nullptr; }
    static void* convertToPolyData(const core::Mesh&, const ConversionOptions& = ConversionOptions(), ConversionStats* = nullptr) { return nullptr; }
    static void* convertToActor(const core::Mesh&, const ConversionOptions& = ConversionOptions()) { return nullptr; }
    static void* convertGroupToActor(const core::Mesh&, const std::vector<core::ElementId>&, const ConversionOptions& = ConversionOptions()) { return nullptr; }
    static void* extractSurface(const core::Mesh&) { return nullptr; }
    static void* convertEdges(const core::Mesh&) { return nullptr; }
};

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
