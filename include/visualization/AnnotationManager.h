/**
 * @file AnnotationManager.h
 * @brief Annotation management for visualization (measurements, labels, axes)
 */

#pragma once

#include <Eigen/Core>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkTextActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkVectorText.h>
#include <vtkFollower.h>
#include <vtkCamera.h>
#include <vtkLeaderActor2D.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Annotation types
 */
enum class AnnotationType {
    DISTANCE_MEASUREMENT,   // Distance between two points
    ANGLE_MEASUREMENT,      // Angle between three points
    TEXT_LABEL,            // Free text label
    COORDINATE_AXES,       // 3D coordinate axes
    SCALAR_BAR,            // Color legend bar
    ARROW,                 // Directional arrow
    DIMENSION              // Dimension line with arrows
};

/**
 * @brief Annotation style configuration
 */
struct AnnotationStyle {
    // Text properties
    double fontSize = 12.0;
    Eigen::Vector3d textColor = {1.0, 1.0, 1.0};
    bool textBold = false;
    bool textItalic = false;
    std::string fontFamily = "Arial";

    // Line properties
    double lineWidth = 2.0;
    Eigen::Vector3d lineColor = {1.0, 1.0, 0.0};
    bool dashedLine = false;

    // Point properties
    double pointSize = 5.0;
    Eigen::Vector3d pointColor = {1.0, 0.0, 0.0};

    // Background
    bool useBackground = true;
    Eigen::Vector3d backgroundColor = {0.0, 0.0, 0.0};
    double backgroundOpacity = 0.7;
};

/**
 * @brief Annotation data structure
 */
struct Annotation {
    int id;
    AnnotationType type;
    std::string text;
    std::vector<Eigen::Vector3d> points;
    AnnotationStyle style;
    bool visible = true;

#ifdef KOOMESH_HAS_VTK
    std::vector<vtkSmartPointer<vtkProp>> actors;
#endif
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Annotation manager for VTK-based visualization
 *
 * Provides comprehensive annotation capabilities:
 * - Distance measurements between points
 * - Angle measurements
 * - Text labels with 3D positioning
 * - Coordinate axes display
 * - Scalar bar (color legend)
 * - Dimension lines with arrows
 * - Custom annotations
 */
class AnnotationManager {
public:
    /**
     * @brief Constructor
     * @param renderer VTK renderer to add annotations to
     */
    explicit AnnotationManager(vtkRenderer* renderer);

    /**
     * @brief Destructor
     */
    ~AnnotationManager();

    // ========================================================================
    // Measurement Annotations
    // ========================================================================

    /**
     * @brief Add distance measurement between two points
     * @param p1 First point
     * @param p2 Second point
     * @param label Optional custom label (default: distance value)
     * @return Annotation ID
     */
    int addDistanceMeasurement(
        const Eigen::Vector3d& p1,
        const Eigen::Vector3d& p2,
        const std::string& label = ""
    );

    /**
     * @brief Add angle measurement between three points
     * @param p1 First point (vertex)
     * @param p2 Middle point (angle vertex)
     * @param p3 Third point
     * @param label Optional custom label (default: angle value)
     * @return Annotation ID
     */
    int addAngleMeasurement(
        const Eigen::Vector3d& p1,
        const Eigen::Vector3d& p2,
        const Eigen::Vector3d& p3,
        const std::string& label = ""
    );

    /**
     * @brief Add dimension line with arrows
     * @param p1 Start point
     * @param p2 End point
     * @param offset Offset distance from line
     * @return Annotation ID
     */
    int addDimensionLine(
        const Eigen::Vector3d& p1,
        const Eigen::Vector3d& p2,
        double offset = 0.1
    );

    // ========================================================================
    // Label Annotations
    // ========================================================================

    /**
     * @brief Add text label at 3D position
     * @param text Label text
     * @param position 3D position
     * @param attachedToCamera If true, always faces camera
     * @return Annotation ID
     */
    int addTextLabel(
        const std::string& text,
        const Eigen::Vector3d& position,
        bool attachedToCamera = true
    );

    /**
     * @brief Add 2D text label at screen position
     * @param text Label text
     * @param screenX X position (0-1, normalized)
     * @param screenY Y position (0-1, normalized)
     * @return Annotation ID
     */
    int addScreenLabel(
        const std::string& text,
        double screenX,
        double screenY
    );

    /**
     * @brief Add caption with leader line
     * @param text Caption text
     * @param position Attach point in 3D
     * @param offset Caption offset
     * @return Annotation ID
     */
    int addCaption(
        const std::string& text,
        const Eigen::Vector3d& position,
        const Eigen::Vector3d& offset = {0.1, 0.1, 0.0}
    );

    // ========================================================================
    // Axes and Reference
    // ========================================================================

    /**
     * @brief Add coordinate axes widget
     * @param interactive If true, user can interact with it
     * @return Annotation ID
     */
    int addCoordinateAxes(bool interactive = true);

    /**
     * @brief Add simple axes at origin
     * @param length Axes length
     * @return Annotation ID
     */
    int addSimpleAxes(double length = 1.0);

    /**
     * @brief Add orientation marker (small axes in corner)
     * @param cornerPosition Corner (0=lower-left, 1=lower-right, 2=upper-left, 3=upper-right)
     * @return Annotation ID
     */
    int addOrientationMarker(int cornerPosition = 3);

    // ========================================================================
    // Color Legend
    // ========================================================================

    /**
     * @brief Add scalar bar (color legend)
     * @param title Bar title
     * @param lookupTable VTK lookup table for colors
     * @param position Position (0=right, 1=left, 2=top, 3=bottom)
     * @return Annotation ID
     */
    int addScalarBar(
        const std::string& title,
        vtkLookupTable* lookupTable,
        int position = 0
    );

    /**
     * @brief Add custom scalar bar with full configuration
     * @param title Bar title
     * @param lookupTable VTK lookup table
     * @param width Width (0-1, normalized)
     * @param height Height (0-1, normalized)
     * @param posX X position (0-1)
     * @param posY Y position (0-1)
     * @return Annotation ID
     */
    int addCustomScalarBar(
        const std::string& title,
        vtkLookupTable* lookupTable,
        double width,
        double height,
        double posX,
        double posY
    );

    // ========================================================================
    // Annotation Management
    // ========================================================================

    /**
     * @brief Remove annotation by ID
     * @param id Annotation ID
     * @return True if removed successfully
     */
    bool removeAnnotation(int id);

    /**
     * @brief Remove all annotations
     */
    void clear();

    /**
     * @brief Remove all annotations of specific type
     * @param type Annotation type
     */
    void clearType(AnnotationType type);

    /**
     * @brief Get annotation count
     * @return Number of annotations
     */
    int getAnnotationCount() const;

    /**
     * @brief Check if annotation exists
     * @param id Annotation ID
     * @return True if exists
     */
    bool hasAnnotation(int id) const;

    // ========================================================================
    // Visibility Control
    // ========================================================================

    /**
     * @brief Set annotation visibility
     * @param id Annotation ID
     * @param visible Visibility flag
     */
    void setVisible(int id, bool visible);

    /**
     * @brief Set visibility for all annotations
     * @param visible Visibility flag
     */
    void setAllVisible(bool visible);

    /**
     * @brief Set visibility for annotation type
     * @param type Annotation type
     * @param visible Visibility flag
     */
    void setTypeVisible(AnnotationType type, bool visible);

    // ========================================================================
    // Style Configuration
    // ========================================================================

    /**
     * @brief Set default annotation style
     * @param style Style configuration
     */
    void setDefaultStyle(const AnnotationStyle& style);

    /**
     * @brief Get default style
     * @return Current default style
     */
    const AnnotationStyle& getDefaultStyle() const;

    /**
     * @brief Set style for specific annotation
     * @param id Annotation ID
     * @param style Style configuration
     */
    void setStyle(int id, const AnnotationStyle& style);

    /**
     * @brief Update annotation text
     * @param id Annotation ID
     * @param text New text
     */
    void updateText(int id, const std::string& text);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Update all annotations (call after camera changes)
     */
    void update();

    /**
     * @brief Get annotation info
     * @param id Annotation ID
     * @return Annotation data (nullptr if not found)
     */
    const Annotation* getAnnotation(int id) const;

    /**
     * @brief Get statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

private:
    vtkRenderer* m_renderer;
    std::vector<std::unique_ptr<Annotation>> m_annotations;
    AnnotationStyle m_defaultStyle;
    int m_nextId;

    /**
     * @brief Create distance measurement actors
     */
    void createDistanceMeasurement(Annotation* annotation);

    /**
     * @brief Create angle measurement actors
     */
    void createAngleMeasurement(Annotation* annotation);

    /**
     * @brief Create text label actors
     */
    void createTextLabel(Annotation* annotation, bool attachToCamera);

    /**
     * @brief Create scalar bar actor
     */
    void createScalarBar(Annotation* annotation, vtkLookupTable* lut);

    /**
     * @brief Apply style to annotation
     */
    void applyStyle(Annotation* annotation);

    /**
     * @brief Calculate distance between points
     */
    double calculateDistance(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2) const;

    /**
     * @brief Calculate angle between three points (in degrees)
     */
    double calculateAngle(
        const Eigen::Vector3d& p1,
        const Eigen::Vector3d& p2,
        const Eigen::Vector3d& p3
    ) const;

    /**
     * @brief Format number for display
     */
    std::string formatNumber(double value, int precision = 2) const;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class AnnotationManager {
public:
    explicit AnnotationManager(void* renderer) : m_renderer(renderer), m_nextId(0), m_count(0) {}
    ~AnnotationManager() {}

    int addDistanceMeasurement(const Eigen::Vector3d&, const Eigen::Vector3d&, const std::string& = "") {
        m_count++;
        return m_nextId++;
    }

    int addAngleMeasurement(const Eigen::Vector3d&, const Eigen::Vector3d&, const Eigen::Vector3d&, const std::string& = "") {
        m_count++;
        return m_nextId++;
    }

    int addDimensionLine(const Eigen::Vector3d&, const Eigen::Vector3d&, double = 0.1) {
        m_count++;
        return m_nextId++;
    }

    int addTextLabel(const std::string&, const Eigen::Vector3d&, bool = true) {
        m_count++;
        return m_nextId++;
    }

    int addScreenLabel(const std::string&, double, double) {
        m_count++;
        return m_nextId++;
    }

    int addCaption(const std::string&, const Eigen::Vector3d&, const Eigen::Vector3d& = {0.1, 0.1, 0.0}) {
        m_count++;
        return m_nextId++;
    }

    int addCoordinateAxes(bool = true) {
        m_count++;
        return m_nextId++;
    }

    int addSimpleAxes(double = 1.0) {
        m_count++;
        return m_nextId++;
    }

    int addOrientationMarker(int = 3) {
        m_count++;
        return m_nextId++;
    }

    int addScalarBar(const std::string&, void*, int = 0) {
        m_count++;
        return m_nextId++;
    }

    int addCustomScalarBar(const std::string&, void*, double, double, double, double) {
        m_count++;
        return m_nextId++;
    }

    bool removeAnnotation(int id) {
        if (id >= 0 && id < m_nextId && m_count > 0) {
            m_count--;
            return true;
        }
        return false;
    }

    void clear() {
        m_count = 0;
    }

    void clearType(AnnotationType) {}

    int getAnnotationCount() const { return m_count; }

    bool hasAnnotation(int id) const { return id >= 0 && id < m_nextId && m_count > 0; }

    void setVisible(int, bool) {}
    void setAllVisible(bool) {}
    void setTypeVisible(AnnotationType, bool) {}

    void setDefaultStyle(const AnnotationStyle&) {}
    const AnnotationStyle& getDefaultStyle() const { return m_defaultStyle; }
    void setStyle(int, const AnnotationStyle&) {}
    void updateText(int, const std::string&) {}

    void update() {}
    const Annotation* getAnnotation(int) const { return nullptr; }

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Annotation Manager Statistics (VTK not available):\n";
        oss << "  Total Annotations: " << m_count << "\n";
        return oss.str();
    }

private:
    void* m_renderer;
    int m_nextId;
    int m_count;
    AnnotationStyle m_defaultStyle;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
