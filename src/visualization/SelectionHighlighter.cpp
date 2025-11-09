/**
 * @file SelectionHighlighter.cpp
 * @brief Implementation of SelectionHighlighter
 */

#include "visualization/SelectionHighlighter.h"

#ifdef KOOMESH_HAS_VTK

#include "visualization/MeshToVTK.h"
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkGeometryFilter.h>
#include <vtkExtractEdges.h>
#include <vtkOutlineFilter.h>
#include <vtkProperty.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkIdTypeArray.h>
#include <algorithm>
#include <cmath>

namespace koomesh {
namespace visualization {

// ============================================================================
// Constructor / Destructor
// ============================================================================

SelectionHighlighter::SelectionHighlighter()
    : m_highlightActor(vtkSmartPointer<vtkActor>::New())
    , m_edgeActor(vtkSmartPointer<vtkActor>::New())
    , m_highlightGeometry(vtkSmartPointer<vtkUnstructuredGrid>::New())
    , m_currentRenderer(nullptr)
    , m_pulsePhase(0.0)
    , m_visible(true)
{
    // Initialize actors with default properties
    m_highlightActor->GetProperty()->SetLighting(true);
    m_edgeActor->GetProperty()->SetLighting(false);

    // Set initial style
    updateProperties();
}

SelectionHighlighter::~SelectionHighlighter()
{
    if (m_currentRenderer) {
        removeFromRenderer(m_currentRenderer);
    }
}

// ============================================================================
// Selection Management
// ============================================================================

void SelectionHighlighter::setSelectedElements(
    const core::Mesh& mesh,
    const std::vector<core::ElementId>& elementIds)
{
    m_selectedElements.clear();
    m_selectedElements.insert(elementIds.begin(), elementIds.end());

    if (!m_selectedElements.empty()) {
        buildHighlightActor(mesh);
    } else {
        // Clear actors if no selection
        m_highlightActor->SetMapper(nullptr);
        m_edgeActor->SetMapper(nullptr);
    }
}

void SelectionHighlighter::addToSelection(
    const core::Mesh& mesh,
    const std::vector<core::ElementId>& elementIds)
{
    bool changed = false;
    for (const auto& id : elementIds) {
        if (m_selectedElements.insert(id).second) {
            changed = true;
        }
    }

    if (changed) {
        buildHighlightActor(mesh);
    }
}

void SelectionHighlighter::removeFromSelection(
    const std::vector<core::ElementId>& elementIds)
{
    bool changed = false;
    for (const auto& id : elementIds) {
        if (m_selectedElements.erase(id) > 0) {
            changed = true;
        }
    }

    if (changed && m_currentRenderer) {
        // Need to rebuild with current mesh - caller must provide mesh
        // For now, just clear if empty
        if (m_selectedElements.empty()) {
            m_highlightActor->SetMapper(nullptr);
            m_edgeActor->SetMapper(nullptr);
        }
    }
}

void SelectionHighlighter::clearSelection()
{
    m_selectedElements.clear();
    m_highlightActor->SetMapper(nullptr);
    m_edgeActor->SetMapper(nullptr);
}

const std::set<core::ElementId>& SelectionHighlighter::getSelectedElements() const
{
    return m_selectedElements;
}

bool SelectionHighlighter::isSelected(core::ElementId elementId) const
{
    return m_selectedElements.find(elementId) != m_selectedElements.end();
}

size_t SelectionHighlighter::getSelectionCount() const
{
    return m_selectedElements.size();
}

// ============================================================================
// Style Management
// ============================================================================

void SelectionHighlighter::setHighlightStyle(const SelectionHighlightStyle& style)
{
    m_style = style;
    updateProperties();
}

const SelectionHighlightStyle& SelectionHighlighter::getHighlightStyle() const
{
    return m_style;
}

void SelectionHighlighter::setHighlightColor(double r, double g, double b)
{
    m_style.color[0] = r;
    m_style.color[1] = g;
    m_style.color[2] = b;
    updateProperties();
}

void SelectionHighlighter::setEdgeColor(double r, double g, double b)
{
    m_style.edgeColor[0] = r;
    m_style.edgeColor[1] = g;
    m_style.edgeColor[2] = b;
    updateProperties();
}

void SelectionHighlighter::setOpacity(double opacity)
{
    m_style.opacity = std::clamp(opacity, 0.0, 1.0);
    updateProperties();
}

void SelectionHighlighter::setLineWidth(double width)
{
    m_style.lineWidth = std::max(width, 0.1);
    updateProperties();
}

void SelectionHighlighter::setHighlightMode(HighlightMode mode)
{
    m_style.mode = mode;
    updateProperties();
}

// ============================================================================
// VTK Integration
// ============================================================================

vtkActor* SelectionHighlighter::getHighlightActor() const
{
    return m_highlightActor;
}

vtkActor* SelectionHighlighter::getEdgeActor() const
{
    return m_edgeActor;
}

void SelectionHighlighter::addToRenderer(vtkRenderer* renderer)
{
    if (!renderer || m_currentRenderer == renderer) {
        return;
    }

    // Remove from previous renderer if any
    if (m_currentRenderer) {
        removeFromRenderer(m_currentRenderer);
    }

    renderer->AddActor(m_highlightActor);
    renderer->AddActor(m_edgeActor);
    m_currentRenderer = renderer;
}

void SelectionHighlighter::removeFromRenderer(vtkRenderer* renderer)
{
    if (!renderer) {
        return;
    }

    renderer->RemoveActor(m_highlightActor);
    renderer->RemoveActor(m_edgeActor);

    if (m_currentRenderer == renderer) {
        m_currentRenderer = nullptr;
    }
}

bool SelectionHighlighter::isAddedToRenderer() const
{
    return m_currentRenderer != nullptr;
}

void SelectionHighlighter::setVisible(bool visible)
{
    m_visible = visible;
    m_highlightActor->SetVisibility(visible);
    m_edgeActor->SetVisibility(visible &&
        (m_style.mode == HighlightMode::SURFACE_WITH_EDGES ||
         m_style.mode == HighlightMode::WIREFRAME));
}

bool SelectionHighlighter::isVisible() const
{
    return m_visible;
}

// ============================================================================
// Animation
// ============================================================================

void SelectionHighlighter::setPulseAnimation(bool enable)
{
    m_style.enablePulse = enable;
    m_pulsePhase = 0.0;
}

void SelectionHighlighter::updateAnimation(double deltaTime)
{
    if (m_style.enablePulse && !m_selectedElements.empty()) {
        updatePulse(deltaTime);
    }
}

// ============================================================================
// Utility
// ============================================================================

void SelectionHighlighter::updateGeometry(const core::Mesh& mesh)
{
    buildHighlightActor(mesh);
}

void SelectionHighlighter::updateProperties()
{
    applyStyleToActor(m_highlightActor, false);
    applyStyleToActor(m_edgeActor, true);
}

// ============================================================================
// Private Methods
// ============================================================================

void SelectionHighlighter::buildHighlightActor(const core::Mesh& mesh)
{
    if (m_selectedElements.empty()) {
        m_highlightActor->SetMapper(nullptr);
        m_edgeActor->SetMapper(nullptr);
        return;
    }

    // Convert selected elements to vector
    std::vector<core::ElementId> selectedIds(
        m_selectedElements.begin(),
        m_selectedElements.end()
    );

    // Use MeshToVTK to convert selected elements
    ConversionOptions options;
    options.computeNormals = true;
    options.generateEdges = (m_style.mode == HighlightMode::SURFACE_WITH_EDGES ||
                            m_style.mode == HighlightMode::WIREFRAME);

    auto actor = MeshToVTK::convertGroupToActor(mesh, selectedIds, options);

    if (actor && actor->GetMapper()) {
        // Copy mapper to our actor
        m_highlightActor->SetMapper(actor->GetMapper());

        // Apply style
        applyStyleToActor(m_highlightActor, false);

        // Build edge actor if needed
        if (m_style.mode == HighlightMode::SURFACE_WITH_EDGES) {
            buildEdgeActor(mesh);
        } else {
            m_edgeActor->SetMapper(nullptr);
        }
    }
}

void SelectionHighlighter::buildEdgeActor(const core::Mesh& mesh)
{
    if (m_selectedElements.empty() || !m_highlightActor->GetMapper()) {
        m_edgeActor->SetMapper(nullptr);
        return;
    }

    // Get the unstructured grid from main actor
    auto mapper = m_highlightActor->GetMapper();
    auto input = mapper->GetInput();

    if (!input) {
        return;
    }

    // Extract edges
    auto edgeFilter = vtkSmartPointer<vtkExtractEdges>::New();
    edgeFilter->SetInputData(input);
    edgeFilter->Update();

    // Create mapper for edges
    auto edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    edgeMapper->SetInputConnection(edgeFilter->GetOutputPort());
    edgeMapper->ScalarVisibilityOff();

    m_edgeActor->SetMapper(edgeMapper);
    applyStyleToActor(m_edgeActor, true);
}

void SelectionHighlighter::applyStyleToActor(vtkActor* actor, bool isEdgeActor)
{
    if (!actor) {
        return;
    }

    auto property = actor->GetProperty();

    if (isEdgeActor) {
        // Edge actor properties
        property->SetColor(m_style.edgeColor[0], m_style.edgeColor[1], m_style.edgeColor[2]);
        property->SetOpacity(m_style.edgeOpacity);
        property->SetLineWidth(m_style.lineWidth);
        property->SetRepresentationToWireframe();
        property->SetLighting(false);
    } else {
        // Main highlight actor properties
        property->SetColor(m_style.color[0], m_style.color[1], m_style.color[2]);
        property->SetOpacity(m_style.opacity);
        property->SetLineWidth(m_style.lineWidth);
        property->SetPointSize(m_style.pointSize);

        // Set representation based on mode
        switch (m_style.mode) {
            case HighlightMode::SOLID:
            case HighlightMode::SURFACE_WITH_EDGES:
            case HighlightMode::GLOW:
                property->SetRepresentationToSurface();
                break;
            case HighlightMode::WIREFRAME:
                property->SetRepresentationToWireframe();
                break;
            case HighlightMode::OUTLINE:
                property->SetRepresentationToWireframe();
                break;
        }

        // Lighting settings
        property->SetLighting(!m_style.depthPeeling);
        property->SetAmbient(0.3);
        property->SetDiffuse(0.7);
        property->SetSpecular(0.3);
        property->SetSpecularPower(20.0);

        // Backface culling
        if (m_style.backfaceCulling) {
            property->BackfaceCullingOn();
        } else {
            property->BackfaceCullingOff();
        }

        // Edge visibility (for surface mode)
        if (m_style.mode == HighlightMode::SURFACE_WITH_EDGES) {
            property->EdgeVisibilityOff(); // We use separate edge actor
        } else {
            property->EdgeVisibilityOff();
        }
    }

    // Apply visibility
    actor->SetVisibility(m_visible);
}

void SelectionHighlighter::createOutlineActor(const core::Mesh& mesh)
{
    // TODO: Implement outline mode if needed
    // This would create a bounding box outline around selected elements
}

void SelectionHighlighter::updatePulse(double deltaTime)
{
    // Update pulse phase
    m_pulsePhase += deltaTime * m_style.pulseSpeed * 2.0 * M_PI;
    if (m_pulsePhase > 2.0 * M_PI) {
        m_pulsePhase -= 2.0 * M_PI;
    }

    // Calculate pulse opacity
    double t = (std::sin(m_pulsePhase) + 1.0) * 0.5; // 0 to 1
    double opacity = m_style.pulseMinOpacity +
                    (m_style.opacity - m_style.pulseMinOpacity) * t;

    // Apply to actor
    if (m_highlightActor) {
        m_highlightActor->GetProperty()->SetOpacity(opacity);
    }
}

} // namespace visualization
} // namespace koomesh

#else // !KOOMESH_HAS_VTK

// Empty implementation when VTK is not available
namespace koomesh {
namespace visualization {

// No implementation needed for stub class

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
