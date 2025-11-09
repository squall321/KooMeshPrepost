/**
 * @file CustomInteractorStyle.h
 * @brief Custom VTK interactor style for mesh visualization
 */

#pragma once

#include <functional>
#include <memory>

#ifdef KOOMESH_HAS_VTK
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPropPicker.h>
#include <vtkAreaPicker.h>
#endif

namespace koomesh {
namespace visualization {

#ifdef KOOMESH_HAS_VTK
class ActorManager;
class CameraController;
#endif

/**
 * @brief Interaction mode
 */
enum class InteractionMode {
    CAMERA,         // Camera manipulation (rotate, pan, zoom)
    SELECT_SINGLE,  // Single element selection
    SELECT_AREA,    // Area/rubber band selection
    MEASURE,        // Distance/angle measurement
    CUSTOM          // Custom user-defined mode
};

/**
 * @brief Mouse button enumeration
 */
enum class MouseButton {
    LEFT,
    MIDDLE,
    RIGHT,
    NONE
};

/**
 * @brief Keyboard modifiers
 */
struct KeyModifiers {
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
};

/**
 * @brief Pick event data
 */
struct PickEvent {
    double worldPosition[3] = {0.0, 0.0, 0.0};
    double displayPosition[2] = {0.0, 0.0};
    void* actor = nullptr;
    bool validPick = false;
    KeyModifiers modifiers;
};

/**
 * @brief Area selection event data
 */
struct AreaSelectionEvent {
    int startPosition[2] = {0, 0};
    int endPosition[2] = {0, 0};
    KeyModifiers modifiers;
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Custom interactor style for mesh visualization
 *
 * Extends vtkInteractorStyleTrackballCamera with:
 * - Multiple interaction modes (camera, selection, measurement)
 * - Element picking and highlighting
 * - Rubber band area selection
 * - Customizable keyboard shortcuts
 * - Event callbacks for custom behavior
 * - Integration with ActorManager and CameraController
 */
class CustomInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static CustomInteractorStyle* New();
    vtkTypeMacro(CustomInteractorStyle, vtkInteractorStyleTrackballCamera);

    /**
     * @brief Constructor
     */
    CustomInteractorStyle();

    /**
     * @brief Destructor
     */
    ~CustomInteractorStyle() override;

    // ========================================================================
    // Setup
    // ========================================================================

    /**
     * @brief Set renderer
     * @param renderer VTK renderer
     */
    void setRenderer(vtkRenderer* renderer);

    /**
     * @brief Get renderer
     * @return Renderer pointer
     */
    vtkRenderer* getRenderer() const;

    /**
     * @brief Set actor manager
     * @param manager Actor manager
     */
    void setActorManager(ActorManager* manager);

    /**
     * @brief Get actor manager
     * @return Actor manager pointer
     */
    ActorManager* getActorManager() const;

    /**
     * @brief Set camera controller
     * @param controller Camera controller
     */
    void setCameraController(CameraController* controller);

    /**
     * @brief Get camera controller
     * @return Camera controller pointer
     */
    CameraController* getCameraController() const;

    // ========================================================================
    // Interaction Mode
    // ========================================================================

    /**
     * @brief Set interaction mode
     * @param mode Interaction mode
     */
    void setInteractionMode(InteractionMode mode);

    /**
     * @brief Get interaction mode
     * @return Current interaction mode
     */
    InteractionMode getInteractionMode() const;

    /**
     * @brief Enable/disable picking
     * @param enable Enable picking
     */
    void setPickingEnabled(bool enable);

    /**
     * @brief Check if picking is enabled
     * @return True if picking enabled
     */
    bool isPickingEnabled() const;

    // ========================================================================
    // Event Callbacks
    // ========================================================================

    /**
     * @brief Pick callback function type
     */
    using PickCallback = std::function<void(const PickEvent&)>;

    /**
     * @brief Area selection callback function type
     */
    using AreaSelectionCallback = std::function<void(const AreaSelectionEvent&)>;

    /**
     * @brief Key press callback function type
     */
    using KeyCallback = std::function<void(const std::string& key, const KeyModifiers&)>;

    /**
     * @brief Set pick callback
     * @param callback Pick callback function
     */
    void setPickCallback(PickCallback callback);

    /**
     * @brief Set area selection callback
     * @param callback Area selection callback function
     */
    void setAreaSelectionCallback(AreaSelectionCallback callback);

    /**
     * @brief Set key press callback
     * @param callback Key callback function
     */
    void setKeyCallback(KeyCallback callback);

    // ========================================================================
    // VTK Event Handlers (overrides)
    // ========================================================================

    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseMove() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
    void OnKeyPress() override;
    void OnKeyRelease() override;

    // ========================================================================
    // Picking
    // ========================================================================

    /**
     * @brief Pick at current mouse position
     * @return Pick event data
     */
    PickEvent pickAtCurrentPosition();

    /**
     * @brief Pick at display position
     * @param x X coordinate
     * @param y Y coordinate
     * @return Pick event data
     */
    PickEvent pickAtPosition(int x, int y);

    /**
     * @brief Get actors in area
     * @param x1 Start X
     * @param y1 Start Y
     * @param x2 End X
     * @param y2 End Y
     * @return Vector of picked actors
     */
    std::vector<vtkActor*> pickInArea(int x1, int y1, int x2, int y2);

    // ========================================================================
    // Rubber Band Selection
    // ========================================================================

    /**
     * @brief Enable/disable rubber band selection
     * @param enable Enable rubber band
     */
    void setRubberBandEnabled(bool enable);

    /**
     * @brief Check if rubber band is enabled
     * @return True if rubber band enabled
     */
    bool isRubberBandEnabled() const;

    /**
     * @brief Start rubber band selection
     */
    void startRubberBand();

    /**
     * @brief Update rubber band selection
     */
    void updateRubberBand();

    /**
     * @brief End rubber band selection
     */
    void endRubberBand();

    // ========================================================================
    // Keyboard Shortcuts
    // ========================================================================

    /**
     * @brief Add keyboard shortcut
     * @param key Key character or name
     * @param callback Callback function
     */
    void addKeyboardShortcut(const std::string& key, std::function<void()> callback);

    /**
     * @brief Remove keyboard shortcut
     * @param key Key character or name
     */
    void removeKeyboardShortcut(const std::string& key);

    /**
     * @brief Clear all keyboard shortcuts
     */
    void clearKeyboardShortcuts();

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Convert display to world coordinates
     * @param display Display coordinates [x, y]
     * @param world Output world coordinates [x, y, z]
     * @param z Z-depth (default 0 = near plane)
     */
    void displayToWorld(const int display[2], double world[3], double z = 0.0);

    /**
     * @brief Convert world to display coordinates
     * @param world World coordinates [x, y, z]
     * @param display Output display coordinates [x, y]
     */
    void worldToDisplay(const double world[3], int display[2]);

    /**
     * @brief Get current key modifiers
     * @return Key modifiers state
     */
    KeyModifiers getKeyModifiers() const;

protected:
    /**
     * @brief Handle pick event
     */
    void handlePick(MouseButton button);

    /**
     * @brief Handle area selection
     */
    void handleAreaSelection();

    /**
     * @brief Execute keyboard shortcut
     */
    void executeKeyboardShortcut(const std::string& key);

private:
    vtkRenderer* m_renderer;
    ActorManager* m_actorManager;
    CameraController* m_cameraController;

    InteractionMode m_interactionMode;
    bool m_pickingEnabled;
    bool m_rubberBandEnabled;

    vtkSmartPointer<vtkPropPicker> m_propPicker;
    vtkSmartPointer<vtkAreaPicker> m_areaPicker;

    // Rubber band selection state
    bool m_rubberBandActive;
    int m_rubberBandStart[2];
    int m_rubberBandEnd[2];

    // Callbacks
    PickCallback m_pickCallback;
    AreaSelectionCallback m_areaSelectionCallback;
    KeyCallback m_keyCallback;

    // Keyboard shortcuts
    std::map<std::string, std::function<void()>> m_keyboardShortcuts;

    // Last pick result
    PickEvent m_lastPickEvent;
};

vtkStandardNewMacro(CustomInteractorStyle);

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class CustomInteractorStyle {
public:
    CustomInteractorStyle() {}
    ~CustomInteractorStyle() {}

    void setRenderer(void*) {}
    void* getRenderer() const { return nullptr; }
    void setActorManager(void*) {}
    void* getActorManager() const { return nullptr; }
    void setCameraController(void*) {}
    void* getCameraController() const { return nullptr; }

    void setInteractionMode(InteractionMode) {}
    InteractionMode getInteractionMode() const { return InteractionMode::CAMERA; }
    void setPickingEnabled(bool) {}
    bool isPickingEnabled() const { return false; }

    void setPickCallback(std::function<void(const PickEvent&)>) {}
    void setAreaSelectionCallback(std::function<void(const AreaSelectionEvent&)>) {}
    void setKeyCallback(std::function<void(const std::string&, const KeyModifiers&)>) {}

    PickEvent pickAtCurrentPosition() { return PickEvent(); }
    PickEvent pickAtPosition(int, int) { return PickEvent(); }
    std::vector<void*> pickInArea(int, int, int, int) { return {}; }

    void setRubberBandEnabled(bool) {}
    bool isRubberBandEnabled() const { return false; }
    void startRubberBand() {}
    void updateRubberBand() {}
    void endRubberBand() {}

    void addKeyboardShortcut(const std::string&, std::function<void()>) {}
    void removeKeyboardShortcut(const std::string&) {}
    void clearKeyboardShortcuts() {}

    void displayToWorld(const int[2], double[3], double = 0.0) {}
    void worldToDisplay(const double[3], int[2]) {}
    KeyModifiers getKeyModifiers() const { return KeyModifiers(); }
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
