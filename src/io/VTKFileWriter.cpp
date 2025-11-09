#include "io/VTKFileWriter.h"
#include "io/FileIOException.h"
#include <algorithm>

namespace koomesh {
namespace io {

// ======================================================================
// VTKFileWriter Implementation (STUB)
// ======================================================================

VTKFileWriter::VTKFileWriter()
    : m_progress(0.0)
    , m_cancelRequested(false)
    , m_isWriting(false)
{
}

VTKFileWriter::~VTKFileWriter() = default;

bool VTKFileWriter::canWrite(const std::string& filename) const {
    auto extensions = supportedExtensions();
    for (const auto& ext : extensions) {
        if (filename.length() >= ext.length()) {
            std::string fileExt = filename.substr(filename.length() - ext.length());
            std::string extLower = ext;
            std::string fileExtLower = fileExt;
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
            std::transform(fileExtLower.begin(), fileExtLower.end(), fileExtLower.begin(), ::tolower);
            if (extLower == fileExtLower) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> VTKFileWriter::supportedExtensions() const {
    return {".vtk", ".vtu", ".vtp"};
}

std::string VTKFileWriter::formatName() const {
    return "VTK (Visualization Toolkit)";
}

WriteResult VTKFileWriter::write(
    const std::string& filename,
    const core::Mesh& mesh,
    const WriteOptions& options,
    ProgressCallback progressCallback)
{
    WriteResult result;
    result.success = false;
    result.message = "VTK writer not yet implemented";

    // TODO: Implement VTK file writing
    // 1. Choose format based on extension (.vtk = Legacy, .vtu = XML Unstructured)
    // 2. Write header
    // 3. Write POINTS section (all nodes)
    // 4. Write CELLS section (element connectivity)
    // 5. Write CELL_TYPES section (map ElementType to VTK cell types)
    // 6. Optionally write POINT_DATA or CELL_DATA
    //
    // VTK cell type mapping:
    // - TRIANGLE -> VTK_TRIANGLE (5)
    // - QUADRILATERAL -> VTK_QUAD (9)
    // - TETRAHEDRON -> VTK_TETRA (10)
    // - HEXAHEDRON -> VTK_HEXAHEDRON (12)
    // - PENTAHEDRON -> VTK_WEDGE (13)
    // - PYRAMID -> VTK_PYRAMID (14)

    throw FileIOException("VTK format support not yet implemented: " + filename);

    return result;
}

double VTKFileWriter::progress() const {
    return m_progress.load();
}

void VTKFileWriter::cancel() {
    m_cancelRequested = true;
}

bool VTKFileWriter::isWriting() const {
    return m_isWriting.load();
}

// ======================================================================
// Registration
// ======================================================================

void registerVTKWriter() {
    // TODO: Uncomment when implementation is ready
    // FileWriterFactory::registerWriter([]() {
    //     return std::make_unique<VTKFileWriter>();
    // });
}

} // namespace io
} // namespace koomesh
