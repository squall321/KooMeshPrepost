#include "io/FileFormatDetector.h"
#include "io/FileIOException.h"
#include <fstream>
#include <algorithm>
#include <cctype>

namespace koomesh {
namespace io {

// ======================================================================
// Public Methods
// ======================================================================

FileFormat FileFormatDetector::detect(const std::string& filepath) {
    // First try extension-based detection (fast)
    FileFormat format = detectByExtension(filepath);

    // If ambiguous or unknown, try content-based detection
    if (format == FileFormat::UNKNOWN) {
        format = detectByContent(filepath);
    }

    // Special case: .dat can be Nastran or other formats
    // Verify with content if we got NASTRAN from extension
    if (format == FileFormat::NASTRAN) {
        std::string ext = getExtension(filepath);
        if (ext == ".dat") {
            // .dat is ambiguous, verify with content
            FileFormat contentFormat = detectByContent(filepath);
            if (contentFormat != FileFormat::UNKNOWN) {
                format = contentFormat;
            }
        }
    }

    return format;
}

FileFormat FileFormatDetector::detectByExtension(const std::string& filepath) {
    std::string ext = getExtension(filepath);

    // LS-DYNA extensions
    if (ext == ".k" || ext == ".key" || ext == ".dyn") {
        return FileFormat::LSDYNA;
    }

    // STL extensions
    if (ext == ".stl") {
        return FileFormat::STL;
    }

    // VTK extensions
    if (ext == ".vtk" || ext == ".vtu" || ext == ".vtp" ||
        ext == ".vts" || ext == ".vti") {
        return FileFormat::VTK;
    }

    // Nastran extensions (note: .dat is ambiguous)
    if (ext == ".bdf" || ext == ".nas" || ext == ".dat") {
        return FileFormat::NASTRAN;
    }

    return FileFormat::UNKNOWN;
}

FileFormat FileFormatDetector::detectByContent(const std::string& filepath) {
    if (!fileExists(filepath)) {
        return FileFormat::UNKNOWN;
    }

    try {
        // Read first 512 bytes of file
        std::string header = readFileHeader(filepath, 512);

        if (header.empty()) {
            return FileFormat::UNKNOWN;
        }

        // Convert to lowercase for case-insensitive matching
        std::string headerLower = header;
        std::transform(headerLower.begin(), headerLower.end(),
                      headerLower.begin(), ::tolower);

        // Check for STL ASCII (starts with "solid")
        if (header.substr(0, 5) == "solid") {
            return FileFormat::STL;
        }

        // Check for STL Binary (first 80 bytes should NOT start with "solid")
        // STL binary has 80-byte header + 4-byte triangle count
        if (header.size() >= 84 && header.substr(0, 5) != "solid") {
            // Additional check: byte 80-83 should be triangle count (reasonable value)
            return FileFormat::STL;
        }

        // Check for VTK Legacy format
        if (headerLower.find("# vtk datafile version") != std::string::npos) {
            return FileFormat::VTK;
        }

        // Check for VTK XML format
        if (headerLower.find("<?xml") != std::string::npos &&
            headerLower.find("vtkfile") != std::string::npos) {
            return FileFormat::VTK;
        }

        // Check for LS-DYNA format
        if (headerLower.find("*keyword") != std::string::npos ||
            headerLower.find("*node") != std::string::npos ||
            headerLower.find("*element") != std::string::npos ||
            headerLower.find("*part") != std::string::npos) {
            return FileFormat::LSDYNA;
        }

        // Check for Nastran format
        if (headerLower.find("begin bulk") != std::string::npos ||
            headerLower.find("grid") == 0 ||
            headerLower.find("ctetra") != std::string::npos ||
            headerLower.find("chexa") != std::string::npos ||
            headerLower.find("ctria3") != std::string::npos ||
            headerLower.find("cquad4") != std::string::npos) {
            return FileFormat::NASTRAN;
        }

    } catch (const std::exception&) {
        return FileFormat::UNKNOWN;
    }

    return FileFormat::UNKNOWN;
}

std::string FileFormatDetector::formatName(FileFormat format) {
    switch (format) {
        case FileFormat::LSDYNA:
            return "LS-DYNA Keyword";
        case FileFormat::STL:
            return "STereoLithography (STL)";
        case FileFormat::VTK:
            return "VTK (Visualization Toolkit)";
        case FileFormat::NASTRAN:
            return "Nastran BDF";
        case FileFormat::UNKNOWN:
            return "Unknown";
        default:
            return "Unknown";
    }
}

std::vector<std::string> FileFormatDetector::formatExtensions(FileFormat format) {
    switch (format) {
        case FileFormat::LSDYNA:
            return {".k", ".key", ".dyn"};
        case FileFormat::STL:
            return {".stl"};
        case FileFormat::VTK:
            return {".vtk", ".vtu", ".vtp", ".vts", ".vti"};
        case FileFormat::NASTRAN:
            return {".bdf", ".nas", ".dat"};
        case FileFormat::UNKNOWN:
        default:
            return {};
    }
}

bool FileFormatDetector::fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

// ======================================================================
// Private Methods
// ======================================================================

std::string FileFormatDetector::getExtension(const std::string& filepath) {
    // Find last dot
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }

    // Extract extension and convert to lowercase
    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext;
}

std::string FileFormatDetector::readFileHeader(const std::string& filepath, size_t numBytes) {
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        throw FileOpenException(filepath, "Cannot open file for format detection");
    }

    // Determine actual file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read minimum of requested bytes and file size
    size_t bytesToRead = std::min(numBytes, fileSize);

    std::string header(bytesToRead, '\0');
    file.read(&header[0], bytesToRead);

    return header;
}

} // namespace io
} // namespace koomesh
