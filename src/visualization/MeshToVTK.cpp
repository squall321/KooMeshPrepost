/**
 * @file MeshToVTK.cpp
 * @brief Implementation of Mesh to VTK conversion
 */

#include "visualization/MeshToVTK.h"

#ifdef KOOMESH_HAS_VTK

#include "core/Element.h"
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkGeometryFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <chrono>
#include <map>
#include <set>

namespace koomesh {
namespace visualization {

// ============================================================================
// Public Methods
// ============================================================================

vtkSmartPointer<vtkUnstructuredGrid> MeshToVTK::convert(
    const core::Mesh& mesh,
    const ConversionOptions& options,
    ConversionStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    vtkSmartPointer<vtkUnstructuredGrid> grid =
        vtkSmartPointer<vtkUnstructuredGrid>::New();

    // Add nodes
    addNodes(grid, mesh);

    // Add elements
    addElements(grid, mesh);

    auto endTime = std::chrono::high_resolution_clock::now();

    if (stats) {
        stats->nodesConverted = mesh.nodeCount();
        stats->elementsConverted = mesh.elementCount();
        stats->conversionTime = std::chrono::duration<double>(endTime - startTime).count();
    }

    (void)options;  // Options used in future enhancements

    return grid;
}

vtkSmartPointer<vtkPolyData> MeshToVTK::convertToPolyData(
    const core::Mesh& mesh,
    const ConversionOptions& options,
    ConversionStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // First convert to unstructured grid
    auto grid = convert(mesh, options, stats);

    // Then extract surface
    vtkSmartPointer<vtkGeometryFilter> geometryFilter =
        vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputData(grid);

    if (options.computeNormals) {
        vtkSmartPointer<vtkPolyDataNormals> normalGenerator =
            vtkSmartPointer<vtkPolyDataNormals>::New();
        normalGenerator->SetInputConnection(geometryFilter->GetOutputPort());
        normalGenerator->SetFeatureAngle(options.edgeAngle);
        normalGenerator->Update();

        auto endTime = std::chrono::high_resolution_clock::now();

        if (stats) {
            stats->conversionTime = std::chrono::duration<double>(endTime - startTime).count();
        }

        return normalGenerator->GetOutput();
    } else {
        geometryFilter->Update();

        auto endTime = std::chrono::high_resolution_clock::now();

        if (stats) {
            stats->conversionTime = std::chrono::duration<double>(endTime - startTime).count();
        }

        return geometryFilter->GetOutput();
    }
}

vtkSmartPointer<vtkActor> MeshToVTK::convertToActor(
    const core::Mesh& mesh,
    const ConversionOptions& options
) {
    // Convert to polydata
    auto polyData = convertToPolyData(mesh, options);

    // Create mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    // Create actor
    vtkSmartPointer<vtkActor> actor =
        vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // Set default properties
    actor->GetProperty()->SetColor(0.8, 0.8, 0.9);
    actor->GetProperty()->SetEdgeVisibility(options.generateEdges);

    return actor;
}

vtkSmartPointer<vtkActor> MeshToVTK::convertGroupToActor(
    const core::Mesh& mesh,
    const std::vector<core::ElementId>& elementIds,
    const ConversionOptions& options
) {
    vtkSmartPointer<vtkUnstructuredGrid> grid =
        vtkSmartPointer<vtkUnstructuredGrid>::New();

    // Add nodes (all nodes, will be referenced by selected elements)
    addNodes(grid, mesh);

    // Add only selected elements
    addSelectedElements(grid, mesh, elementIds);

    // Convert to polydata
    vtkSmartPointer<vtkGeometryFilter> geometryFilter =
        vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputData(grid);

    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();

    if (options.computeNormals) {
        vtkSmartPointer<vtkPolyDataNormals> normalGenerator =
            vtkSmartPointer<vtkPolyDataNormals>::New();
        normalGenerator->SetInputConnection(geometryFilter->GetOutputPort());
        normalGenerator->SetFeatureAngle(options.edgeAngle);
        normalGenerator->Update();

        mapper->SetInputData(normalGenerator->GetOutput());
    } else {
        geometryFilter->Update();
        mapper->SetInputData(geometryFilter->GetOutput());
    }

    // Create actor
    vtkSmartPointer<vtkActor> actor =
        vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // Highlight color for selection
    actor->GetProperty()->SetColor(1.0, 0.5, 0.0);  // Orange
    actor->GetProperty()->SetEdgeVisibility(options.generateEdges);

    return actor;
}

vtkSmartPointer<vtkPolyData> MeshToVTK::extractSurface(
    const core::Mesh& mesh
) {
    auto grid = convert(mesh);

    vtkSmartPointer<vtkGeometryFilter> geometryFilter =
        vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputData(grid);
    geometryFilter->Update();

    return geometryFilter->GetOutput();
}

vtkSmartPointer<vtkPolyData> MeshToVTK::convertEdges(
    const core::Mesh& mesh
) {
    auto polyData = extractSurface(mesh);

    vtkSmartPointer<vtkFeatureEdges> featureEdges =
        vtkSmartPointer<vtkFeatureEdges>::New();
    featureEdges->SetInputData(polyData);
    featureEdges->BoundaryEdgesOn();
    featureEdges->FeatureEdgesOff();
    featureEdges->ManifoldEdgesOff();
    featureEdges->NonManifoldEdgesOff();
    featureEdges->Update();

    return featureEdges->GetOutput();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void MeshToVTK::addNodes(vtkUnstructuredGrid* grid, const core::Mesh& mesh) {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // Create map from NodeId to VTK point index
    std::map<core::NodeId, vtkIdType> nodeIdToVtkId;
    vtkIdType vtkId = 0;

    for (const auto& pair : mesh.nodes()) {
        const auto& node = pair.second;
        const auto& coords = node.coordinates();

        points->InsertNextPoint(coords.x(), coords.y(), coords.z());
        nodeIdToVtkId[node.id()] = vtkId++;
    }

    grid->SetPoints(points);

    // Store mapping for element conversion
    // Note: This is stored in a way that can be accessed during element conversion
    // In practice, we'd need to pass this map to addElements
}

void MeshToVTK::addElements(vtkUnstructuredGrid* grid, const core::Mesh& mesh) {
    // Create node ID to VTK ID mapping
    std::map<core::NodeId, vtkIdType> nodeIdToVtkId;
    vtkIdType vtkId = 0;
    for (const auto& pair : mesh.nodes()) {
        nodeIdToVtkId[pair.first] = vtkId++;
    }

    // Add each element
    for (const auto& pair : mesh.elements()) {
        const auto& element = pair.second;

        vtkSmartPointer<vtkIdList> cellPoints = vtkSmartPointer<vtkIdList>::New();

        for (const auto& nodeId : element->nodeIds()) {
            cellPoints->InsertNextId(nodeIdToVtkId[nodeId]);
        }

        int vtkCellType = getVTKCellType(element.get());
        grid->InsertNextCell(vtkCellType, cellPoints);
    }
}

void MeshToVTK::addSelectedElements(
    vtkUnstructuredGrid* grid,
    const core::Mesh& mesh,
    const std::vector<core::ElementId>& elementIds
) {
    // Create node ID to VTK ID mapping
    std::map<core::NodeId, vtkIdType> nodeIdToVtkId;
    vtkIdType vtkId = 0;
    for (const auto& pair : mesh.nodes()) {
        nodeIdToVtkId[pair.first] = vtkId++;
    }

    // Add only selected elements
    for (const auto& elemId : elementIds) {
        const auto& element = mesh.getElement(elemId);
        if (!element) {
            continue;
        }

        vtkSmartPointer<vtkIdList> cellPoints = vtkSmartPointer<vtkIdList>::New();

        for (const auto& nodeId : element->nodeIds()) {
            cellPoints->InsertNextId(nodeIdToVtkId[nodeId]);
        }

        int vtkCellType = getVTKCellType(element);
        grid->InsertNextCell(vtkCellType, cellPoints);
    }
}

int MeshToVTK::getVTKCellType(const core::Element* element) {
    using namespace core;

    // VTK cell type constants
    const int VTK_HEXAHEDRON = 12;
    const int VTK_TETRA = 10;
    const int VTK_WEDGE = 13;
    const int VTK_PYRAMID = 14;
    const int VTK_QUAD = 9;
    const int VTK_TRIANGLE = 5;

    ElementType type = element->type();

    switch (type) {
        case ElementType::HEXAHEDRON:
            return VTK_HEXAHEDRON;
        case ElementType::TETRAHEDRON:
            return VTK_TETRA;
        case ElementType::WEDGE:
            return VTK_WEDGE;
        case ElementType::PYRAMID:
            return VTK_PYRAMID;
        case ElementType::QUADRILATERAL:
            return VTK_QUAD;
        case ElementType::TRIANGLE:
            return VTK_TRIANGLE;
        default:
            return VTK_HEXAHEDRON;  // Default
    }
}

std::vector<std::vector<core::NodeId>> MeshToVTK::getElementFaces(
    const core::Element* element,
    const core::Mesh& mesh
) {
    // Placeholder: Extract faces from element
    // This would depend on element type
    (void)mesh;  // Unused

    std::vector<std::vector<core::NodeId>> faces;

    // For hexahedron, there are 6 faces
    if (element->type() == core::ElementType::HEXAHEDRON) {
        const auto& nodes = element->nodeIds();
        if (nodes.size() == 8) {
            // Define 6 faces of hexahedron
            faces.push_back({nodes[0], nodes[1], nodes[2], nodes[3]});
            faces.push_back({nodes[4], nodes[5], nodes[6], nodes[7]});
            faces.push_back({nodes[0], nodes[1], nodes[5], nodes[4]});
            faces.push_back({nodes[2], nodes[3], nodes[7], nodes[6]});
            faces.push_back({nodes[0], nodes[3], nodes[7], nodes[4]});
            faces.push_back({nodes[1], nodes[2], nodes[6], nodes[5]});
        }
    }

    // Add more element types as needed

    return faces;
}

bool MeshToVTK::isSurfaceFace(
    const std::vector<core::NodeId>& face,
    const core::Mesh& mesh,
    const core::Element* element
) {
    // Simplified: Check if face nodes are referenced by only one element
    // Full implementation would need to check neighboring elements

    (void)face;  // Unused
    (void)mesh;  // Unused
    (void)element;  // Unused

    // Placeholder: Always return true for now
    return true;
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
