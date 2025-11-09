/**
 * @file SelectionHighlighter.h
 * @brief Selection highlighting system for visualization
 */

#pragma once

#include "core/Mesh.h"
#include "core/Element.h"
#include <vector>
#include <set>
#include <memory>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Highlight display mode
 */
enum class HighlightMode {
    SOLID,              // Solid fill with highlight color
    WIREFRAME,          // Wireframe only
    SURFACE_WITH_EDGES, // Solid with edge overlay
    OUTLINE,            // Bounding box outline
    GLOW                // Glowing effect (solid + transparent overlay)
};

/**
 * @brief Selection highlight style configuration
 */
struct SelectionHighlightStyle {
    // Color settings
    double color[3] = {1.0, 1.0, 0.0};      // Highlight color (default: yellow)
    double edgeColor[3] = {1.0, 0.5, 0.0};  // Edge color (default: orange)

    // Transparency
    double opacity = 1.0;                    // Main opacity (0-1)
    double edgeOpacity = 1.0;                // Edge opacity (0-1)

    // Line properties
    double lineWidth = 3.0;                  // Line width for edges
    double pointSize = 5.0;                  // Point size for vertices

    // Display mode
    HighlightMode mode = HighlightMode::SURFACE_WITH_EDGES;

    // Rendering properties
    bool depthPeeling = false;               // Enable depth peeling for transparency
    bool backfaceCulling = false;            // Enable backface culling

    // Animation
    bool enablePulse = false;                // Pulse animation
    double pulseSpeed = 1.0;                 // Pulse speed (Hz)
    double pulseMinOpacity = 0.5;            // Minimum opacity during pulse
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Manages visualization of selected elements
 *
 * The SelectionHighlighter creates and manages VTK actors that display
 * highlighted versions of selected mesh elements. It provides:
 * - Separate actor for selected elements
 * - Customizable highlight styles
 * - Multiple display modes (solid, wireframe, outline, etc.)
 * - Optional animation effects
 * - Efficient updates when selection changes
 */
class SelectionHighlighter {
public:
    /**
     * @brief Constructor
     */
    SelectionHighlighter();

    /**
     * @brief Destructor
     */
    ~SelectionHighlighter();

    // ========================================================================
    // Selection Management
    // ========================================================================

    /**
     * @brief Set selected elements
     * @param mesh Source mesh
     * @param elementIds IDs of selected elements
     */
    void setSelectedElements(
        const core::Mesh& mesh,
        const std::vector<core::ElementId>& elementIds
    );

    /**
     * @brief Add elements to selection
     * @param mesh Source mesh
     * @param elementIds IDs to add to selection
     */
    void addToSelection(
        const core::Mesh& mesh,
        const std::vector<core::ElementId>& elementIds
    );

    /**
     * @brief Remove elements from selection
     * @param elementIds IDs to remove from selection
     */
    void removeFromSelection(
        const std::vector<core::ElementId>& elementIds
    );

    /**
     * @brief Clear all selections
     */
    void clearSelection();

    /**
     * @brief Get currently selected element IDs
     * @return Set of selected element IDs
     */
    const std::set<core::ElementId>& getSelectedElements() const;

    /**
     * @brief Check if element is selected
     * @param elementId Element ID to check
     * @return True if selected
     */
    bool isSelected(core::ElementId elementId) const;

    /**
     * @brief Get number of selected elements
     * @return Selection count
     */
    size_t getSelectionCount() const;

    // ========================================================================
    // Style Management
    // ========================================================================

    /**
     * @brief Set highlight style
     * @param style Highlight style configuration
     */
    void setHighlightStyle(const SelectionHighlightStyle& style);

    /**
     * @brief Get current highlight style
     * @return Current style
     */
    const SelectionHighlightStyle& getHighlightStyle() const;

    /**
     * @brief Set highlight color
     * @param r Red component (0-1)
     * @param g Green component (0-1)
     * @param b Blue component (0-1)
     */
    void setHighlightColor(double r, double g, double b);

    /**
     * @brief Set edge color
     * @param r Red component (0-1)
     * @param g Green component (0-1)
     * @param b Blue component (0-1)
     */
    void setEdgeColor(double r, double g, double b);

    /**
     * @brief Set highlight opacity
     * @param opacity Opacity value (0-1)
     */
    void setOpacity(double opacity);

    /**
     * @brief Set line width for edges
     * @param width Line width
     */
    void setLineWidth(double width);

    /**
     * @brief Set highlight mode
     * @param mode Display mode
     */
    void setHighlightMode(HighlightMode mode);

    // ========================================================================
    // VTK Integration
    // ========================================================================

    /**
     * @brief Get main highlight actor
     * @return VTK actor for highlighted elements
     */
    vtkActor* getHighlightActor() const;

    /**
     * @brief Get edge actor (for SURFACE_WITH_EDGES mode)
     * @return VTK actor for edges
     */
    vtkActor* getEdgeActor() const;

    /**
     * @brief Add highlight actors to renderer
     * @param renderer VTK renderer
     */
    void addToRenderer(vtkRenderer* renderer);

    /**
     * @brief Remove highlight actors from renderer
     * @param renderer VTK renderer
     */
    void removeFromRenderer(vtkRenderer* renderer);

    /**
     * @brief Check if actors are added to renderer
     * @return True if added
     */
    bool isAddedToRenderer() const;

    /**
     * @brief Set visibility of highlight
     * @param visible Visibility flag
     */
    void setVisible(bool visible);

    /**
     * @brief Get visibility status
     * @return True if visible
     */
    bool isVisible() const;

    // ========================================================================
    // Animation
    // ========================================================================

    /**
     * @brief Enable/disable pulse animation
     * @param enable Enable flag
     */
    void setPulseAnimation(bool enable);

    /**
     * @brief Update animation (call from render loop)
     * @param deltaTime Time since last update (seconds)
     */
    void updateAnimation(double deltaTime);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Force update of highlight geometry
     * @param mesh Source mesh
     */
    void updateGeometry(const core::Mesh& mesh);

    /**
     * @brief Update only visual properties (no geometry rebuild)
     */
    void updateProperties();

private:
    /**
     * @brief Build VTK actor from selected elements
     */
    void buildHighlightActor(const core::Mesh& mesh);

    /**
     * @brief Build edge actor for SURFACE_WITH_EDGES mode
     */
    void buildEdgeActor(const core::Mesh& mesh);

    /**
     * @brief Apply style to actor
     */
    void applyStyleToActor(vtkActor* actor, bool isEdgeActor = false);

    /**
     * @brief Create outline actor
     */
    void createOutlineActor(const core::Mesh& mesh);

    /**
     * @brief Update pulse animation
     */
    void updatePulse(double deltaTime);

    // VTK actors
    vtkSmartPointer<vtkActor> m_highlightActor;
    vtkSmartPointer<vtkActor> m_edgeActor;
    vtkSmartPointer<vtkUnstructuredGrid> m_highlightGeometry;

    // Selection data
    std::set<core::ElementId> m_selectedElements;

    // Style
    SelectionHighlightStyle m_style;

    // Renderer tracking
    vtkRenderer* m_currentRenderer;

    // Animation state
    double m_pulsePhase;
    bool m_visible;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class SelectionHighlighter {
public:
    SelectionHighlighter() {}
    ~SelectionHighlighter() {}

    void setSelectedElements(const core::Mesh&, const std::vector<core::ElementId>&) {}
    void addToSelection(const core::Mesh&, const std::vector<core::ElementId>&) {}
    void removeFromSelection(const std::vector<core::ElementId>&) {}
    void clearSelection() {}
    std::set<core::ElementId> getSelectedElements() const { return {}; }
    bool isSelected(core::ElementId) const { return false; }
    size_t getSelectionCount() const { return 0; }

    void setHighlightStyle(const SelectionHighlightStyle&) {}
    SelectionHighlightStyle getHighlightStyle() const { return SelectionHighlightStyle(); }
    void setHighlightColor(double, double, double) {}
    void setEdgeColor(double, double, double) {}
    void setOpacity(double) {}
    void setLineWidth(double) {}
    void setHighlightMode(HighlightMode) {}

    void* getHighlightActor() const { return nullptr; }
    void* getEdgeActor() const { return nullptr; }
    void addToRenderer(void*) {}
    void removeFromRenderer(void*) {}
    bool isAddedToRenderer() const { return false; }
    void setVisible(bool) {}
    bool isVisible() const { return false; }

    void setPulseAnimation(bool) {}
    void updateAnimation(double) {}

    void updateGeometry(const core::Mesh&) {}
    void updateProperties() {}
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
