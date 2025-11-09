/**
 * @file test_actor_manager.cpp
 * @brief Unit tests for ActorManager
 */

#include <gtest/gtest.h>
#include "visualization/ActorManager.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#endif

using namespace koomesh::visualization;

/**
 * @brief Test fixture for ActorManager tests
 */
class ActorManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<ActorManager>();
    }

    std::unique_ptr<ActorManager> manager;

#ifdef KOOMESH_HAS_VTK
    vtkSmartPointer<vtkActor> createTestActor() {
        auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(cubeSource->GetOutputPort());

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);

        return actor;
    }
#endif
};

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// VTK Available Tests
// ============================================================================

TEST_F(ActorManagerTest, Initialization) {
    EXPECT_EQ(manager->getActorCount(), 0);
    EXPECT_TRUE(manager->getAllActorIds().empty());
}

TEST_F(ActorManagerTest, AddActor) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor, "TestActor");

    EXPECT_NE(id, 0);
    EXPECT_EQ(manager->getActorCount(), 1);
    EXPECT_TRUE(manager->hasActor(id));
}

TEST_F(ActorManagerTest, AddNullActor) {
    ActorId id = manager->addActor(nullptr);
    EXPECT_EQ(id, 0);
    EXPECT_EQ(manager->getActorCount(), 0);
}

TEST_F(ActorManagerTest, AddMultipleActors) {
    ActorId id1 = manager->addActor(createTestActor(), "Actor1");
    ActorId id2 = manager->addActor(createTestActor(), "Actor2");
    ActorId id3 = manager->addActor(createTestActor(), "Actor3");

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
    EXPECT_EQ(manager->getActorCount(), 3);
}

TEST_F(ActorManagerTest, RemoveActor) {
    ActorId id = manager->addActor(createTestActor());

    bool removed = manager->removeActor(id);
    EXPECT_TRUE(removed);
    EXPECT_EQ(manager->getActorCount(), 0);
    EXPECT_FALSE(manager->hasActor(id));
}

TEST_F(ActorManagerTest, RemoveNonexistentActor) {
    bool removed = manager->removeActor(999);
    EXPECT_FALSE(removed);
}

TEST_F(ActorManagerTest, RemoveAllActors) {
    manager->addActor(createTestActor());
    manager->addActor(createTestActor());
    manager->addActor(createTestActor());

    manager->removeAllActors();

    EXPECT_EQ(manager->getActorCount(), 0);
}

TEST_F(ActorManagerTest, GetActor) {
    auto originalActor = createTestActor();
    ActorId id = manager->addActor(originalActor);

    vtkActor* retrievedActor = manager->getActor(id);

    EXPECT_NE(retrievedActor, nullptr);
    EXPECT_EQ(retrievedActor, originalActor);
}

TEST_F(ActorManagerTest, GetNonexistentActor) {
    vtkActor* actor = manager->getActor(999);
    EXPECT_EQ(actor, nullptr);
}

TEST_F(ActorManagerTest, GetAllActorIds) {
    ActorId id1 = manager->addActor(createTestActor());
    ActorId id2 = manager->addActor(createTestActor());
    ActorId id3 = manager->addActor(createTestActor());

    auto ids = manager->getAllActorIds();

    EXPECT_EQ(ids.size(), 3);
    EXPECT_NE(std::find(ids.begin(), ids.end(), id1), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), id2), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), id3), ids.end());
}

// ============================================================================
// Property Management Tests
// ============================================================================

TEST_F(ActorManagerTest, SetActorColor) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    bool success = manager->setActorColor(id, 1.0, 0.0, 0.0);
    EXPECT_TRUE(success);

    double color[3];
    actor->GetProperty()->GetColor(color);
    EXPECT_DOUBLE_EQ(color[0], 1.0);
    EXPECT_DOUBLE_EQ(color[1], 0.0);
    EXPECT_DOUBLE_EQ(color[2], 0.0);
}

TEST_F(ActorManagerTest, SetActorOpacity) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    bool success = manager->setActorOpacity(id, 0.5);
    EXPECT_TRUE(success);

    EXPECT_DOUBLE_EQ(actor->GetProperty()->GetOpacity(), 0.5);
}

TEST_F(ActorManagerTest, SetActorVisibility) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    // Hide
    bool success = manager->setActorVisibility(id, false);
    EXPECT_TRUE(success);
    EXPECT_EQ(actor->GetVisibility(), 0);

    // Show
    success = manager->setActorVisibility(id, true);
    EXPECT_TRUE(success);
    EXPECT_EQ(actor->GetVisibility(), 1);
}

TEST_F(ActorManagerTest, SetActorRepresentation) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    // Wireframe
    bool success = manager->setActorRepresentation(id, 1);
    EXPECT_TRUE(success);
    EXPECT_EQ(actor->GetProperty()->GetRepresentation(), 1);

    // Surface
    success = manager->setActorRepresentation(id, 2);
    EXPECT_TRUE(success);
    EXPECT_EQ(actor->GetProperty()->GetRepresentation(), 2);
}

TEST_F(ActorManagerTest, SetGetActorProperties) {
    ActorId id = manager->addActor(createTestActor());

    ActorProperties props;
    props.color[0] = 0.5;
    props.color[1] = 0.6;
    props.color[2] = 0.7;
    props.opacity = 0.8;
    props.lineWidth = 2.0;
    props.representation = 1;

    bool success = manager->setActorProperties(id, props);
    EXPECT_TRUE(success);

    ActorProperties retrieved;
    success = manager->getActorProperties(id, retrieved);
    EXPECT_TRUE(success);

    EXPECT_DOUBLE_EQ(retrieved.color[0], 0.5);
    EXPECT_DOUBLE_EQ(retrieved.color[1], 0.6);
    EXPECT_DOUBLE_EQ(retrieved.color[2], 0.7);
    EXPECT_DOUBLE_EQ(retrieved.opacity, 0.8);
}

TEST_F(ActorManagerTest, SetPropertiesNonexistentActor) {
    ActorProperties props;
    bool success = manager->setActorProperties(999, props);
    EXPECT_FALSE(success);
}

// ============================================================================
// Group Management Tests
// ============================================================================

TEST_F(ActorManagerTest, CreateGroup) {
    GroupId id = manager->createGroup();
    EXPECT_NE(id, 0);
}

TEST_F(ActorManagerTest, AddActorToGroup) {
    GroupId groupId = manager->createGroup();
    ActorId actorId = manager->addActor(createTestActor());

    bool success = manager->addActorToGroup(actorId, groupId);
    EXPECT_TRUE(success);

    auto actorsInGroup = manager->getActorsInGroup(groupId);
    EXPECT_EQ(actorsInGroup.size(), 1);
    EXPECT_EQ(actorsInGroup[0], actorId);
}

TEST_F(ActorManagerTest, AddMultipleActorsToGroup) {
    GroupId groupId = manager->createGroup();
    ActorId id1 = manager->addActor(createTestActor());
    ActorId id2 = manager->addActor(createTestActor());
    ActorId id3 = manager->addActor(createTestActor());

    manager->addActorToGroup(id1, groupId);
    manager->addActorToGroup(id2, groupId);
    manager->addActorToGroup(id3, groupId);

    auto actorsInGroup = manager->getActorsInGroup(groupId);
    EXPECT_EQ(actorsInGroup.size(), 3);
}

TEST_F(ActorManagerTest, RemoveActorFromGroup) {
    GroupId groupId = manager->createGroup();
    ActorId actorId = manager->addActor(createTestActor());

    manager->addActorToGroup(actorId, groupId);

    bool success = manager->removeActorFromGroup(actorId);
    EXPECT_TRUE(success);

    auto actorsInGroup = manager->getActorsInGroup(groupId);
    EXPECT_TRUE(actorsInGroup.empty());
}

TEST_F(ActorManagerTest, SetGroupVisibility) {
    GroupId groupId = manager->createGroup();

    auto actor1 = createTestActor();
    auto actor2 = createTestActor();
    auto actor3 = createTestActor();

    ActorId id1 = manager->addActor(actor1);
    ActorId id2 = manager->addActor(actor2);
    ActorId id3 = manager->addActor(actor3);

    manager->addActorToGroup(id1, groupId);
    manager->addActorToGroup(id2, groupId);
    manager->addActorToGroup(id3, groupId);

    size_t count = manager->setGroupVisibility(groupId, false);
    EXPECT_EQ(count, 3);

    EXPECT_EQ(actor1->GetVisibility(), 0);
    EXPECT_EQ(actor2->GetVisibility(), 0);
    EXPECT_EQ(actor3->GetVisibility(), 0);
}

TEST_F(ActorManagerTest, SetGroupProperties) {
    GroupId groupId = manager->createGroup();

    auto actor1 = createTestActor();
    auto actor2 = createTestActor();

    ActorId id1 = manager->addActor(actor1);
    ActorId id2 = manager->addActor(actor2);

    manager->addActorToGroup(id1, groupId);
    manager->addActorToGroup(id2, groupId);

    ActorProperties props;
    props.color[0] = 1.0;
    props.color[1] = 0.0;
    props.color[2] = 0.0;
    props.opacity = 0.5;

    size_t count = manager->setGroupProperties(groupId, props);
    EXPECT_EQ(count, 2);

    double color1[3], color2[3];
    actor1->GetProperty()->GetColor(color1);
    actor2->GetProperty()->GetColor(color2);

    EXPECT_DOUBLE_EQ(color1[0], 1.0);
    EXPECT_DOUBLE_EQ(color2[0], 1.0);
    EXPECT_DOUBLE_EQ(actor1->GetProperty()->GetOpacity(), 0.5);
    EXPECT_DOUBLE_EQ(actor2->GetProperty()->GetOpacity(), 0.5);
}

// ============================================================================
// Highlighting Tests
// ============================================================================

TEST_F(ActorManagerTest, HighlightActor) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    // Set initial color
    manager->setActorColor(id, 0.5, 0.5, 0.5);

    // Highlight
    bool success = manager->highlightActor(id);
    EXPECT_TRUE(success);

    auto highlighted = manager->getHighlightedActors();
    EXPECT_EQ(highlighted.size(), 1);
    EXPECT_NE(highlighted.find(id), highlighted.end());

    // Check color changed to highlight color
    double color[3];
    actor->GetProperty()->GetColor(color);
    EXPECT_DOUBLE_EQ(color[0], 1.0);  // Default highlight color is yellow (1, 1, 0)
    EXPECT_DOUBLE_EQ(color[1], 1.0);
    EXPECT_DOUBLE_EQ(color[2], 0.0);
}

TEST_F(ActorManagerTest, UnhighlightActor) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    // Set initial color
    manager->setActorColor(id, 0.5, 0.5, 0.5);

    // Highlight and unhighlight
    manager->highlightActor(id);
    bool success = manager->unhighlightActor(id);
    EXPECT_TRUE(success);

    auto highlighted = manager->getHighlightedActors();
    EXPECT_TRUE(highlighted.empty());

    // Check color restored
    double color[3];
    actor->GetProperty()->GetColor(color);
    EXPECT_DOUBLE_EQ(color[0], 0.5);
    EXPECT_DOUBLE_EQ(color[1], 0.5);
    EXPECT_DOUBLE_EQ(color[2], 0.5);
}

TEST_F(ActorManagerTest, ClearHighlights) {
    ActorId id1 = manager->addActor(createTestActor());
    ActorId id2 = manager->addActor(createTestActor());
    ActorId id3 = manager->addActor(createTestActor());

    manager->highlightActor(id1);
    manager->highlightActor(id2);
    manager->highlightActor(id3);

    EXPECT_EQ(manager->getHighlightedActors().size(), 3);

    manager->clearHighlights();

    EXPECT_TRUE(manager->getHighlightedActors().empty());
}

TEST_F(ActorManagerTest, CustomHighlightStyle) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    // Set custom highlight style
    HighlightStyle style;
    style.color[0] = 1.0;
    style.color[1] = 0.0;
    style.color[2] = 1.0;  // Magenta
    style.lineWidth = 5.0;

    manager->setHighlightStyle(style);

    manager->highlightActor(id);

    double color[3];
    actor->GetProperty()->GetColor(color);
    EXPECT_DOUBLE_EQ(color[0], 1.0);
    EXPECT_DOUBLE_EQ(color[1], 0.0);
    EXPECT_DOUBLE_EQ(color[2], 1.0);
}

TEST_F(ActorManagerTest, HighlightPreservesVisibility) {
    auto actor = createTestActor();
    ActorId id = manager->addActor(actor);

    manager->setActorVisibility(id, false);
    manager->highlightActor(id);

    // Visibility should still be false
    EXPECT_EQ(actor->GetVisibility(), 0);

    manager->unhighlightActor(id);
    EXPECT_EQ(actor->GetVisibility(), 0);
}

// ============================================================================
// Batch Operations Tests
// ============================================================================

TEST_F(ActorManagerTest, ShowAll) {
    auto actor1 = createTestActor();
    auto actor2 = createTestActor();
    auto actor3 = createTestActor();

    manager->addActor(actor1);
    manager->addActor(actor2);
    manager->addActor(actor3);

    manager->hideAll();
    manager->showAll();

    EXPECT_EQ(actor1->GetVisibility(), 1);
    EXPECT_EQ(actor2->GetVisibility(), 1);
    EXPECT_EQ(actor3->GetVisibility(), 1);
}

TEST_F(ActorManagerTest, HideAll) {
    auto actor1 = createTestActor();
    auto actor2 = createTestActor();
    auto actor3 = createTestActor();

    manager->addActor(actor1);
    manager->addActor(actor2);
    manager->addActor(actor3);

    manager->hideAll();

    EXPECT_EQ(actor1->GetVisibility(), 0);
    EXPECT_EQ(actor2->GetVisibility(), 0);
    EXPECT_EQ(actor3->GetVisibility(), 0);
}

TEST_F(ActorManagerTest, ResetAll) {
    ActorId id1 = manager->addActor(createTestActor());
    ActorId id2 = manager->addActor(createTestActor());

    // Modify properties
    manager->setActorColor(id1, 1.0, 0.0, 0.0);
    manager->setActorOpacity(id2, 0.5);
    manager->highlightActor(id1);

    manager->resetAll();

    // Check default properties restored
    ActorProperties props;
    manager->getActorProperties(id1, props);

    EXPECT_DOUBLE_EQ(props.color[0], 0.8);  // Default color
    EXPECT_DOUBLE_EQ(props.opacity, 1.0);   // Default opacity

    EXPECT_TRUE(manager->getHighlightedActors().empty());
}

TEST_F(ActorManagerTest, ApplyToAll) {
    manager->addActor(createTestActor());
    manager->addActor(createTestActor());
    manager->addActor(createTestActor());

    ActorProperties props;
    props.color[0] = 0.1;
    props.color[1] = 0.2;
    props.color[2] = 0.3;
    props.opacity = 0.7;

    manager->applyToAll(props);

    // Verify all actors have the same properties
    for (ActorId id : manager->getAllActorIds()) {
        ActorProperties retrieved;
        manager->getActorProperties(id, retrieved);

        EXPECT_DOUBLE_EQ(retrieved.color[0], 0.1);
        EXPECT_DOUBLE_EQ(retrieved.color[1], 0.2);
        EXPECT_DOUBLE_EQ(retrieved.color[2], 0.3);
        EXPECT_DOUBLE_EQ(retrieved.opacity, 0.7);
    }
}

// ============================================================================
// Name Management Tests
// ============================================================================

TEST_F(ActorManagerTest, SetGetActorName) {
    ActorId id = manager->addActor(createTestActor());

    bool success = manager->setActorName(id, "MyActor");
    EXPECT_TRUE(success);

    std::string name = manager->getActorName(id);
    EXPECT_EQ(name, "MyActor");
}

TEST_F(ActorManagerTest, FindActorByName) {
    ActorId id1 = manager->addActor(createTestActor(), "Actor1");
    ActorId id2 = manager->addActor(createTestActor(), "Actor2");
    ActorId id3 = manager->addActor(createTestActor(), "Actor3");

    ActorId foundId = manager->findActorByName("Actor2");
    EXPECT_EQ(foundId, id2);

    foundId = manager->findActorByName("NonExistent");
    EXPECT_EQ(foundId, 0);
}

TEST_F(ActorManagerTest, GetNameNonexistentActor) {
    std::string name = manager->getActorName(999);
    EXPECT_TRUE(name.empty());
}

#else // !KOOMESH_HAS_VTK

// ============================================================================
// VTK Not Available Tests (Stub)
// ============================================================================

TEST_F(ActorManagerTest, StubOperations) {
    // All operations should be safe no-ops
    ActorId id = manager->addActor(nullptr);
    EXPECT_EQ(id, 0);

    EXPECT_EQ(manager->getActorCount(), 0);
    EXPECT_FALSE(manager->hasActor(1));
    EXPECT_EQ(manager->getActor(1), nullptr);
    EXPECT_TRUE(manager->getAllActorIds().empty());

    EXPECT_FALSE(manager->removeActor(1));
    manager->removeAllActors();

    ActorProperties props;
    EXPECT_FALSE(manager->setActorProperties(1, props));
    EXPECT_FALSE(manager->getActorProperties(1, props));
    EXPECT_FALSE(manager->setActorColor(1, 1.0, 0.0, 0.0));
    EXPECT_FALSE(manager->setActorOpacity(1, 0.5));
    EXPECT_FALSE(manager->setActorVisibility(1, true));
    EXPECT_FALSE(manager->setActorRepresentation(1, 1));

    GroupId groupId = manager->createGroup();
    EXPECT_EQ(groupId, 0);
    EXPECT_FALSE(manager->addActorToGroup(1, 1));
    EXPECT_FALSE(manager->removeActorFromGroup(1));
    EXPECT_TRUE(manager->getActorsInGroup(1).empty());
    EXPECT_EQ(manager->setGroupVisibility(1, true), 0);
    EXPECT_EQ(manager->setGroupProperties(1, props), 0);

    HighlightStyle style;
    manager->setHighlightStyle(style);
    EXPECT_FALSE(manager->highlightActor(1));
    EXPECT_FALSE(manager->unhighlightActor(1));
    manager->clearHighlights();
    EXPECT_TRUE(manager->getHighlightedActors().empty());

    manager->showAll();
    manager->hideAll();
    manager->resetAll();
    manager->applyToAll(props);

    EXPECT_FALSE(manager->setActorName(1, "Test"));
    EXPECT_TRUE(manager->getActorName(1).empty());
    EXPECT_EQ(manager->findActorByName("Test"), 0);
}

#endif // KOOMESH_HAS_VTK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
