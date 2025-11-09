/**
 * @file ActorManager.h
 * @brief Management of VTK actors for visualization
 */

#pragma once

#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Actor identifier type
 */
using ActorId = size_t;

/**
 * @brief Actor group identifier type
 */
using GroupId = size_t;

/**
 * @brief Actor display properties
 */
struct ActorProperties {
    double color[3] = {0.8, 0.8, 0.8};      // RGB color
    double opacity = 1.0;                    // Transparency (0-1)
    double lineWidth = 1.0;                  // Line width for wireframe
    double pointSize = 1.0;                  // Point size for vertex rendering
    bool visible = true;                     // Visibility flag
    bool lighting = true;                    // Enable lighting
    int representation = 2;                  // 0=points, 1=wireframe, 2=surface
    double specular = 0.0;                   // Specular reflection
    double specularPower = 1.0;              // Specular power
    double ambient = 0.0;                    // Ambient lighting
    double diffuse = 1.0;                    // Diffuse lighting
    bool edgeVisibility = false;             // Show edges on surface
    double edgeColor[3] = {0.0, 0.0, 0.0};  // Edge color
};

/**
 * @brief Selection highlight style
 */
struct HighlightStyle {
    double color[3] = {1.0, 1.0, 0.0};      // Highlight color (yellow)
    double opacity = 1.0;                    // Highlight opacity
    double lineWidth = 3.0;                  // Line width when highlighted
    bool edgeVisibility = true;              // Show edges when highlighted
    double edgeColor[3] = {1.0, 0.0, 0.0};  // Edge color (red)
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Actor information structure
 */
struct ActorInfo {
    vtkSmartPointer<vtkActor> actor;
    ActorProperties properties;
    ActorProperties originalProperties;  // Saved for restoring after highlight
    std::string name;
    GroupId group = 0;
    bool isHighlighted = false;
};

/**
 * @brief Manager for VTK actors in visualization
 *
 * Provides centralized management of VTK actors including:
 * - Actor registration and lifecycle
 * - Property management (color, opacity, visibility)
 * - Grouping and batch operations
 * - Selection highlighting
 * - Visibility control
 */
class ActorManager {
public:
    /**
     * @brief Constructor
     */
    ActorManager();

    /**
     * @brief Destructor
     */
    ~ActorManager();

    // ========================================================================
    // Actor Registration
    // ========================================================================

    /**
     * @brief Add actor to manager
     * @param actor VTK actor to add
     * @param name Optional actor name
     * @param properties Initial properties
     * @return Actor ID
     */
    ActorId addActor(
        vtkSmartPointer<vtkActor> actor,
        const std::string& name = "",
        const ActorProperties& properties = ActorProperties()
    );

    /**
     * @brief Remove actor from manager
     * @param id Actor ID
     * @return True if removed
     */
    bool removeActor(ActorId id);

    /**
     * @brief Remove all actors
     */
    void removeAllActors();

    /**
     * @brief Check if actor exists
     * @param id Actor ID
     * @return True if exists
     */
    bool hasActor(ActorId id) const;

    /**
     * @brief Get actor count
     * @return Number of actors
     */
    size_t getActorCount() const;

    /**
     * @brief Get actor by ID
     * @param id Actor ID
     * @return Actor pointer, or nullptr if not found
     */
    vtkActor* getActor(ActorId id) const;

    /**
     * @brief Get all actor IDs
     * @return Vector of actor IDs
     */
    std::vector<ActorId> getAllActorIds() const;

    // ========================================================================
    // Property Management
    // ========================================================================

    /**
     * @brief Set actor properties
     * @param id Actor ID
     * @param properties Properties to set
     * @return True if successful
     */
    bool setActorProperties(ActorId id, const ActorProperties& properties);

    /**
     * @brief Get actor properties
     * @param id Actor ID
     * @param properties Output properties
     * @return True if successful
     */
    bool getActorProperties(ActorId id, ActorProperties& properties) const;

    /**
     * @brief Set actor color
     * @param id Actor ID
     * @param r Red (0-1)
     * @param g Green (0-1)
     * @param b Blue (0-1)
     * @return True if successful
     */
    bool setActorColor(ActorId id, double r, double g, double b);

    /**
     * @brief Set actor opacity
     * @param id Actor ID
     * @param opacity Opacity (0-1)
     * @return True if successful
     */
    bool setActorOpacity(ActorId id, double opacity);

    /**
     * @brief Set actor visibility
     * @param id Actor ID
     * @param visible Visibility flag
     * @return True if successful
     */
    bool setActorVisibility(ActorId id, bool visible);

    /**
     * @brief Set actor representation
     * @param id Actor ID
     * @param representation 0=points, 1=wireframe, 2=surface
     * @return True if successful
     */
    bool setActorRepresentation(ActorId id, int representation);

    // ========================================================================
    // Group Management
    // ========================================================================

    /**
     * @brief Create actor group
     * @return Group ID
     */
    GroupId createGroup();

    /**
     * @brief Add actor to group
     * @param actorId Actor ID
     * @param groupId Group ID
     * @return True if successful
     */
    bool addActorToGroup(ActorId actorId, GroupId groupId);

    /**
     * @brief Remove actor from group
     * @param actorId Actor ID
     * @return True if successful
     */
    bool removeActorFromGroup(ActorId actorId);

    /**
     * @brief Get actors in group
     * @param groupId Group ID
     * @return Vector of actor IDs in group
     */
    std::vector<ActorId> getActorsInGroup(GroupId groupId) const;

    /**
     * @brief Set visibility for all actors in group
     * @param groupId Group ID
     * @param visible Visibility flag
     * @return Number of actors affected
     */
    size_t setGroupVisibility(GroupId groupId, bool visible);

    /**
     * @brief Set properties for all actors in group
     * @param groupId Group ID
     * @param properties Properties to set
     * @return Number of actors affected
     */
    size_t setGroupProperties(GroupId groupId, const ActorProperties& properties);

    // ========================================================================
    // Highlighting (Selection)
    // ========================================================================

    /**
     * @brief Set highlight style
     * @param style Highlight style
     */
    void setHighlightStyle(const HighlightStyle& style);

    /**
     * @brief Get highlight style
     * @return Current highlight style
     */
    const HighlightStyle& getHighlightStyle() const;

    /**
     * @brief Highlight actor
     * @param id Actor ID
     * @return True if successful
     */
    bool highlightActor(ActorId id);

    /**
     * @brief Unhighlight actor
     * @param id Actor ID
     * @return True if successful
     */
    bool unhighlightActor(ActorId id);

    /**
     * @brief Clear all highlights
     */
    void clearHighlights();

    /**
     * @brief Get highlighted actors
     * @return Set of highlighted actor IDs
     */
    std::set<ActorId> getHighlightedActors() const;

    // ========================================================================
    // Batch Operations
    // ========================================================================

    /**
     * @brief Show all actors
     */
    void showAll();

    /**
     * @brief Hide all actors
     */
    void hideAll();

    /**
     * @brief Reset all actors to default properties
     */
    void resetAll();

    /**
     * @brief Apply properties to all actors
     * @param properties Properties to apply
     */
    void applyToAll(const ActorProperties& properties);

    // ========================================================================
    // Name Management
    // ========================================================================

    /**
     * @brief Set actor name
     * @param id Actor ID
     * @param name Actor name
     * @return True if successful
     */
    bool setActorName(ActorId id, const std::string& name);

    /**
     * @brief Get actor name
     * @param id Actor ID
     * @return Actor name, or empty string if not found
     */
    std::string getActorName(ActorId id) const;

    /**
     * @brief Find actor by name
     * @param name Actor name
     * @return Actor ID, or 0 if not found
     */
    ActorId findActorByName(const std::string& name) const;

private:
    /**
     * @brief Apply properties to VTK actor
     */
    void applyPropertiesToVTKActor(vtkActor* actor, const ActorProperties& properties);

    /**
     * @brief Read properties from VTK actor
     */
    ActorProperties readPropertiesFromVTKActor(vtkActor* actor) const;

    std::map<ActorId, ActorInfo> m_actors;
    std::map<GroupId, std::set<ActorId>> m_groups;
    ActorId m_nextActorId;
    GroupId m_nextGroupId;
    HighlightStyle m_highlightStyle;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class ActorManager {
public:
    ActorManager() {}
    ~ActorManager() {}

    ActorId addActor(void*, const std::string& = "", const ActorProperties& = ActorProperties()) { return 0; }
    bool removeActor(ActorId) { return false; }
    void removeAllActors() {}
    bool hasActor(ActorId) const { return false; }
    size_t getActorCount() const { return 0; }
    void* getActor(ActorId) const { return nullptr; }
    std::vector<ActorId> getAllActorIds() const { return {}; }

    bool setActorProperties(ActorId, const ActorProperties&) { return false; }
    bool getActorProperties(ActorId, ActorProperties&) const { return false; }
    bool setActorColor(ActorId, double, double, double) { return false; }
    bool setActorOpacity(ActorId, double) { return false; }
    bool setActorVisibility(ActorId, bool) { return false; }
    bool setActorRepresentation(ActorId, int) { return false; }

    GroupId createGroup() { return 0; }
    bool addActorToGroup(ActorId, GroupId) { return false; }
    bool removeActorFromGroup(ActorId) { return false; }
    std::vector<ActorId> getActorsInGroup(GroupId) const { return {}; }
    size_t setGroupVisibility(GroupId, bool) { return 0; }
    size_t setGroupProperties(GroupId, const ActorProperties&) { return 0; }

    void setHighlightStyle(const HighlightStyle&) {}
    HighlightStyle getHighlightStyle() const { return HighlightStyle(); }
    bool highlightActor(ActorId) { return false; }
    bool unhighlightActor(ActorId) { return false; }
    void clearHighlights() {}
    std::set<ActorId> getHighlightedActors() const { return {}; }

    void showAll() {}
    void hideAll() {}
    void resetAll() {}
    void applyToAll(const ActorProperties&) {}

    bool setActorName(ActorId, const std::string&) { return false; }
    std::string getActorName(ActorId) const { return ""; }
    ActorId findActorByName(const std::string&) const { return 0; }
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
