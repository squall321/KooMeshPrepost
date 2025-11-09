/**
 * @file ActorManager.cpp
 * @brief Implementation of ActorManager
 */

#include "visualization/ActorManager.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkProperty.h>
#include <algorithm>
#endif

namespace koomesh {
namespace visualization {

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// Constructor / Destructor
// ============================================================================

ActorManager::ActorManager()
    : m_nextActorId(1)
    , m_nextGroupId(1)
{
}

ActorManager::~ActorManager() {
    removeAllActors();
}

// ============================================================================
// Actor Registration
// ============================================================================

ActorId ActorManager::addActor(
    vtkSmartPointer<vtkActor> actor,
    const std::string& name,
    const ActorProperties& properties
) {
    if (!actor) {
        return 0;
    }

    ActorId id = m_nextActorId++;

    ActorInfo info;
    info.actor = actor;
    info.properties = properties;
    info.originalProperties = properties;
    info.name = name;
    info.group = 0;
    info.isHighlighted = false;

    m_actors[id] = info;

    // Apply properties to VTK actor
    applyPropertiesToVTKActor(actor, properties);

    return id;
}

bool ActorManager::removeActor(ActorId id) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    // Remove from group if in one
    removeActorFromGroup(id);

    m_actors.erase(it);
    return true;
}

void ActorManager::removeAllActors() {
    m_actors.clear();
    m_groups.clear();
}

bool ActorManager::hasActor(ActorId id) const {
    return m_actors.find(id) != m_actors.end();
}

size_t ActorManager::getActorCount() const {
    return m_actors.size();
}

vtkActor* ActorManager::getActor(ActorId id) const {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return nullptr;
    }
    return it->second.actor;
}

std::vector<ActorId> ActorManager::getAllActorIds() const {
    std::vector<ActorId> ids;
    ids.reserve(m_actors.size());
    for (const auto& pair : m_actors) {
        ids.push_back(pair.first);
    }
    return ids;
}

// ============================================================================
// Property Management
// ============================================================================

bool ActorManager::setActorProperties(ActorId id, const ActorProperties& properties) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.properties = properties;

    // Don't apply if highlighted (will be applied when unhighlighted)
    if (!it->second.isHighlighted) {
        applyPropertiesToVTKActor(it->second.actor, properties);
    }

    return true;
}

bool ActorManager::getActorProperties(ActorId id, ActorProperties& properties) const {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    properties = it->second.properties;
    return true;
}

bool ActorManager::setActorColor(ActorId id, double r, double g, double b) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.properties.color[0] = r;
    it->second.properties.color[1] = g;
    it->second.properties.color[2] = b;

    if (!it->second.isHighlighted) {
        it->second.actor->GetProperty()->SetColor(r, g, b);
    }

    return true;
}

bool ActorManager::setActorOpacity(ActorId id, double opacity) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.properties.opacity = opacity;

    if (!it->second.isHighlighted) {
        it->second.actor->GetProperty()->SetOpacity(opacity);
    }

    return true;
}

bool ActorManager::setActorVisibility(ActorId id, bool visible) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.properties.visible = visible;
    it->second.actor->SetVisibility(visible ? 1 : 0);

    return true;
}

bool ActorManager::setActorRepresentation(ActorId id, int representation) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.properties.representation = representation;

    if (!it->second.isHighlighted) {
        it->second.actor->GetProperty()->SetRepresentation(representation);
    }

    return true;
}

// ============================================================================
// Group Management
// ============================================================================

GroupId ActorManager::createGroup() {
    GroupId id = m_nextGroupId++;
    m_groups[id] = std::set<ActorId>();
    return id;
}

bool ActorManager::addActorToGroup(ActorId actorId, GroupId groupId) {
    // Check if actor exists
    if (!hasActor(actorId)) {
        return false;
    }

    // Create group if it doesn't exist
    if (m_groups.find(groupId) == m_groups.end()) {
        m_groups[groupId] = std::set<ActorId>();
    }

    // Remove from current group
    removeActorFromGroup(actorId);

    // Add to new group
    m_groups[groupId].insert(actorId);
    m_actors[actorId].group = groupId;

    return true;
}

bool ActorManager::removeActorFromGroup(ActorId actorId) {
    auto it = m_actors.find(actorId);
    if (it == m_actors.end()) {
        return false;
    }

    GroupId groupId = it->second.group;
    if (groupId == 0) {
        return false;  // Not in a group
    }

    auto groupIt = m_groups.find(groupId);
    if (groupIt != m_groups.end()) {
        groupIt->second.erase(actorId);
    }

    it->second.group = 0;
    return true;
}

std::vector<ActorId> ActorManager::getActorsInGroup(GroupId groupId) const {
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return {};
    }

    return std::vector<ActorId>(it->second.begin(), it->second.end());
}

size_t ActorManager::setGroupVisibility(GroupId groupId, bool visible) {
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return 0;
    }

    size_t count = 0;
    for (ActorId actorId : it->second) {
        if (setActorVisibility(actorId, visible)) {
            ++count;
        }
    }

    return count;
}

size_t ActorManager::setGroupProperties(GroupId groupId, const ActorProperties& properties) {
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return 0;
    }

    size_t count = 0;
    for (ActorId actorId : it->second) {
        if (setActorProperties(actorId, properties)) {
            ++count;
        }
    }

    return count;
}

// ============================================================================
// Highlighting (Selection)
// ============================================================================

void ActorManager::setHighlightStyle(const HighlightStyle& style) {
    m_highlightStyle = style;
}

const HighlightStyle& ActorManager::getHighlightStyle() const {
    return m_highlightStyle;
}

bool ActorManager::highlightActor(ActorId id) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    if (it->second.isHighlighted) {
        return true;  // Already highlighted
    }

    // Save current properties
    it->second.originalProperties = it->second.properties;

    // Apply highlight style
    ActorProperties highlightProps;
    highlightProps.color[0] = m_highlightStyle.color[0];
    highlightProps.color[1] = m_highlightStyle.color[1];
    highlightProps.color[2] = m_highlightStyle.color[2];
    highlightProps.opacity = m_highlightStyle.opacity;
    highlightProps.lineWidth = m_highlightStyle.lineWidth;
    highlightProps.edgeVisibility = m_highlightStyle.edgeVisibility;
    highlightProps.edgeColor[0] = m_highlightStyle.edgeColor[0];
    highlightProps.edgeColor[1] = m_highlightStyle.edgeColor[1];
    highlightProps.edgeColor[2] = m_highlightStyle.edgeColor[2];
    highlightProps.representation = it->second.properties.representation;
    highlightProps.visible = it->second.properties.visible;

    applyPropertiesToVTKActor(it->second.actor, highlightProps);

    it->second.isHighlighted = true;

    return true;
}

bool ActorManager::unhighlightActor(ActorId id) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    if (!it->second.isHighlighted) {
        return true;  // Already not highlighted
    }

    // Restore original properties
    applyPropertiesToVTKActor(it->second.actor, it->second.originalProperties);

    it->second.isHighlighted = false;

    return true;
}

void ActorManager::clearHighlights() {
    for (auto& pair : m_actors) {
        if (pair.second.isHighlighted) {
            unhighlightActor(pair.first);
        }
    }
}

std::set<ActorId> ActorManager::getHighlightedActors() const {
    std::set<ActorId> highlighted;
    for (const auto& pair : m_actors) {
        if (pair.second.isHighlighted) {
            highlighted.insert(pair.first);
        }
    }
    return highlighted;
}

// ============================================================================
// Batch Operations
// ============================================================================

void ActorManager::showAll() {
    for (auto& pair : m_actors) {
        setActorVisibility(pair.first, true);
    }
}

void ActorManager::hideAll() {
    for (auto& pair : m_actors) {
        setActorVisibility(pair.first, false);
    }
}

void ActorManager::resetAll() {
    clearHighlights();

    ActorProperties defaultProps;
    for (auto& pair : m_actors) {
        setActorProperties(pair.first, defaultProps);
    }
}

void ActorManager::applyToAll(const ActorProperties& properties) {
    for (auto& pair : m_actors) {
        setActorProperties(pair.first, properties);
    }
}

// ============================================================================
// Name Management
// ============================================================================

bool ActorManager::setActorName(ActorId id, const std::string& name) {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return false;
    }

    it->second.name = name;
    return true;
}

std::string ActorManager::getActorName(ActorId id) const {
    auto it = m_actors.find(id);
    if (it == m_actors.end()) {
        return "";
    }

    return it->second.name;
}

ActorId ActorManager::findActorByName(const std::string& name) const {
    for (const auto& pair : m_actors) {
        if (pair.second.name == name) {
            return pair.first;
        }
    }
    return 0;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void ActorManager::applyPropertiesToVTKActor(vtkActor* actor, const ActorProperties& properties) {
    if (!actor) {
        return;
    }

    vtkProperty* prop = actor->GetProperty();

    prop->SetColor(properties.color[0], properties.color[1], properties.color[2]);
    prop->SetOpacity(properties.opacity);
    prop->SetLineWidth(static_cast<float>(properties.lineWidth));
    prop->SetPointSize(static_cast<float>(properties.pointSize));
    prop->SetRepresentation(properties.representation);
    prop->SetLighting(properties.lighting ? 1 : 0);
    prop->SetSpecular(properties.specular);
    prop->SetSpecularPower(properties.specularPower);
    prop->SetAmbient(properties.ambient);
    prop->SetDiffuse(properties.diffuse);
    prop->SetEdgeVisibility(properties.edgeVisibility ? 1 : 0);
    prop->SetEdgeColor(properties.edgeColor[0], properties.edgeColor[1], properties.edgeColor[2]);

    actor->SetVisibility(properties.visible ? 1 : 0);
}

ActorProperties ActorManager::readPropertiesFromVTKActor(vtkActor* actor) const {
    ActorProperties properties;

    if (!actor) {
        return properties;
    }

    vtkProperty* prop = actor->GetProperty();

    prop->GetColor(properties.color);
    properties.opacity = prop->GetOpacity();
    properties.lineWidth = prop->GetLineWidth();
    properties.pointSize = prop->GetPointSize();
    properties.representation = prop->GetRepresentation();
    properties.lighting = (prop->GetLighting() != 0);
    properties.specular = prop->GetSpecular();
    properties.specularPower = prop->GetSpecularPower();
    properties.ambient = prop->GetAmbient();
    properties.diffuse = prop->GetDiffuse();
    properties.edgeVisibility = (prop->GetEdgeVisibility() != 0);
    prop->GetEdgeColor(properties.edgeColor);

    properties.visible = (actor->GetVisibility() != 0);

    return properties;
}

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
