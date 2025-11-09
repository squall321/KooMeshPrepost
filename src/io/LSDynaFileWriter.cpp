#include "io/LSDynaFileWriter.h"
#include "io/FileIOException.h"
#include "io/IFileReader.h"  // For ProgressCallback definition
#include "core/MeshStatistics.h"
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <map>

namespace koomesh {
namespace io {

// ======================================================================
// LSDynaFileWriter Implementation
// ======================================================================

LSDynaFileWriter::LSDynaFileWriter() = default;

LSDynaFileWriter::~LSDynaFileWriter() = default;

bool LSDynaFileWriter::canWrite(const std::string& filename) const {
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

std::vector<std::string> LSDynaFileWriter::supportedExtensions() const {
    return {".k", ".key", ".dyn", ".keyword"};
}

std::string LSDynaFileWriter::formatName() const {
    return "LS-DYNA Keyword";
}

WriteResult LSDynaFileWriter::write(
    const std::string& filename,
    const core::Mesh& mesh,
    const WriteOptions& options,
    ProgressCallback progressCallback)
{
    LSDynaWriteOptions lsdynaOptions;
    // Convert generic options to LS-DYNA specific
    return write(filename, mesh, lsdynaOptions, progressCallback);
}

WriteResult LSDynaFileWriter::write(
    const std::string& filename,
    const core::Mesh& mesh,
    const LSDynaWriteOptions& options,
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

        // Open output file
        std::ofstream out(filename);
        if (!out.is_open()) {
            throw FileOpenException(filename, "Cannot open file for writing");
        }

        // Setup write context
        WriteContext context;
        context.filename = filename;
        context.format = options.format;

        // Update progress: Writing header
        if (!updateProgress(0.1, "Writing header...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write header
        if (options.writeHeader) {
            writeHeader(out, options);
        }

        // Update progress: Writing parts
        if (!updateProgress(0.15, "Writing parts...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write parts
        writeParts(out, mesh, options, context);

        // Update progress: Writing nodes
        if (!updateProgress(0.2, "Writing nodes...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write nodes
        writeNodes(out, mesh, options, context, progressCallback);

        // Update progress: Writing elements
        if (!updateProgress(0.6, "Writing elements...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write elements
        writeElements(out, mesh, options, context, progressCallback);

        // Update progress: Writing footer
        if (!updateProgress(0.95, "Writing footer...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Write footer
        if (options.writeEnd) {
            writeFooter(out, options);
        }

        // Update progress: Complete
        updateProgress(1.0, "Complete", progressCallback);

        out.close();

        // Build result
        result.success = true;
        result.nodesWritten = context.nodesWritten;
        result.elementsWritten = context.elementsWritten;
        result.partsWritten = context.partsWritten;
        result.warningCount = context.warnings.size();
        result.errorCount = context.errors.size();
        result.warnings = context.warnings;
        result.errors = context.errors;

        if (result.errorCount > 0) {
            result.message = "Writing completed with errors";
        } else if (result.warningCount > 0) {
            result.message = "Writing completed with warnings";
        } else {
            result.message = "Writing completed successfully";
        }

    } catch (const OperationCancelledException&) {
        result.success = false;
        result.message = "Operation cancelled by user";
        throw;
    } catch (const FileIOException& e) {
        result.success = false;
        result.message = e.message();
        result.errorCount = 1;
        result.errors.push_back(e.message());
        throw;
    } catch (const std::exception& e) {
        result.success = false;
        result.message = std::string("Unexpected error: ") + e.what();
        result.errorCount = 1;
        result.errors.push_back(e.what());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.writeTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

    m_isWriting = false;
    m_progress = 1.0;

    return result;
}

double LSDynaFileWriter::progress() const {
    return m_progress.load();
}

void LSDynaFileWriter::cancel() {
    m_cancelRequested = true;
}

bool LSDynaFileWriter::isWriting() const {
    return m_isWriting.load();
}

// ======================================================================
// Private Methods
// ======================================================================

void LSDynaFileWriter::writeHeader(std::ofstream& out, const LSDynaWriteOptions& options) {
    if (options.writeComments) {
        out << "$# LS-DYNA Keyword File\n";
        out << "$# Generated by KooMeshPrepost\n";
        out << "$# Format: " << (options.format == KeywordFormat::FREE ? "Free" : "Fixed") << "\n";
        out << "$#\n";
    }
    out << "*KEYWORD\n";
}

void LSDynaFileWriter::writeNodes(
    std::ofstream& out,
    const core::Mesh& mesh,
    const LSDynaWriteOptions& options,
    WriteContext& context,
    ProgressCallback progressCallback)
{
    const auto& nodes = mesh.nodes();
    if (nodes.empty()) {
        return;
    }

    if (options.writeComments) {
        out << "$#\n";
        out << "$# NODES\n";
        out << "$#\n";
    }

    out << "*NODE\n";

    if (options.writeComments) {
        if (options.format == KeywordFormat::FREE) {
            out << "$# nid, x, y, z\n";
        } else {
            out << "$#     nid               x               y               z\n";
        }
    }

    size_t nodeCount = nodes.size();
    size_t nodesProcessed = 0;

    for (const auto& [id, node] : nodes) {
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        if (options.format == KeywordFormat::FREE) {
            writeNodeFree(out, node, options);
        } else {
            writeNodeFixed(out, node, options);
        }

        ++context.nodesWritten;
        ++nodesProcessed;

        // Update progress periodically
        if (nodesProcessed % 1000 == 0) {
            double progress = 0.2 + (0.4 * nodesProcessed / nodeCount);
            std::ostringstream msg;
            msg << "Writing node " << nodesProcessed << "/" << nodeCount;
            updateProgress(progress, msg.str(), progressCallback);
        }
    }
}

void LSDynaFileWriter::writeElements(
    std::ofstream& out,
    const core::Mesh& mesh,
    const LSDynaWriteOptions& options,
    WriteContext& context,
    ProgressCallback progressCallback)
{
    const auto& elements = mesh.elements();
    if (elements.empty()) {
        return;
    }

    // Group elements by type
    std::map<core::ElementType, std::vector<const core::Element*>> elementsByType;

    for (const auto& [id, elemPtr] : elements) {
        core::ElementType type = elemPtr->type();
        elementsByType[type].push_back(elemPtr.get());
    }

    size_t totalElements = elements.size();
    size_t elementsProcessed = 0;

    // Write each element type group
    for (const auto& [type, elemList] : elementsByType) {
        if (elemList.empty()) continue;

        if (options.writeComments) {
            out << "$#\n";
            out << "$# " << core::MeshStatistics::Statistics::getElementTypeName(type) << " ELEMENTS\n";
            out << "$#\n";
        }

        std::string keyword = getElementKeyword(type);
        out << keyword << "\n";

        if (options.writeComments) {
            if (options.format == KeywordFormat::FREE) {
                out << "$# eid, pid, n1, n2, n3, n4, ...\n";
            } else {
                out << "$#     eid     pid      n1      n2      n3      n4      n5      n6      n7      n8\n";
            }
        }

        for (const auto* elem : elemList) {
            if (m_cancelRequested) {
                throw OperationCancelledException();
            }

            writeElement(out, *elem, options);
            ++context.elementsWritten;
            ++elementsProcessed;

            // Update progress periodically
            if (elementsProcessed % 1000 == 0) {
                double progress = 0.6 + (0.35 * elementsProcessed / totalElements);
                std::ostringstream msg;
                msg << "Writing element " << elementsProcessed << "/" << totalElements;
                updateProgress(progress, msg.str(), progressCallback);
            }
        }
    }
}

void LSDynaFileWriter::writeParts(
    std::ofstream& out,
    const core::Mesh& mesh,
    const LSDynaWriteOptions& options,
    WriteContext& context)
{
    const auto& parts = mesh.parts();
    if (parts.empty()) {
        return;
    }

    if (options.writeComments) {
        out << "$#\n";
        out << "$# PARTS\n";
        out << "$#\n";
    }

    for (const auto& [id, part] : parts) {
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        writePart(out, part, options);
        ++context.partsWritten;
    }
}

void LSDynaFileWriter::writeFooter(std::ofstream& out, const LSDynaWriteOptions& options) {
    if (options.writeComments) {
        out << "$#\n";
        out << "$# END OF FILE\n";
        out << "$#\n";
    }
    out << "*END\n";
}

void LSDynaFileWriter::writeNodeFixed(
    std::ofstream& out,
    const core::Node& node,
    const LSDynaWriteOptions& options)
{
    const auto& coords = node.coordinates();

    out << std::setw(8) << node.id()
        << std::setw(16) << std::setprecision(options.precision) << std::scientific << coords.x()
        << std::setw(16) << std::setprecision(options.precision) << std::scientific << coords.y()
        << std::setw(16) << std::setprecision(options.precision) << std::scientific << coords.z()
        << "\n";
}

void LSDynaFileWriter::writeNodeFree(
    std::ofstream& out,
    const core::Node& node,
    const LSDynaWriteOptions& options)
{
    const auto& coords = node.coordinates();

    out << node.id() << ","
        << std::setprecision(options.precision) << std::fixed << coords.x() << ","
        << std::setprecision(options.precision) << std::fixed << coords.y() << ","
        << std::setprecision(options.precision) << std::fixed << coords.z()
        << "\n";
}

void LSDynaFileWriter::writeElement(
    std::ofstream& out,
    const core::Element& element,
    const LSDynaWriteOptions& options)
{
    const auto& nodeIds = element.nodeIds();

    if (options.format == KeywordFormat::FREE) {
        // Free format
        out << element.id() << "," << element.partId();
        for (auto nid : nodeIds) {
            out << "," << nid;
        }
        out << "\n";
    } else {
        // Fixed format
        out << std::setw(8) << element.id()
            << std::setw(8) << element.partId();

        for (size_t i = 0; i < nodeIds.size() && i < 8; ++i) {
            out << std::setw(8) << nodeIds[i];
        }

        // If more than 8 nodes, write continuation line
        if (nodeIds.size() > 8) {
            out << "\n";
            for (size_t i = 8; i < nodeIds.size(); ++i) {
                out << std::setw(8) << nodeIds[i];
            }
        }

        out << "\n";
    }
}

void LSDynaFileWriter::writePart(
    std::ofstream& out,
    const core::Part& part,
    const LSDynaWriteOptions& options)
{
    out << "*PART\n";

    if (options.writeComments) {
        out << "$# " << part.name() << "\n";
    } else {
        out << part.name() << "\n";
    }

    if (options.format == KeywordFormat::FREE) {
        out << part.id() << "\n";
    } else {
        out << std::setw(10) << part.id() << "\n";
    }
}

std::string LSDynaFileWriter::getElementKeyword(core::ElementType type) const {
    using namespace core;

    switch (type) {
        case ElementType::TETRAHEDRON:
        case ElementType::HEXAHEDRON:
        case ElementType::PENTAHEDRON:
        case ElementType::PYRAMID:
            return "*ELEMENT_SOLID";

        case ElementType::TRIANGLE:
        case ElementType::QUADRILATERAL:
            return "*ELEMENT_SHELL";

        case ElementType::BEAM:
            return "*ELEMENT_BEAM";

        default:
            return "*ELEMENT_SOLID";  // Fallback
    }
}

bool LSDynaFileWriter::updateProgress(
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

void registerLSDynaWriter() {
    FileWriterFactory::registerWriter([]() {
        return std::make_unique<LSDynaFileWriter>();
    });
}

} // namespace io
} // namespace koomesh
