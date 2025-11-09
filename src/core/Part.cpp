#include "core/Part.h"

namespace koomesh {
namespace core {

// ======================================================================
// 생성자
// ======================================================================

Part::Part(PartId id, const std::string& name)
    : m_id(id)
    , m_name(name)
    , m_material()
    , m_color()
    , m_enabled(true) {
}

// ======================================================================
// 요소 관리
// ======================================================================

void Part::addElement(ElementId id) {
    m_elementIds.insert(id);
}

bool Part::removeElement(ElementId id) {
    return m_elementIds.erase(id) > 0;
}

bool Part::containsElement(ElementId id) const {
    return m_elementIds.find(id) != m_elementIds.end();
}

void Part::clear() {
    m_elementIds.clear();
}

} // namespace core
} // namespace koomesh
