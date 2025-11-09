#include "io/STLFileReader.h"
#include "io/FileIOException.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <cstring>

namespace koomesh {
namespace io {

// ======================================================================
// Helper structures
// ======================================================================

struct Triangle {
    Eigen::Vector3d normal;
    Eigen::Vector3d vertices[3];
};

// Hash function for vertex positions (for merging)
struct Vector3dHash {
    std::size_t operator()(const Eigen::Vector3d& v) const {
        std::size_t h1 = std::hash<double>{}(v.x());
        std::size_t h2 = std::hash<double>{}(v.y());
        std::size_t h3 = std::hash<double>{}(v.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Equality comparison with tolerance
struct Vector3dEqual {
    double tolerance;

    Vector3dEqual(double tol = 1e-6) : tolerance(tol) {}

    bool operator()(const Eigen::Vector3d& a, const Eigen::Vector3d& b) const {
        return (a - b).norm() < tolerance;
    }
};

// ======================================================================
// STLFileReader Implementation
// ======================================================================

STLFileReader::STLFileReader()
    : m_progress(0.0)
    , m_cancelRequested(false)
    , m_isReading(false)
{
}

STLFileReader::~STLFileReader() = default;

bool STLFileReader::canRead(const std::string& filename) const {
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

std::vector<std::string> STLFileReader::supportedExtensions() const {
    return {".stl"};
}

std::string STLFileReader::formatName() const {
    return "STL (STereoLithography)";
}

ReadResult STLFileReader::read(
    const std::string& filename,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback progressCallback)
{
    STLReadOptions stlOptions;
    // Use generic read options
    return read(filename, mesh, stlOptions, progressCallback);
}

ReadResult STLFileReader::read(
    const std::string& filename,
    core::Mesh& mesh,
    const STLReadOptions& options,
    ProgressCallback progressCallback)
{
    ReadResult result;
    result.success = false;

    m_isReading = true;
    m_cancelRequested = false;
    m_progress = 0.0;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Update progress: Opening file
        if (!updateProgress(0.05, "Opening file...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Detect format
        bool isAscii = isAsciiSTL(filename);

        // Update progress: Parsing
        if (!updateProgress(0.1, "Parsing STL data...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Read based on format
        if (isAscii) {
            readAscii(filename, mesh, options, progressCallback);
        } else {
            readBinary(filename, mesh, options, progressCallback);
        }

        // Update progress: Complete
        updateProgress(1.0, "Complete", progressCallback);

        // Build result
        result.success = true;
        result.nodesRead = mesh.nodeCount();
        result.elementsRead = mesh.elementCount();
        result.message = "STL file read successfully";

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
    result.readTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

    m_isReading = false;
    m_progress = 1.0;

    return result;
}

double STLFileReader::progress() const {
    return m_progress.load();
}

void STLFileReader::cancel() {
    m_cancelRequested = true;
}

bool STLFileReader::isReading() const {
    return m_isReading.load();
}

// ======================================================================
// Private Methods
// ======================================================================

bool STLFileReader::isAsciiSTL(const std::string& filename) const {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw FileOpenException(filename, "Cannot open file");
    }

    // Read first 80 bytes
    char header[80];
    file.read(header, 80);

    // ASCII STL starts with "solid"
    std::string headerStr(header, std::min(size_t(5), size_t(80)));
    return headerStr.find("solid") != std::string::npos;
}

void STLFileReader::readAscii(
    const std::string& filename,
    core::Mesh& mesh,
    const STLReadOptions& options,
    ProgressCallback progressCallback)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw FileOpenException(filename, "Cannot open file");
    }

    std::string line;
    std::vector<Triangle> triangles;
    Triangle currentTriangle;
    int vertexIndex = 0;

    // Get file size for progress tracking
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t bytesRead = 0;

    while (std::getline(file, line)) {
        bytesRead += line.size() + 1;

        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "facet") {
            // facet normal nx ny nz
            std::string normal_keyword;
            iss >> normal_keyword; // "normal"
            iss >> currentTriangle.normal.x()
                >> currentTriangle.normal.y()
                >> currentTriangle.normal.z();
            vertexIndex = 0;
        }
        else if (keyword == "vertex") {
            // vertex x y z
            if (vertexIndex < 3) {
                iss >> currentTriangle.vertices[vertexIndex].x()
                    >> currentTriangle.vertices[vertexIndex].y()
                    >> currentTriangle.vertices[vertexIndex].z();
                vertexIndex++;
            }
        }
        else if (keyword == "endfacet") {
            if (vertexIndex == 3) {
                triangles.push_back(currentTriangle);
            }
        }

        // Update progress periodically
        if (triangles.size() % 1000 == 0) {
            double progress = 0.1 + (0.8 * bytesRead / fileSize);
            updateProgress(progress, "Reading triangles...", progressCallback);
        }
    }

    file.close();

    // Convert triangles to mesh
    if (!updateProgress(0.9, "Building mesh...", progressCallback)) {
        throw OperationCancelledException();
    }

    if (options.mergeVertices) {
        mergeVertices(mesh, options.mergeTolerance);
    }

    // Create nodes and elements
    core::NodeId nodeId = 1;
    core::ElementId elemId = 1;
    core::PartId defaultPartId = 1;

    // Create default part for STL data
    core::Part defaultPart(defaultPartId, "STL Surface");
    mesh.addPart(defaultPart);

    for (const auto& tri : triangles) {
        // Create three nodes for this triangle
        core::NodeId n1 = nodeId++;
        core::NodeId n2 = nodeId++;
        core::NodeId n3 = nodeId++;

        mesh.addNode(core::Node(n1, tri.vertices[0]));
        mesh.addNode(core::Node(n2, tri.vertices[1]));
        mesh.addNode(core::Node(n3, tri.vertices[2]));

        // Create triangle element
        auto element = std::make_unique<core::TriangleElement>(
            elemId++,
            defaultPartId,
            std::vector<core::NodeId>{n1, n2, n3}
        );

        mesh.addElement(std::move(element));
    }
}

void STLFileReader::readBinary(
    const std::string& filename,
    core::Mesh& mesh,
    const STLReadOptions& options,
    ProgressCallback progressCallback)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw FileOpenException(filename, "Cannot open file");
    }

    // Read header (80 bytes)
    char header[80];
    file.read(header, 80);

    // Read number of triangles
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(uint32_t));

    if (!updateProgress(0.1, "Reading binary STL...", progressCallback)) {
        throw OperationCancelledException();
    }

    // Create default part
    core::PartId defaultPartId = 1;
    core::Part defaultPart(defaultPartId, "STL Surface");
    mesh.addPart(defaultPart);

    core::NodeId nodeId = 1;
    core::ElementId elemId = 1;

    // Read triangles
    for (uint32_t i = 0; i < numTriangles; ++i) {
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        // Each triangle: 3 floats (normal) + 9 floats (3 vertices) + 2 bytes (attribute)
        float normal[3];
        float vertex1[3], vertex2[3], vertex3[3];
        uint16_t attribute;

        file.read(reinterpret_cast<char*>(normal), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(vertex1), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(vertex2), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(vertex3), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(&attribute), sizeof(uint16_t));

        // Create nodes
        core::NodeId n1 = nodeId++;
        core::NodeId n2 = nodeId++;
        core::NodeId n3 = nodeId++;

        mesh.addNode(core::Node(n1, Eigen::Vector3d(vertex1[0], vertex1[1], vertex1[2])));
        mesh.addNode(core::Node(n2, Eigen::Vector3d(vertex2[0], vertex2[1], vertex2[2])));
        mesh.addNode(core::Node(n3, Eigen::Vector3d(vertex3[0], vertex3[1], vertex3[2])));

        // Create triangle element
        auto element = std::make_unique<core::TriangleElement>(
            elemId++,
            defaultPartId,
            std::vector<core::NodeId>{n1, n2, n3}
        );

        mesh.addElement(std::move(element));

        // Update progress periodically
        if (i % 1000 == 0) {
            double progress = 0.1 + (0.8 * i / numTriangles);
            std::ostringstream msg;
            msg << "Reading triangle " << i << "/" << numTriangles;
            updateProgress(progress, msg.str(), progressCallback);
        }
    }

    file.close();
}

void STLFileReader::mergeVertices(
    core::Mesh& mesh,
    double tolerance)
{
    // TODO: Implement vertex merging to reduce duplicate nodes
    // This would require building a spatial hash and remapping element connectivity
}

bool STLFileReader::updateProgress(
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


} // namespace io
} // namespace koomesh
