#pragma once

#include "Group.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace koomesh {
namespace core {

/**
 * @brief IGroupObserver 인터페이스
 *
 * Observer 패턴: 그룹 변경 시 알림을 받는 인터페이스
 */
class IGroupObserver {
public:
    virtual ~IGroupObserver() = default;

    /**
     * @brief 그룹 생성 알림
     */
    virtual void onGroupCreated(const std::string& groupName) = 0;

    /**
     * @brief 그룹 삭제 알림
     */
    virtual void onGroupDeleted(const std::string& groupName) = 0;

    /**
     * @brief 그룹 수정 알림
     */
    virtual void onGroupModified(const std::string& groupName) = 0;

    /**
     * @brief 그룹 이름 변경 알림
     */
    virtual void onGroupRenamed(const std::string& oldName, const std::string& newName) = 0;
};

/**
 * @brief GroupManager 클래스
 *
 * 여러 그룹을 중앙에서 관리하며, Observer 패턴을 통해 변경사항을 알립니다.
 */
class GroupManager {
public:
    GroupManager() = default;
    ~GroupManager() = default;

    // 복사/이동 방지
    GroupManager(const GroupManager&) = delete;
    GroupManager& operator=(const GroupManager&) = delete;

    /**
     * @brief 그룹 생성
     */
    bool createGroup(const std::string& name);

    /**
     * @brief 그룹 삭제
     */
    bool deleteGroup(const std::string& name);

    /**
     * @brief 그룹 조회
     */
    Group* getGroup(const std::string& name);
    const Group* getGroup(const std::string& name) const;

    /**
     * @brief 그룹 존재 여부
     */
    bool hasGroup(const std::string& name) const;

    /**
     * @brief 그룹 이름 변경
     */
    bool renameGroup(const std::string& oldName, const std::string& newName);

    /**
     * @brief 모든 그룹 이름 조회
     */
    std::vector<std::string> getAllGroupNames() const;

    /**
     * @brief 그룹 개수
     */
    size_t groupCount() const { return m_groups.size(); }

    /**
     * @brief 모든 그룹 제거
     */
    void clearAll();

    /**
     * @brief Observer 등록
     */
    void addObserver(IGroupObserver* observer);

    /**
     * @brief Observer 해제
     */
    void removeObserver(IGroupObserver* observer);

    /**
     * @brief 그룹 수정 알림 (외부에서 그룹을 직접 수정한 경우 호출)
     */
    void notifyGroupModified(const std::string& groupName);

private:
    void notifyGroupCreated(const std::string& groupName);
    void notifyGroupDeleted(const std::string& groupName);
    void notifyGroupRenamed(const std::string& oldName, const std::string& newName);

    std::unordered_map<std::string, std::unique_ptr<Group>> m_groups;
    std::vector<IGroupObserver*> m_observers;
};

} // namespace core
} // namespace koomesh
