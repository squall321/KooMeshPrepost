#include "io/STLFileWriter.h"
#include "io/FileIOException.h"
#include "io/IFileReader.h"  // For ProgressCallback definition
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>

namespace koomesh {
namespace io {

// ======================================================================
// STLFileWriter Implementation
// ======================================================================

STLFileWriter::STLFileWriter()
    : m_progress(0.0)
    , m_cancelRequested(false)
    , m_isWriting(false)
{
}

STLFileWriter::~STLFileWriter() = default;

bool STLFileWriter::canWrite(const std::string& filename) const {
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

std::vector<std::string> STLFileWriter::supportedExtensions() const {
    return {".stl"};
}

std::string STLFileWriter::formatName() const {
    return "STL (STereoLithography)";
}

WriteResult STLFileWriter::write(
    const std::string& filename,
    const core::Mesh& mesh,
    const WriteOptions& options,
    ProgressCallback progressCallback)
{
    STLWriteOptions stlOptions;
    // Convert generic options
    return write(filename, mesh, stlOptions, progressCallback);
}

WriteResult STLFileWriter::write(
    const std::string& filename,
    const core::Mesh& mesh,
    const STLWriteOptions& options,
    ProgressCallback progressCallback)
{
    WriteResult result;
    result.success = false;

    m_isWriting = true;
    m_cancelRequested = false;
    m_progress = 0.0;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Update progress: Opening file
        if (!updateProgress(0.05, "Opening file...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write based on format
        if (options.format == STLFormat::ASCII) {
            writeAscii(filename, mesh, options, progressCallback);
        } else {
            writeBinary(filename, mesh, options, progressCallback);
        }

        // Update progress: Complete
        updateProgress(1.0, "Complete", progressCallback);

        // Build result
        result.success = true;
        result.message = "STL file written successfully";

    } catch (const OperationCancelledException&) {
        result.success = false;
        result.message = "Operation cancelled by user";
        throw;
    } catch (const FileIOException& e) {
        result.success = false;
        result.message = e.message();
        throw;
    } catch (const std::exception& e) {
        result.success = false;
        result.message = std::string("Unexpected error: ") + e.what();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.writeTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

    m_isWriting = false;
    m_progress = 1.0;

    return result;
}

double STLFileWriter::progress() const {
    return m_progress.load();
}

void STLFileWriter::cancel() {
    m_cancelRequested = true;
}

bool STLFileWriter::isWriting() const {
    return m_isWriting.load();
}

// ======================================================================
// Private Methods
// ======================================================================

void STLFileWriter::writeAscii(
    const std::string& filename,
    const core::Mesh& mesh,
    const STLWriteOptions& options,
    ProgressCallback progressCallback)
{
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw FileOpenException(filename, "Cannot open file for writing");
    }

    // Write header
    out << "solid " << options.solidName << "\n";

    // Get all elements
    const auto& elements = mesh.elements();
    size_t totalElements = elements.size();
    size_t elementsProcessed = 0;
    size_t trianglesWritten = 0;

    for (const auto& [id, elemPtr] : elements) {
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        const auto& element = *elemPtr;

        // Only export triangles
        if (element.type() != core::ElementType::TRIANGLE) {
            if (element.type() == core::ElementType::QUADRILATERAL && options.triangulateQuads) {
                // TODO: Implement quad triangulation
                continue;
            }
            if (options.exportOnlyTriangles) {
                continue;
            }
        }

        if (element.type() == core::ElementType::TRIANGLE) {
            // Get nodes
            const auto& nodeIds = element.nodeIds();
            if (nodeIds.size() != 3) continue;

            const core::Node* n1 = mesh.getNode(nodeIds[0]);
            const core::Node* n2 = mesh.getNode(nodeIds[1]);
            const core::Node* n3 = mesh.getNode(nodeIds[2]);

            if (!n1 || !n2 || !n3) continue;

            const auto& v1 = n1->coordinates();
            const auto& v2 = n2->coordinates();
            const auto& v3 = n3->coordinates();

            // Calculate normal
            Eigen::Vector3d normal = calculateNormal(v1, v2, v3);

            // Write facet
            out << "  facet normal "
                << std::scientific << std::setprecision(6)
                << normal.x() << " " << normal.y() << " " << normal.z() << "\n";
            out << "    outer loop\n";
            out << "      vertex "
                << v1.x() << " " << v1.y() << " " << v1.z() << "\n";
            out << "      vertex "
                << v2.x() << " " << v2.y() << " " << v2.z() << "\n";
            out << "      vertex "
                << v3.x() << " " << v3.y() << " " << v3.z() << "\n";
            out << "    endloop\n";
            out << "  endfacet\n";

            trianglesWritten++;
        }

        elementsProcessed++;

        // Update progress periodically
        if (elementsProcessed % 1000 == 0) {
            double progress = 0.1 + (0.85 * elementsProcessed / totalElements);
            std::ostringstream msg;
            msg << "Writing triangle " << trianglesWritten;
            updateProgress(progress, msg.str(), progressCallback);
        }
    }

    // Write footer
    out << "endsolid " << options.solidName << "\n";

    out.close();
}

void STLFileWriter::writeBinary(
    const std::string& filename,
    const core::Mesh& mesh,
    const STLWriteOptions& options,
    ProgressCallback progressCallback)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        throw FileOpenException(filename, "Cannot open file for writing");
    }

    // Count triangles first
    size_t triangleCount = 0;
    const auto& elements = mesh.elements();
    for (const auto& [id, elemPtr] : elements) {
        if (elemPtr->type() == core::ElementType::TRIANGLE) {
            triangleCount++;
        }
    }

    // Write header (80 bytes)
    char header[80];
    std::memset(header, 0, 80);
    std::string headerStr = "Binary STL generated by KooMeshPrepost";
    std::memcpy(header, headerStr.c_str(), std::min(headerStr.size(), size_t(80)));
    out.write(header, 80);

    // Write number of triangles
    uint32_t numTriangles = static_cast<uint32_t>(triangleCount);
    out.write(reinterpret_cast<const char*>(&numTriangles), sizeof(uint32_t));

    // Write triangles
    size_t elementsProcessed = 0;
    size_t trianglesWritten = 0;

    for (const auto& [id, elemPtr] : elements) {
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        const auto& element = *elemPtr;

        if (element.type() == core::ElementType::TRIANGLE) {
            // Get nodes
            const auto& nodeIds = element.nodeIds();
            if (nodeIds.size() != 3) continue;

            const core::Node* n1 = mesh.getNode(nodeIds[0]);
            const core::Node* n2 = mesh.getNode(nodeIds[1]);
            const core::Node* n3 = mesh.getNode(nodeIds[2]);

            if (!n1 || !n2 || !n3) continue;

            const auto& v1 = n1->coordinates();
            const auto& v2 = n2->coordinates();
            const auto& v3 = n3->coordinates();

            // Calculate normal
            Eigen::Vector3d normal = calculateNormal(v1, v2, v3);

            // Write normal (3 floats)
            float normalFloat[3] = {
                static_cast<float>(normal.x()),
                static_cast<float>(normal.y()),
                static_cast<float>(normal.z())
            };
            out.write(reinterpret_cast<const char*>(normalFloat), 3 * sizeof(float));

            // Write vertices (9 floats)
            float vertex1[3] = {
                static_cast<float>(v1.x()),
                static_cast<float>(v1.y()),
                static_cast<float>(v1.z())
            };
            float vertex2[3] = {
                static_cast<float>(v2.x()),
                static_cast<float>(v2.y()),
                static_cast<float>(v2.z())
            };
            float vertex3[3] = {
                static_cast<float>(v3.x()),
                static_cast<float>(v3.y()),
                static_cast<float>(v3.z())
            };

            out.write(reinterpret_cast<const char*>(vertex1), 3 * sizeof(float));
            out.write(reinterpret_cast<const char*>(vertex2), 3 * sizeof(float));
            out.write(reinterpret_cast<const char*>(vertex3), 3 * sizeof(float));

            // Write attribute (2 bytes, usually 0)
            uint16_t attribute = 0;
            out.write(reinterpret_cast<const char*>(&attribute), sizeof(uint16_t));

            trianglesWritten++;

            // Update progress periodically
            if (trianglesWritten % 1000 == 0) {
                double progress = 0.1 + (0.85 * trianglesWritten / numTriangles);
                std::ostringstream msg;
                msg << "Writing triangle " << trianglesWritten << "/" << numTriangles;
                updateProgress(progress, msg.str(), progressCallback);
            }
        }

        elementsProcessed++;
    }

    out.close();
}

Eigen::Vector3d STLFileWriter::calculateNormal(
    const Eigen::Vector3d& v1,
    const Eigen::Vector3d& v2,
    const Eigen::Vector3d& v3) const
{
    Eigen::Vector3d edge1 = v2 - v1;
    Eigen::Vector3d edge2 = v3 - v1;
    Eigen::Vector3d normal = edge1.cross(edge2);

    double length = normal.norm();
    if (length > 1e-10) {
        normal /= length;
    }

    return normal;
}

bool STLFileWriter::updateProgress(
    double progress,
    const std::string& message,
    ProgressCallback callback)
{
    m_progress = progress;

    if (callback) {
        bool shouldContinue = callback(progress, message);
        if (!shouldContinue) {
            m_cancelRequested = true;
            return false;
        }
    }

    return !m_cancelRequested;
}

// ======================================================================
// Registration
// ======================================================================

void registerSTLWriter() {
    FileWriterFactory::registerWriter([]() {
        return std::make_unique<STLFileWriter>();
    });
}

} // namespace io
} // namespace koomesh
