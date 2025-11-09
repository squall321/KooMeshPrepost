/**
 * @file CustomInteractorStyle.cpp
 * @brief Implementation of CustomInteractorStyle
 */

#include "visualization/CustomInteractorStyle.h"

#ifdef KOOMESH_HAS_VTK
#include "visualization/ActorManager.h"
#include "visualization/CameraController.h"
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkActor.h>
#include <vtkProp3DCollection.h>
#include <vtkCoordinate.h>
#endif

namespace koomesh {
namespace visualization {

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// Constructor / Destructor
// ============================================================================

CustomInteractorStyle::CustomInteractorStyle()
    : m_renderer(nullptr)
    , m_actorManager(nullptr)
    , m_cameraController(nullptr)
    , m_interactionMode(InteractionMode::CAMERA)
    , m_pickingEnabled(true)
    , m_rubberBandEnabled(false)
    , m_rubberBandActive(false)
{
    m_propPicker = vtkSmartPointer<vtkPropPicker>::New();
    m_areaPicker = vtkSmartPointer<vtkAreaPicker>::New();

    m_rubberBandStart[0] = 0;
    m_rubberBandStart[1] = 0;
    m_rubberBandEnd[0] = 0;
    m_rubberBandEnd[1] = 0;
}

CustomInteractorStyle::~CustomInteractorStyle() = default;

// ============================================================================
// Setup
// ============================================================================

void CustomInteractorStyle::setRenderer(vtkRenderer* renderer) {
    m_renderer = renderer;
    if (m_areaPicker) {
        m_areaPicker->SetRenderer(renderer);
    }
}

vtkRenderer* CustomInteractorStyle::getRenderer() const {
    return m_renderer;
}

void CustomInteractorStyle::setActorManager(ActorManager* manager) {
    m_actorManager = manager;
}

ActorManager* CustomInteractorStyle::getActorManager() const {
    return m_actorManager;
}

void CustomInteractorStyle::setCameraController(CameraController* controller) {
    m_cameraController = controller;
}

CameraController* CustomInteractorStyle::getCameraController() const {
    return m_cameraController;
}

// ============================================================================
// Interaction Mode
// ============================================================================

void CustomInteractorStyle::setInteractionMode(InteractionMode mode) {
    m_interactionMode = mode;

    // Configure behavior based on mode
    switch (mode) {
        case InteractionMode::CAMERA:
            m_pickingEnabled = false;
            m_rubberBandEnabled = false;
            break;

        case InteractionMode::SELECT_SINGLE:
            m_pickingEnabled = true;
            m_rubberBandEnabled = false;
            break;

        case InteractionMode::SELECT_AREA:
            m_pickingEnabled = true;
            m_rubberBandEnabled = true;
            break;

        case InteractionMode::MEASURE:
        case InteractionMode::CUSTOM:
            m_pickingEnabled = true;
            m_rubberBandEnabled = false;
            break;
    }
}

InteractionMode CustomInteractorStyle::getInteractionMode() const {
    return m_interactionMode;
}

void CustomInteractorStyle::setPickingEnabled(bool enable) {
    m_pickingEnabled = enable;
}

bool CustomInteractorStyle::isPickingEnabled() const {
    return m_pickingEnabled;
}

// ============================================================================
// Event Callbacks
// ============================================================================

void CustomInteractorStyle::setPickCallback(PickCallback callback) {
    m_pickCallback = callback;
}

void CustomInteractorStyle::setAreaSelectionCallback(AreaSelectionCallback callback) {
    m_areaSelectionCallback = callback;
}

void CustomInteractorStyle::setKeyCallback(KeyCallback callback) {
    m_keyCallback = callback;
}

// ============================================================================
// VTK Event Handlers
// ============================================================================

void CustomInteractorStyle::OnLeftButtonDown() {
    if (m_interactionMode == InteractionMode::SELECT_SINGLE && m_pickingEnabled) {
        handlePick(MouseButton::LEFT);
    } else if (m_interactionMode == InteractionMode::SELECT_AREA && m_rubberBandEnabled) {
        startRubberBand();
    } else if (m_interactionMode == InteractionMode::CAMERA) {
        // Delegate to parent for camera rotation
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }
}

void CustomInteractorStyle::OnLeftButtonUp() {
    if (m_rubberBandActive) {
        endRubberBand();
    } else if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
    }
}

void CustomInteractorStyle::OnMiddleButtonDown() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
    } else if (m_pickingEnabled) {
        handlePick(MouseButton::MIDDLE);
    }
}

void CustomInteractorStyle::OnMiddleButtonUp() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnMiddleButtonUp();
    }
}

void CustomInteractorStyle::OnRightButtonDown() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnRightButtonDown();
    } else if (m_pickingEnabled) {
        handlePick(MouseButton::RIGHT);
    }
}

void CustomInteractorStyle::OnRightButtonUp() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnRightButtonUp();
    }
}

void CustomInteractorStyle::OnMouseMove() {
    if (m_rubberBandActive) {
        updateRubberBand();
    } else if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnMouseMove();
    }
}

void CustomInteractorStyle::OnMouseWheelForward() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
    }
}

void CustomInteractorStyle::OnMouseWheelBackward() {
    if (m_interactionMode == InteractionMode::CAMERA) {
        vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
    }
}

void CustomInteractorStyle::OnKeyPress() {
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (!interactor) {
        return;
    }

    std::string key = interactor->GetKeySym();

    // Execute keyboard shortcut if registered
    executeKeyboardShortcut(key);

    // Call user callback
    if (m_keyCallback) {
        m_keyCallback(key, getKeyModifiers());
    }

    // Default key handling
    if (key == "Escape") {
        if (m_rubberBandActive) {
            m_rubberBandActive = false;
            if (interactor->GetRenderWindow()) {
                interactor->GetRenderWindow()->Render();
            }
        }
    }

    // Delegate to parent
    vtkInteractorStyleTrackballCamera::OnKeyPress();
}

void CustomInteractorStyle::OnKeyRelease() {
    vtkInteractorStyleTrackballCamera::OnKeyRelease();
}

// ============================================================================
// Picking
// ============================================================================

PickEvent CustomInteractorStyle::pickAtCurrentPosition() {
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (!interactor) {
        return PickEvent();
    }

    int* pos = interactor->GetEventPosition();
    return pickAtPosition(pos[0], pos[1]);
}

PickEvent CustomInteractorStyle::pickAtPosition(int x, int y) {
    PickEvent event;
    event.displayPosition[0] = static_cast<double>(x);
    event.displayPosition[1] = static_cast<double>(y);
    event.modifiers = getKeyModifiers();

    if (!m_renderer || !m_propPicker) {
        return event;
    }

    int pickResult = m_propPicker->Pick(
        static_cast<double>(x),
        static_cast<double>(y),
        0.0,
        m_renderer
    );

    if (pickResult != 0) {
        event.validPick = true;
        m_propPicker->GetPickPosition(event.worldPosition);
        event.actor = m_propPicker->GetActor();
    }

    m_lastPickEvent = event;
    return event;
}

std::vector<vtkActor*> CustomInteractorStyle::pickInArea(int x1, int y1, int x2, int y2) {
    std::vector<vtkActor*> actors;

    if (!m_renderer || !m_areaPicker) {
        return actors;
    }

    int pickResult = m_areaPicker->AreaPick(
        static_cast<double>(x1),
        static_cast<double>(y1),
        static_cast<double>(x2),
        static_cast<double>(y2),
        m_renderer
    );

    if (pickResult != 0) {
        vtkProp3DCollection* props = m_areaPicker->GetProp3Ds();
        if (props) {
            props->InitTraversal();
            for (vtkIdType i = 0; i < props->GetNumberOfItems(); ++i) {
                vtkActor* actor = vtkActor::SafeDownCast(props->GetNextProp3D());
                if (actor) {
                    actors.push_back(actor);
                }
            }
        }
    }

    return actors;
}

// ============================================================================
// Rubber Band Selection
// ============================================================================

void CustomInteractorStyle::setRubberBandEnabled(bool enable) {
    m_rubberBandEnabled = enable;
}

bool CustomInteractorStyle::isRubberBandEnabled() const {
    return m_rubberBandEnabled;
}

void CustomInteractorStyle::startRubberBand() {
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (!interactor) {
        return;
    }

    int* pos = interactor->GetEventPosition();
    m_rubberBandStart[0] = pos[0];
    m_rubberBandStart[1] = pos[1];
    m_rubberBandEnd[0] = pos[0];
    m_rubberBandEnd[1] = pos[1];
    m_rubberBandActive = true;
}

void CustomInteractorStyle::updateRubberBand() {
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (!interactor || !m_rubberBandActive) {
        return;
    }

    int* pos = interactor->GetEventPosition();
    m_rubberBandEnd[0] = pos[0];
    m_rubberBandEnd[1] = pos[1];

    // Render to show rubber band (would need overlay in real implementation)
    if (interactor->GetRenderWindow()) {
        interactor->GetRenderWindow()->Render();
    }
}

void CustomInteractorStyle::endRubberBand() {
    if (!m_rubberBandActive) {
        return;
    }

    m_rubberBandActive = false;

    // Perform area selection
    handleAreaSelection();

    // Render
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (interactor && interactor->GetRenderWindow()) {
        interactor->GetRenderWindow()->Render();
    }
}

// ============================================================================
// Keyboard Shortcuts
// ============================================================================

void CustomInteractorStyle::addKeyboardShortcut(
    const std::string& key,
    std::function<void()> callback
) {
    m_keyboardShortcuts[key] = callback;
}

void CustomInteractorStyle::removeKeyboardShortcut(const std::string& key) {
    m_keyboardShortcuts.erase(key);
}

void CustomInteractorStyle::clearKeyboardShortcuts() {
    m_keyboardShortcuts.clear();
}

// ============================================================================
// Utility
// ============================================================================

void CustomInteractorStyle::displayToWorld(
    const int display[2],
    double world[3],
    double z
) {
    if (!m_renderer) {
        return;
    }

    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToDisplay();
    coord->SetValue(display[0], display[1], z);

    double* worldCoord = coord->GetComputedWorldValue(m_renderer);
    world[0] = worldCoord[0];
    world[1] = worldCoord[1];
    world[2] = worldCoord[2];
}

void CustomInteractorStyle::worldToDisplay(const double world[3], int display[2]) {
    if (!m_renderer) {
        return;
    }

    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToWorld();
    coord->SetValue(world[0], world[1], world[2]);

    int* displayCoord = coord->GetComputedDisplayValue(m_renderer);
    display[0] = displayCoord[0];
    display[1] = displayCoord[1];
}

KeyModifiers CustomInteractorStyle::getKeyModifiers() const {
    KeyModifiers modifiers;

    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    if (interactor) {
        modifiers.shift = (interactor->GetShiftKey() != 0);
        modifiers.ctrl = (interactor->GetControlKey() != 0);
        modifiers.alt = (interactor->GetAltKey() != 0);
    }

    return modifiers;
}

// ============================================================================
// Protected Helper Methods
// ============================================================================

void CustomInteractorStyle::handlePick(MouseButton button) {
    PickEvent event = pickAtCurrentPosition();

    if (event.validPick && m_actorManager) {
        // Find actor ID in actor manager
        vtkActor* actor = static_cast<vtkActor*>(event.actor);
        if (actor) {
            // Highlight the actor
            auto actorIds = m_actorManager->getAllActorIds();
            for (auto id : actorIds) {
                if (m_actorManager->getActor(id) == actor) {
                    KeyModifiers mods = getKeyModifiers();

                    if (mods.ctrl) {
                        // Toggle highlight
                        auto highlighted = m_actorManager->getHighlightedActors();
                        if (highlighted.find(id) != highlighted.end()) {
                            m_actorManager->unhighlightActor(id);
                        } else {
                            m_actorManager->highlightActor(id);
                        }
                    } else {
                        // Replace selection
                        m_actorManager->clearHighlights();
                        m_actorManager->highlightActor(id);
                    }
                    break;
                }
            }
        }
    }

    // Call user callback
    if (m_pickCallback) {
        m_pickCallback(event);
    }
}

void CustomInteractorStyle::handleAreaSelection() {
    AreaSelectionEvent event;
    event.startPosition[0] = m_rubberBandStart[0];
    event.startPosition[1] = m_rubberBandStart[1];
    event.endPosition[0] = m_rubberBandEnd[0];
    event.endPosition[1] = m_rubberBandEnd[1];
    event.modifiers = getKeyModifiers();

    // Pick actors in area
    std::vector<vtkActor*> actors = pickInArea(
        m_rubberBandStart[0],
        m_rubberBandStart[1],
        m_rubberBandEnd[0],
        m_rubberBandEnd[1]
    );

    // Highlight picked actors
    if (m_actorManager && !actors.empty()) {
        KeyModifiers mods = getKeyModifiers();

        if (!mods.ctrl) {
            m_actorManager->clearHighlights();
        }

        auto allActorIds = m_actorManager->getAllActorIds();
        for (vtkActor* actor : actors) {
            for (auto id : allActorIds) {
                if (m_actorManager->getActor(id) == actor) {
                    m_actorManager->highlightActor(id);
                    break;
                }
            }
        }
    }

    // Call user callback
    if (m_areaSelectionCallback) {
        m_areaSelectionCallback(event);
    }
}

void CustomInteractorStyle::executeKeyboardShortcut(const std::string& key) {
    auto it = m_keyboardShortcuts.find(key);
    if (it != m_keyboardShortcuts.end()) {
        it->second();
    }
}

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
