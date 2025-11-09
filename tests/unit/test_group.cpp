#include <gtest/gtest.h>
#include "core/Group.h"
#include "core/GroupManager.h"
#include "helpers/TestUtils.h"

using namespace koomesh::core;

// ======================================================================
// Group Basic Tests
// ======================================================================

class GroupTest : public ::testing::Test {
protected:
    Group* group;

    void SetUp() override {
        group = new Group("TestGroup");
    }

    void TearDown() override {
        delete group;
    }
};

TEST_F(GroupTest, Construction) {
    EXPECT_EQ("TestGroup", group->name());
    EXPECT_EQ(0, group->elementCount());
    EXPECT_EQ(0, group->nodeCount());
    EXPECT_TRUE(group->isEmpty());
    EXPECT_TRUE(group->isVisible());
}

TEST_F(GroupTest, ConstructionWithoutName) {
    Group g;
    EXPECT_EQ("", g.name());
}

TEST_F(GroupTest, SetName) {
    group->setName("NewName");
    EXPECT_EQ("NewName", group->name());
}

// ======================================================================
// Element Management Tests
// ======================================================================

TEST_F(GroupTest, AddElement) {
    group->addElement(100);
    EXPECT_EQ(1, group->elementCount());
    EXPECT_TRUE(group->containsElement(100));
    EXPECT_FALSE(group->isEmpty());
}

TEST_F(GroupTest, AddMultipleElements) {
    group->addElement(100);
    group->addElement(200);
    group->addElement(300);

    EXPECT_EQ(3, group->elementCount());
    EXPECT_TRUE(group->containsElement(100));
    EXPECT_TRUE(group->containsElement(200));
    EXPECT_TRUE(group->containsElement(300));
}

TEST_F(GroupTest, AddDuplicateElement) {
    group->addElement(100);
    group->addElement(100);

    EXPECT_EQ(1, group->elementCount());
}

TEST_F(GroupTest, RemoveElement) {
    group->addElement(100);
    group->addElement(200);

    bool removed = group->removeElement(100);
    EXPECT_TRUE(removed);
    EXPECT_EQ(1, group->elementCount());
    EXPECT_FALSE(group->containsElement(100));
}

TEST_F(GroupTest, RemoveNonExistentElement) {
    bool removed = group->removeElement(999);
    EXPECT_FALSE(removed);
}

// ======================================================================
// Node Management Tests
// ======================================================================

TEST_F(GroupTest, AddNode) {
    group->addNode(1);
    EXPECT_EQ(1, group->nodeCount());
    EXPECT_TRUE(group->containsNode(1));
    EXPECT_FALSE(group->isEmpty());
}

TEST_F(GroupTest, AddMultipleNodes) {
    group->addNode(1);
    group->addNode(2);
    group->addNode(3);

    EXPECT_EQ(3, group->nodeCount());
    EXPECT_TRUE(group->containsNode(1));
    EXPECT_TRUE(group->containsNode(2));
    EXPECT_TRUE(group->containsNode(3));
}

TEST_F(GroupTest, RemoveNode) {
    group->addNode(1);
    group->addNode(2);

    bool removed = group->removeNode(1);
    EXPECT_TRUE(removed);
    EXPECT_EQ(1, group->nodeCount());
    EXPECT_FALSE(group->containsNode(1));
}

// ======================================================================
// Clear Tests
// ======================================================================

TEST_F(GroupTest, Clear) {
    group->addElement(100);
    group->addElement(200);
    group->addNode(1);
    group->addNode(2);

    group->clear();

    EXPECT_EQ(0, group->elementCount());
    EXPECT_EQ(0, group->nodeCount());
    EXPECT_TRUE(group->isEmpty());
}

TEST_F(GroupTest, ClearElements) {
    group->addElement(100);
    group->addNode(1);

    group->clearElements();

    EXPECT_EQ(0, group->elementCount());
    EXPECT_EQ(1, group->nodeCount());
}

TEST_F(GroupTest, ClearNodes) {
    group->addElement(100);
    group->addNode(1);

    group->clearNodes();

    EXPECT_EQ(1, group->elementCount());
    EXPECT_EQ(0, group->nodeCount());
}

// ======================================================================
// Color and Visibility Tests
// ======================================================================

TEST_F(GroupTest, SetColor) {
    auto red = Part::Color::Red();
    group->setColor(red);

    const auto& c = group->color();
    EXPECT_FLOAT_EQ(1.0f, c.r);
    EXPECT_FLOAT_EQ(0.0f, c.g);
    EXPECT_FLOAT_EQ(0.0f, c.b);
}

TEST_F(GroupTest, SetVisibility) {
    EXPECT_TRUE(group->isVisible());

    group->setVisible(false);
    EXPECT_FALSE(group->isVisible());

    group->setVisible(true);
    EXPECT_TRUE(group->isVisible());
}

// ======================================================================
// Set Operations Tests
// ======================================================================

class GroupSetOperationsTest : public ::testing::Test {
protected:
    Group group1{"Group1"};
    Group group2{"Group2"};

    void SetUp() override {
        // Group1: elements {100, 200, 300}, nodes {1, 2, 3}
        group1.addElement(100);
        group1.addElement(200);
        group1.addElement(300);
        group1.addNode(1);
        group1.addNode(2);
        group1.addNode(3);

        // Group2: elements {200, 300, 400}, nodes {2, 3, 4}
        group2.addElement(200);
        group2.addElement(300);
        group2.addElement(400);
        group2.addNode(2);
        group2.addNode(3);
        group2.addNode(4);
    }
};

TEST_F(GroupSetOperationsTest, Merge) {
    group1.merge(group2);

    // Union: {100, 200, 300, 400}
    EXPECT_EQ(4, group1.elementCount());
    EXPECT_TRUE(group1.containsElement(100));
    EXPECT_TRUE(group1.containsElement(200));
    EXPECT_TRUE(group1.containsElement(300));
    EXPECT_TRUE(group1.containsElement(400));

    // Union: {1, 2, 3, 4}
    EXPECT_EQ(4, group1.nodeCount());
    EXPECT_TRUE(group1.containsNode(1));
    EXPECT_TRUE(group1.containsNode(2));
    EXPECT_TRUE(group1.containsNode(3));
    EXPECT_TRUE(group1.containsNode(4));
}

TEST_F(GroupSetOperationsTest, Intersect) {
    group1.intersect(group2);

    // Intersection: {200, 300}
    EXPECT_EQ(2, group1.elementCount());
    EXPECT_FALSE(group1.containsElement(100));
    EXPECT_TRUE(group1.containsElement(200));
    EXPECT_TRUE(group1.containsElement(300));
    EXPECT_FALSE(group1.containsElement(400));

    // Intersection: {2, 3}
    EXPECT_EQ(2, group1.nodeCount());
    EXPECT_FALSE(group1.containsNode(1));
    EXPECT_TRUE(group1.containsNode(2));
    EXPECT_TRUE(group1.containsNode(3));
    EXPECT_FALSE(group1.containsNode(4));
}

TEST_F(GroupSetOperationsTest, Subtract) {
    group1.subtract(group2);

    // Difference: {100}
    EXPECT_EQ(1, group1.elementCount());
    EXPECT_TRUE(group1.containsElement(100));
    EXPECT_FALSE(group1.containsElement(200));
    EXPECT_FALSE(group1.containsElement(300));

    // Difference: {1}
    EXPECT_EQ(1, group1.nodeCount());
    EXPECT_TRUE(group1.containsNode(1));
    EXPECT_FALSE(group1.containsNode(2));
    EXPECT_FALSE(group1.containsNode(3));
}

TEST_F(GroupSetOperationsTest, EmptyIntersection) {
    Group group3("Group3");
    group3.addElement(500);
    group3.addElement(600);

    group1.intersect(group3);

    EXPECT_EQ(0, group1.elementCount());
    EXPECT_TRUE(group1.isEmpty());
}

// ======================================================================
// Comparison Tests
// ======================================================================

TEST(GroupComparisonTest, EqualityByName) {
    Group g1("GroupA");
    Group g2("GroupA");
    Group g3("GroupB");

    EXPECT_TRUE(g1 == g2);
    EXPECT_FALSE(g1 == g3);
}

TEST(GroupComparisonTest, Inequality) {
    Group g1("GroupA");
    Group g2("GroupB");

    EXPECT_TRUE(g1 != g2);
    EXPECT_FALSE(g1 != Group("GroupA"));
}

// ======================================================================
// GroupManager Basic Tests
// ======================================================================

class GroupManagerTest : public ::testing::Test {
protected:
    GroupManager manager;
};

TEST_F(GroupManagerTest, CreateGroup) {
    bool created = manager.createGroup("Group1");
    EXPECT_TRUE(created);
    EXPECT_EQ(1, manager.groupCount());
    EXPECT_TRUE(manager.hasGroup("Group1"));
}

TEST_F(GroupManagerTest, CreateDuplicateGroup) {
    manager.createGroup("Group1");
    bool created = manager.createGroup("Group1");

    EXPECT_FALSE(created);
    EXPECT_EQ(1, manager.groupCount());
}

TEST_F(GroupManagerTest, CreateEmptyName) {
    bool created = manager.createGroup("");
    EXPECT_FALSE(created);
}

TEST_F(GroupManagerTest, GetGroup) {
    manager.createGroup("Group1");

    Group* group = manager.getGroup("Group1");
    ASSERT_NE(nullptr, group);
    EXPECT_EQ("Group1", group->name());
}

TEST_F(GroupManagerTest, GetNonExistentGroup) {
    Group* group = manager.getGroup("NonExistent");
    EXPECT_EQ(nullptr, group);
}

TEST_F(GroupManagerTest, DeleteGroup) {
    manager.createGroup("Group1");

    bool deleted = manager.deleteGroup("Group1");
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(manager.hasGroup("Group1"));
    EXPECT_EQ(0, manager.groupCount());
}

TEST_F(GroupManagerTest, DeleteNonExistentGroup) {
    bool deleted = manager.deleteGroup("NonExistent");
    EXPECT_FALSE(deleted);
}

TEST_F(GroupManagerTest, RenameGroup) {
    manager.createGroup("OldName");

    bool renamed = manager.renameGroup("OldName", "NewName");
    EXPECT_TRUE(renamed);
    EXPECT_FALSE(manager.hasGroup("OldName"));
    EXPECT_TRUE(manager.hasGroup("NewName"));
}

TEST_F(GroupManagerTest, RenameToSameName) {
    manager.createGroup("Group1");

    bool renamed = manager.renameGroup("Group1", "Group1");
    EXPECT_TRUE(renamed);  // Should succeed
}

TEST_F(GroupManagerTest, RenameToExistingName) {
    manager.createGroup("Group1");
    manager.createGroup("Group2");

    bool renamed = manager.renameGroup("Group1", "Group2");
    EXPECT_FALSE(renamed);
    EXPECT_TRUE(manager.hasGroup("Group1"));
}

TEST_F(GroupManagerTest, RenameNonExistentGroup) {
    bool renamed = manager.renameGroup("NonExistent", "NewName");
    EXPECT_FALSE(renamed);
}

TEST_F(GroupManagerTest, GetAllGroupNames) {
    manager.createGroup("Group3");
    manager.createGroup("Group1");
    manager.createGroup("Group2");

    auto names = manager.getAllGroupNames();
    EXPECT_EQ(3, names.size());

    // Should be sorted
    EXPECT_EQ("Group1", names[0]);
    EXPECT_EQ("Group2", names[1]);
    EXPECT_EQ("Group3", names[2]);
}

TEST_F(GroupManagerTest, ClearAll) {
    manager.createGroup("Group1");
    manager.createGroup("Group2");
    manager.createGroup("Group3");

    manager.clearAll();

    EXPECT_EQ(0, manager.groupCount());
    EXPECT_FALSE(manager.hasGroup("Group1"));
}

// ======================================================================
// Observer Pattern Tests
// ======================================================================

class MockGroupObserver : public IGroupObserver {
public:
    int createCount = 0;
    int deleteCount = 0;
    int modifyCount = 0;
    int renameCount = 0;

    std::string lastCreatedGroup;
    std::string lastDeletedGroup;
    std::string lastModifiedGroup;
    std::string lastOldName;
    std::string lastNewName;

    void onGroupCreated(const std::string& groupName) override {
        ++createCount;
        lastCreatedGroup = groupName;
    }

    void onGroupDeleted(const std::string& groupName) override {
        ++deleteCount;
        lastDeletedGroup = groupName;
    }

    void onGroupModified(const std::string& groupName) override {
        ++modifyCount;
        lastModifiedGroup = groupName;
    }

    void onGroupRenamed(const std::string& oldName, const std::string& newName) override {
        ++renameCount;
        lastOldName = oldName;
        lastNewName = newName;
    }
};

class GroupManagerObserverTest : public ::testing::Test {
protected:
    GroupManager manager;
    MockGroupObserver observer;

    void SetUp() override {
        manager.addObserver(&observer);
    }
};

TEST_F(GroupManagerObserverTest, CreateNotification) {
    manager.createGroup("Group1");

    EXPECT_EQ(1, observer.createCount);
    EXPECT_EQ("Group1", observer.lastCreatedGroup);
}

TEST_F(GroupManagerObserverTest, DeleteNotification) {
    manager.createGroup("Group1");
    manager.deleteGroup("Group1");

    EXPECT_EQ(1, observer.deleteCount);
    EXPECT_EQ("Group1", observer.lastDeletedGroup);
}

TEST_F(GroupManagerObserverTest, RenameNotification) {
    manager.createGroup("OldName");
    manager.renameGroup("OldName", "NewName");

    EXPECT_EQ(1, observer.renameCount);
    EXPECT_EQ("OldName", observer.lastOldName);
    EXPECT_EQ("NewName", observer.lastNewName);
}

TEST_F(GroupManagerObserverTest, ModifyNotification) {
    manager.createGroup("Group1");
    manager.notifyGroupModified("Group1");

    EXPECT_EQ(1, observer.modifyCount);
    EXPECT_EQ("Group1", observer.lastModifiedGroup);
}

TEST_F(GroupManagerObserverTest, MultipleObservers) {
    MockGroupObserver observer2;
    manager.addObserver(&observer2);

    manager.createGroup("Group1");

    EXPECT_EQ(1, observer.createCount);
    EXPECT_EQ(1, observer2.createCount);
}

TEST_F(GroupManagerObserverTest, RemoveObserver) {
    manager.removeObserver(&observer);
    manager.createGroup("Group1");

    EXPECT_EQ(0, observer.createCount);
}

TEST_F(GroupManagerObserverTest, ClearAllNotifications) {
    manager.createGroup("Group1");
    manager.createGroup("Group2");
    manager.clearAll();

    EXPECT_EQ(2, observer.deleteCount);  // Both groups deleted
}

// ======================================================================
// Integration Tests
// ======================================================================

class GroupIntegrationTest : public ::testing::Test {
protected:
    GroupManager manager;
};

TEST_F(GroupIntegrationTest, CompleteWorkflow) {
    // Create groups
    manager.createGroup("Chassis");
    manager.createGroup("Engine");
    manager.createGroup("Wheels");

    // Get and populate groups
    Group* chassis = manager.getGroup("Chassis");
    ASSERT_NE(nullptr, chassis);

    for (ElementId i = 1; i <= 1000; ++i) {
        chassis->addElement(i);
    }

    chassis->setColor(Part::Color::Blue());

    // Verify
    EXPECT_EQ(1000, chassis->elementCount());
    EXPECT_FLOAT_EQ(0.0f, chassis->color().r);
    EXPECT_FLOAT_EQ(0.0f, chassis->color().g);
    EXPECT_FLOAT_EQ(1.0f, chassis->color().b);
}

TEST_F(GroupIntegrationTest, GroupSelectionFiltering) {
    manager.createGroup("HighStress");
    manager.createGroup("LowStress");

    Group* highStress = manager.getGroup("HighStress");
    Group* lowStress = manager.getGroup("LowStress");

    // Simulate stress analysis results
    for (ElementId i = 1; i <= 100; ++i) {
        if (i % 3 == 0) {
            highStress->addElement(i);
        } else {
            lowStress->addElement(i);
        }
    }

    EXPECT_GT(lowStress->elementCount(), highStress->elementCount());
    EXPECT_EQ(100, highStress->elementCount() + lowStress->elementCount());
}

TEST_F(GroupIntegrationTest, GroupHierarchySimulation) {
    manager.createGroup("AllElements");
    manager.createGroup("Selected");

    Group* all = manager.getGroup("AllElements");
    Group* selected = manager.getGroup("Selected");

    // All elements
    for (ElementId i = 1; i <= 1000; ++i) {
        all->addElement(i);
    }

    // Selected subset
    for (ElementId i = 100; i <= 200; ++i) {
        selected->addElement(i);
    }

    // Create "Unselected" via set operations
    manager.createGroup("Unselected");
    Group* unselected = manager.getGroup("Unselected");

    unselected->merge(*all);
    unselected->subtract(*selected);

    EXPECT_EQ(899, unselected->elementCount());
    EXPECT_FALSE(unselected->containsElement(150));
    EXPECT_TRUE(unselected->containsElement(50));
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
