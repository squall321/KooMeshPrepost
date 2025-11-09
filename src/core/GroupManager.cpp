#include "core/GroupManager.h"
#include <algorithm>

namespace koomesh {
namespace core {

// ======================================================================
// 그룹 관리
// ======================================================================

bool GroupManager::createGroup(const std::string& name) {
    if (name.empty() || hasGroup(name)) {
        return false;
    }

    m_groups[name] = std::make_unique<Group>(name);
    notifyGroupCreated(name);
    return true;
}

bool GroupManager::deleteGroup(const std::string& name) {
    auto it = m_groups.find(name);
    if (it == m_groups.end()) {
        return false;
    }

    m_groups.erase(it);
    notifyGroupDeleted(name);
    return true;
}

Group* GroupManager::getGroup(const std::string& name) {
    auto it = m_groups.find(name);
    return (it != m_groups.end()) ? it->second.get() : nullptr;
}

const Group* GroupManager::getGroup(const std::string& name) const {
    auto it = m_groups.find(name);
    return (it != m_groups.end()) ? it->second.get() : nullptr;
}

bool GroupManager::hasGroup(const std::string& name) const {
    return m_groups.find(name) != m_groups.end();
}

bool GroupManager::renameGroup(const std::string& oldName, const std::string& newName) {
    if (oldName == newName) {
        return true;  // 이름이 같으면 성공으로 간주
    }

    if (newName.empty() || !hasGroup(oldName) || hasGroup(newName)) {
        return false;
    }

    // 그룹 이동
    auto it = m_groups.find(oldName);
    auto group = std::move(it->second);
    m_groups.erase(it);

    group->setName(newName);
    m_groups[newName] = std::move(group);

    notifyGroupRenamed(oldName, newName);
    return true;
}

std::vector<std::string> GroupManager::getAllGroupNames() const {
    std::vector<std::string> names;
    names.reserve(m_groups.size());

    for (const auto& pair : m_groups) {
        names.push_back(pair.first);
    }

    // 정렬하여 일관된 순서 제공
    std::sort(names.begin(), names.end());
    return names;
}

void GroupManager::clearAll() {
    // 모든 그룹에 대해 삭제 알림
    for (const auto& pair : m_groups) {
        notifyGroupDeleted(pair.first);
    }

    m_groups.clear();
}

// ======================================================================
// Observer 관리
// ======================================================================

void GroupManager::addObserver(IGroupObserver* observer) {
    if (observer == nullptr) {
        return;
    }

    // 중복 방지
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it == m_observers.end()) {
        m_observers.push_back(observer);
    }
}

void GroupManager::removeObserver(IGroupObserver* observer) {
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != m_observers.end()) {
        m_observers.erase(it);
    }
}

void GroupManager::notifyGroupModified(const std::string& groupName) {
    if (!hasGroup(groupName)) {
        return;
    }

    for (IGroupObserver* observer : m_observers) {
        observer->onGroupModified(groupName);
    }
}

// ======================================================================
// 내부 알림 메서드
// ======================================================================

void GroupManager::notifyGroupCreated(const std::string& groupName) {
    for (IGroupObserver* observer : m_observers) {
        observer->onGroupCreated(groupName);
    }
}

void GroupManager::notifyGroupDeleted(const std::string& groupName) {
    for (IGroupObserver* observer : m_observers) {
        observer->onGroupDeleted(groupName);
    }
}

void GroupManager::notifyGroupRenamed(const std::string& oldName, const std::string& newName) {
    for (IGroupObserver* observer : m_observers) {
        observer->onGroupRenamed(oldName, newName);
    }
}

} // namespace core
} // namespace koomesh
