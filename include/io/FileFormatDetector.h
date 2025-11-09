#pragma once

#include <string>
#include <vector>

namespace koomesh {
namespace io {

/**
 * @brief File format enumeration
 *
 * Supported file formats for mesh data import/export
 */
enum class FileFormat {
    LSDYNA,   ///< LS-DYNA Keyword format (.k, .key, .dyn)
    STL,      ///< STereoLithography format (.stl)
    VTK,      ///< VTK format (.vtk, .vtu, .vtp, .vts, .vti)
    NASTRAN,  ///< Nastran BDF format (.bdf, .nas, .dat)
    UNKNOWN   ///< Unknown or unsupported format
};

/**
 * @brief File format detection utility
 *
 * Detects file format based on:
 * - File extension
 * - File content (magic bytes/header)
 *
 * Example usage:
 * @code
 * FileFormat format = FileFormatDetector::detect("model.k");
 * if (format == FileFormat::LSDYNA) {
 *     // Use LS-DYNA reader
 * }
 * @endcode
 *
 * Detection strategy:
 * 1. Check file extension first (fast)
 * 2. If ambiguous, examine file content (slower but accurate)
 *
 * Magic bytes/signatures:
 * - STL Binary: First 80 bytes NOT starting with "solid"
 * - STL ASCII: Starts with "solid"
 * - VTK Legacy: Starts with "# vtk DataFile Version"
 * - VTK XML: Starts with "<?xml" and contains "VTKFile"
 * - LS-DYNA: Contains "*KEYWORD" or "*NODE" or "*ELEMENT"
 * - Nastran: Contains "BEGIN BULK" or starts with "GRID" or "CTETRA"
 */
class FileFormatDetector {
public:
    /**
     * @brief Detect file format
     *
     * @param filepath Path to the file
     * @return Detected file format
     */
    static FileFormat detect(const std::string& filepath);

    /**
     * @brief Detect format by extension only
     *
     * Fast detection based solely on file extension.
     * May be ambiguous for extensions like .dat
     *
     * @param filepath Path to the file
     * @return Detected file format or UNKNOWN
     */
    static FileFormat detectByExtension(const std::string& filepath);

    /**
     * @brief Detect format by content
     *
     * Slower but more accurate detection based on file content.
     * Examines magic bytes and file headers.
     *
     * @param filepath Path to the file
     * @return Detected file format or UNKNOWN
     */
    static FileFormat detectByContent(const std::string& filepath);

    /**
     * @brief Get human-readable format name
     *
     * @param format File format enum
     * @return Format name as string
     */
    static std::string formatName(FileFormat format);

    /**
     * @brief Get typical file extensions for a format
     *
     * @param format File format enum
     * @return Vector of typical extensions (with dot)
     */
    static std::vector<std::string> formatExtensions(FileFormat format);

    /**
     * @brief Check if file exists and is readable
     *
     * @param filepath Path to the file
     * @return true if file exists and can be opened
     */
    static bool fileExists(const std::string& filepath);

private:
    /**
     * @brief Extract file extension (lowercase)
     *
     * @param filepath Path to the file
     * @return Extension with dot (e.g., ".k") or empty string
     */
    static std::string getExtension(const std::string& filepath);

    /**
     * @brief Read first N bytes of file
     *
     * @param filepath Path to the file
     * @param numBytes Number of bytes to read
     * @return File header content
     */
    static std::string readFileHeader(const std::string& filepath, size_t numBytes = 512);
};

} // namespace io
} // namespace koomesh
