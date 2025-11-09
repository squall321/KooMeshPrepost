#include <gtest/gtest.h>
#include "core/Part.h"
#include "helpers/TestUtils.h"

using namespace koomesh::core;

// ======================================================================
// Part Basic Tests
// ======================================================================

class PartTest : public ::testing::Test {
protected:
    Part* part;

    void SetUp() override {
        part = new Part(1, "TestPart");
    }

    void TearDown() override {
        delete part;
    }
};

TEST_F(PartTest, Construction) {
    EXPECT_EQ(1, part->id());
    EXPECT_EQ("TestPart", part->name());
    EXPECT_EQ(0, part->elementCount());
    EXPECT_TRUE(part->isEnabled());
}

TEST_F(PartTest, ConstructionWithoutName) {
    Part p(2);
    EXPECT_EQ(2, p.id());
    EXPECT_EQ("", p.name());
}

TEST_F(PartTest, SetName) {
    part->setName("NewName");
    EXPECT_EQ("NewName", part->name());
}

TEST_F(PartTest, AddElement) {
    part->addElement(100);
    EXPECT_EQ(1, part->elementCount());
    EXPECT_TRUE(part->containsElement(100));
}

TEST_F(PartTest, AddMultipleElements) {
    part->addElement(100);
    part->addElement(200);
    part->addElement(300);

    EXPECT_EQ(3, part->elementCount());
    EXPECT_TRUE(part->containsElement(100));
    EXPECT_TRUE(part->containsElement(200));
    EXPECT_TRUE(part->containsElement(300));
}

TEST_F(PartTest, AddDuplicateElement) {
    part->addElement(100);
    part->addElement(100);  // Duplicate

    // Set only stores unique elements
    EXPECT_EQ(1, part->elementCount());
}

TEST_F(PartTest, RemoveElement) {
    part->addElement(100);
    part->addElement(200);

    bool removed = part->removeElement(100);
    EXPECT_TRUE(removed);
    EXPECT_EQ(1, part->elementCount());
    EXPECT_FALSE(part->containsElement(100));
    EXPECT_TRUE(part->containsElement(200));
}

TEST_F(PartTest, RemoveNonExistentElement) {
    part->addElement(100);

    bool removed = part->removeElement(999);
    EXPECT_FALSE(removed);
    EXPECT_EQ(1, part->elementCount());
}

TEST_F(PartTest, ContainsElement) {
    part->addElement(100);

    EXPECT_TRUE(part->containsElement(100));
    EXPECT_FALSE(part->containsElement(999));
}

TEST_F(PartTest, Clear) {
    part->addElement(100);
    part->addElement(200);
    part->addElement(300);

    part->clear();

    EXPECT_EQ(0, part->elementCount());
    EXPECT_FALSE(part->containsElement(100));
}

TEST_F(PartTest, ElementsGetter) {
    part->addElement(100);
    part->addElement(200);

    const auto& elements = part->elements();
    EXPECT_EQ(2, elements.size());
}

// ======================================================================
// MaterialProperties Tests
// ======================================================================

class MaterialPropertiesTest : public ::testing::Test {
protected:
    Part::MaterialProperties material;
};

TEST_F(MaterialPropertiesTest, DefaultConstruction) {
    EXPECT_DOUBLE_EQ(0.0, material.density);
    EXPECT_DOUBLE_EQ(0.0, material.youngModulus);
    EXPECT_DOUBLE_EQ(0.0, material.poissonRatio);
    EXPECT_DOUBLE_EQ(0.0, material.yieldStrength);
    EXPECT_DOUBLE_EQ(0.0, material.ultimateStrength);
    EXPECT_EQ("", material.materialName);
    EXPECT_EQ(0, material.materialType);
}

TEST_F(MaterialPropertiesTest, SteelProperties) {
    auto steel = Part::MaterialProperties::Steel();

    EXPECT_DOUBLE_EQ(7850.0, steel.density);
    EXPECT_DOUBLE_EQ(200.0e9, steel.youngModulus);
    EXPECT_DOUBLE_EQ(0.3, steel.poissonRatio);
    EXPECT_DOUBLE_EQ(250.0e6, steel.yieldStrength);
    EXPECT_DOUBLE_EQ(400.0e6, steel.ultimateStrength);
    EXPECT_DOUBLE_EQ(12.0e-6, steel.thermalExpansion);
    EXPECT_DOUBLE_EQ(50.0, steel.thermalConductivity);
    EXPECT_DOUBLE_EQ(500.0, steel.specificHeat);
    EXPECT_EQ("Steel", steel.materialName);
    EXPECT_EQ(1, steel.materialType);
}

TEST_F(MaterialPropertiesTest, AluminumProperties) {
    auto aluminum = Part::MaterialProperties::Aluminum();

    EXPECT_DOUBLE_EQ(2700.0, aluminum.density);
    EXPECT_DOUBLE_EQ(70.0e9, aluminum.youngModulus);
    EXPECT_DOUBLE_EQ(0.33, aluminum.poissonRatio);
    EXPECT_DOUBLE_EQ(95.0e6, aluminum.yieldStrength);
    EXPECT_DOUBLE_EQ(110.0e6, aluminum.ultimateStrength);
    EXPECT_DOUBLE_EQ(23.0e-6, aluminum.thermalExpansion);
    EXPECT_DOUBLE_EQ(205.0, aluminum.thermalConductivity);
    EXPECT_DOUBLE_EQ(900.0, aluminum.specificHeat);
    EXPECT_EQ("Aluminum", aluminum.materialName);
    EXPECT_EQ(1, aluminum.materialType);
}

TEST_F(MaterialPropertiesTest, ValidityCheck_Valid) {
    material.density = 7850.0;
    material.youngModulus = 200.0e9;
    material.poissonRatio = 0.3;

    EXPECT_TRUE(material.isValid());
}

TEST_F(MaterialPropertiesTest, ValidityCheck_NegativeDensity) {
    material.density = -1.0;
    material.youngModulus = 200.0e9;
    material.poissonRatio = 0.3;

    EXPECT_FALSE(material.isValid());
}

TEST_F(MaterialPropertiesTest, ValidityCheck_NegativeYoungModulus) {
    material.density = 7850.0;
    material.youngModulus = -1.0;
    material.poissonRatio = 0.3;

    EXPECT_FALSE(material.isValid());
}

TEST_F(MaterialPropertiesTest, ValidityCheck_InvalidPoissonRatio) {
    material.density = 7850.0;
    material.youngModulus = 200.0e9;

    // Too low
    material.poissonRatio = -1.5;
    EXPECT_FALSE(material.isValid());

    // Too high
    material.poissonRatio = 0.6;
    EXPECT_FALSE(material.isValid());

    // Valid boundary
    material.poissonRatio = -1.0;
    EXPECT_TRUE(material.isValid());

    material.poissonRatio = 0.5;
    EXPECT_TRUE(material.isValid());
}

TEST_F(MaterialPropertiesTest, SetAndGetMaterial) {
    Part part(1, "SteelPart");

    auto steel = Part::MaterialProperties::Steel();
    part.setMaterialProperties(steel);

    const auto& mat = part.materialProperties();
    EXPECT_DOUBLE_EQ(7850.0, mat.density);
    EXPECT_EQ("Steel", mat.materialName);
}

TEST_F(MaterialPropertiesTest, ModifyMaterialProperties) {
    Part part(1, "CustomPart");

    auto& mat = part.materialProperties();
    mat.density = 5000.0;
    mat.youngModulus = 150.0e9;
    mat.poissonRatio = 0.25;
    mat.materialName = "CustomMaterial";

    const auto& constMat = part.materialProperties();
    EXPECT_DOUBLE_EQ(5000.0, constMat.density);
    EXPECT_EQ("CustomMaterial", constMat.materialName);
}

// ======================================================================
// Color Tests
// ======================================================================

class ColorTest : public ::testing::Test {
protected:
    Part::Color color;
};

TEST_F(ColorTest, DefaultConstruction) {
    EXPECT_FLOAT_EQ(0.7f, color.r);
    EXPECT_FLOAT_EQ(0.7f, color.g);
    EXPECT_FLOAT_EQ(0.7f, color.b);
    EXPECT_FLOAT_EQ(1.0f, color.a);
}

TEST_F(ColorTest, ConstructionWithValues) {
    Part::Color red(1.0f, 0.0f, 0.0f, 0.5f);

    EXPECT_FLOAT_EQ(1.0f, red.r);
    EXPECT_FLOAT_EQ(0.0f, red.g);
    EXPECT_FLOAT_EQ(0.0f, red.b);
    EXPECT_FLOAT_EQ(0.5f, red.a);
}

TEST_F(ColorTest, PredefinedColors) {
    auto red = Part::Color::Red();
    EXPECT_FLOAT_EQ(1.0f, red.r);
    EXPECT_FLOAT_EQ(0.0f, red.g);
    EXPECT_FLOAT_EQ(0.0f, red.b);

    auto green = Part::Color::Green();
    EXPECT_FLOAT_EQ(0.0f, green.r);
    EXPECT_FLOAT_EQ(1.0f, green.g);
    EXPECT_FLOAT_EQ(0.0f, green.b);

    auto blue = Part::Color::Blue();
    EXPECT_FLOAT_EQ(0.0f, blue.r);
    EXPECT_FLOAT_EQ(0.0f, blue.g);
    EXPECT_FLOAT_EQ(1.0f, blue.b);

    auto yellow = Part::Color::Yellow();
    EXPECT_FLOAT_EQ(1.0f, yellow.r);
    EXPECT_FLOAT_EQ(1.0f, yellow.g);
    EXPECT_FLOAT_EQ(0.0f, yellow.b);

    auto cyan = Part::Color::Cyan();
    EXPECT_FLOAT_EQ(0.0f, cyan.r);
    EXPECT_FLOAT_EQ(1.0f, cyan.g);
    EXPECT_FLOAT_EQ(1.0f, cyan.b);

    auto magenta = Part::Color::Magenta();
    EXPECT_FLOAT_EQ(1.0f, magenta.r);
    EXPECT_FLOAT_EQ(0.0f, magenta.g);
    EXPECT_FLOAT_EQ(1.0f, magenta.b);

    auto gray = Part::Color::Gray();
    EXPECT_FLOAT_EQ(0.7f, gray.r);
    EXPECT_FLOAT_EQ(0.7f, gray.g);
    EXPECT_FLOAT_EQ(0.7f, gray.b);
}

TEST_F(ColorTest, SetAndGetColor) {
    Part part(1, "ColoredPart");

    auto red = Part::Color::Red();
    part.setColor(red);

    const auto& c = part.color();
    EXPECT_FLOAT_EQ(1.0f, c.r);
    EXPECT_FLOAT_EQ(0.0f, c.g);
    EXPECT_FLOAT_EQ(0.0f, c.b);
}

// ======================================================================
// Enable/Disable Tests
// ======================================================================

class PartEnableTest : public ::testing::Test {
protected:
    Part part{1, "TestPart"};
};

TEST_F(PartEnableTest, DefaultEnabled) {
    EXPECT_TRUE(part.isEnabled());
}

TEST_F(PartEnableTest, Disable) {
    part.setEnabled(false);
    EXPECT_FALSE(part.isEnabled());
}

TEST_F(PartEnableTest, Enable) {
    part.setEnabled(false);
    part.setEnabled(true);
    EXPECT_TRUE(part.isEnabled());
}

// ======================================================================
// Comparison Operators Tests
// ======================================================================

class PartComparisonTest : public ::testing::Test {
protected:
    Part part1{1, "Part1"};
    Part part2{2, "Part2"};
    Part part3{1, "Part1Copy"};
};

TEST_F(PartComparisonTest, Equality_SameId) {
    EXPECT_TRUE(part1 == part3);  // Same ID, different name
}

TEST_F(PartComparisonTest, Equality_DifferentId) {
    EXPECT_FALSE(part1 == part2);
}

TEST_F(PartComparisonTest, Inequality_SameId) {
    EXPECT_FALSE(part1 != part3);
}

TEST_F(PartComparisonTest, Inequality_DifferentId) {
    EXPECT_TRUE(part1 != part2);
}

// ======================================================================
// ElementTypeStats Tests
// ======================================================================

class ElementTypeStatsTest : public ::testing::Test {
protected:
    Part::ElementTypeStats stats;
};

TEST_F(ElementTypeStatsTest, DefaultConstruction) {
    EXPECT_EQ(0, stats.tetrahedronCount);
    EXPECT_EQ(0, stats.hexahedronCount);
    EXPECT_EQ(0, stats.pentahedronCount);
    EXPECT_EQ(0, stats.pyramidCount);
    EXPECT_EQ(0, stats.triangleCount);
    EXPECT_EQ(0, stats.quadrilateralCount);
    EXPECT_EQ(0, stats.beamCount);
    EXPECT_EQ(0, stats.unknownCount);
    EXPECT_EQ(0, stats.totalCount());
}

TEST_F(ElementTypeStatsTest, TotalCount) {
    stats.tetrahedronCount = 10;
    stats.hexahedronCount = 20;
    stats.triangleCount = 5;
    stats.beamCount = 3;

    EXPECT_EQ(38, stats.totalCount());
}

TEST_F(ElementTypeStatsTest, AllTypesCount) {
    stats.tetrahedronCount = 1;
    stats.hexahedronCount = 2;
    stats.pentahedronCount = 3;
    stats.pyramidCount = 4;
    stats.triangleCount = 5;
    stats.quadrilateralCount = 6;
    stats.beamCount = 7;
    stats.unknownCount = 8;

    EXPECT_EQ(36, stats.totalCount());
}

// ======================================================================
// Integration Tests
// ======================================================================

class PartIntegrationTest : public ::testing::Test {};

TEST_F(PartIntegrationTest, CompleteWorkflow) {
    // Create part
    Part chassis(1, "Chassis");

    // Set material
    auto steel = Part::MaterialProperties::Steel();
    chassis.setMaterialProperties(steel);

    // Set color
    chassis.setColor(Part::Color::Blue());

    // Add elements
    for (ElementId i = 1; i <= 100; ++i) {
        chassis.addElement(i);
    }

    // Verify
    EXPECT_EQ(1, chassis.id());
    EXPECT_EQ("Chassis", chassis.name());
    EXPECT_EQ(100, chassis.elementCount());
    EXPECT_DOUBLE_EQ(7850.0, chassis.materialProperties().density);
    EXPECT_FLOAT_EQ(0.0f, chassis.color().r);
    EXPECT_FLOAT_EQ(0.0f, chassis.color().g);
    EXPECT_FLOAT_EQ(1.0f, chassis.color().b);
    EXPECT_TRUE(chassis.isEnabled());
}

TEST_F(PartIntegrationTest, MultiplePartsWithDifferentMaterials) {
    Part steelPart(1, "Steel Component");
    steelPart.setMaterialProperties(Part::MaterialProperties::Steel());
    steelPart.setColor(Part::Color::Gray());

    Part aluminumPart(2, "Aluminum Component");
    aluminumPart.setMaterialProperties(Part::MaterialProperties::Aluminum());
    aluminumPart.setColor(Part::Color::Cyan());

    // Verify different materials
    EXPECT_DOUBLE_EQ(7850.0, steelPart.materialProperties().density);
    EXPECT_DOUBLE_EQ(2700.0, aluminumPart.materialProperties().density);

    EXPECT_DOUBLE_EQ(200.0e9, steelPart.materialProperties().youngModulus);
    EXPECT_DOUBLE_EQ(70.0e9, aluminumPart.materialProperties().youngModulus);
}

TEST_F(PartIntegrationTest, LargeElementSet) {
    Part largePart(1, "Large Part");

    // Add 1 million elements
    for (ElementId i = 1; i <= 1000000; ++i) {
        largePart.addElement(i);
    }

    EXPECT_EQ(1000000, largePart.elementCount());
    EXPECT_TRUE(largePart.containsElement(1));
    EXPECT_TRUE(largePart.containsElement(500000));
    EXPECT_TRUE(largePart.containsElement(1000000));
    EXPECT_FALSE(largePart.containsElement(1000001));
}

TEST_F(PartIntegrationTest, ElementRemovalPerformance) {
    Part part(1, "Test Part");

    // Add 10000 elements
    for (ElementId i = 1; i <= 10000; ++i) {
        part.addElement(i);
    }

    // Remove every other element
    for (ElementId i = 2; i <= 10000; i += 2) {
        part.removeElement(i);
    }

    EXPECT_EQ(5000, part.elementCount());

    // Verify odd elements remain
    EXPECT_TRUE(part.containsElement(1));
    EXPECT_FALSE(part.containsElement(2));
    EXPECT_TRUE(part.containsElement(3));
    EXPECT_FALSE(part.containsElement(4));
}

TEST_F(PartIntegrationTest, CustomMaterialDefinition) {
    Part customPart(1, "Custom Material Part");

    auto& mat = customPart.materialProperties();
    mat.density = 3500.0;
    mat.youngModulus = 120.0e9;
    mat.poissonRatio = 0.28;
    mat.yieldStrength = 180.0e6;
    mat.ultimateStrength = 220.0e6;
    mat.thermalExpansion = 15.0e-6;
    mat.thermalConductivity = 80.0;
    mat.specificHeat = 600.0;
    mat.materialName = "CustomAlloy";
    mat.materialType = 2;

    EXPECT_TRUE(mat.isValid());
    EXPECT_EQ("CustomAlloy", mat.materialName);
    EXPECT_EQ(2, mat.materialType);
}

TEST_F(PartIntegrationTest, PartCopy) {
    Part original(1, "Original");
    original.addElement(100);
    original.addElement(200);
    original.setMaterialProperties(Part::MaterialProperties::Steel());
    original.setColor(Part::Color::Red());
    original.setEnabled(false);

    // Copy constructor
    Part copy = original;

    EXPECT_EQ(original.id(), copy.id());
    EXPECT_EQ(original.name(), copy.name());
    EXPECT_EQ(original.elementCount(), copy.elementCount());
    EXPECT_TRUE(copy.containsElement(100));
    EXPECT_TRUE(copy.containsElement(200));
    EXPECT_DOUBLE_EQ(original.materialProperties().density, copy.materialProperties().density);
    EXPECT_FLOAT_EQ(original.color().r, copy.color().r);
    EXPECT_EQ(original.isEnabled(), copy.isEnabled());
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
