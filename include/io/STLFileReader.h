#pragma once

#include "io/IFileReader.h"
#include <string>
#include <atomic>

namespace koomesh {
namespace io {

/**
 * @brief STL (STereoLithography) file format reader
 *
 * Supports both ASCII and binary STL formats.
 * STL files contain triangular mesh data (surface meshes only).
 *
 * Format details:
 * - ASCII: Human-readable text format
 * - Binary: Compact binary format (preferred for large files)
 *
 * Example usage:
 * @code
 * STLFileReader reader;
 * core::Mesh mesh;
 * auto result = reader.read("model.stl", mesh);
 * if (result.success) {
 *     std::cout << "Loaded " << result.trianglesRead << " triangles\n";
 * }
 * @endcode
 */
class STLFileReader : public IFileReader {
public:
    /**
     * @brief STL-specific read options
     */
    struct STLReadOptions : public ReadOptions {
        bool autoDetectFormat = true;  ///< Auto-detect ASCII vs binary
        bool mergeVertices = true;     ///< Merge duplicate vertices
        double mergeTolerance = 1e-6;  ///< Tolerance for vertex merging
        bool generateNormals = false;  ///< Recalculate normals (if false, use file normals)
    };

    STLFileReader();
    ~STLFileReader() override;

    // IFileReader interface
    bool canRead(const std::string& filename) const override;
    std::vector<std::string> supportedExtensions() const override;
    std::string formatName() const override;

    ReadResult read(
        const std::string& filename,
        core::Mesh& mesh,
        const ReadOptions& options = ReadOptions(),
        ProgressCallback progressCallback = nullptr
    ) override;

    /**
     * @brief Read STL file with STL-specific options
     */
    ReadResult read(
        const std::string& filename,
        core::Mesh& mesh,
        const STLReadOptions& options,
        ProgressCallback progressCallback = nullptr
    );

    double progress() const override;
    void cancel() override;
    bool isReading() const override;

private:
    /**
     * @brief Detect if file is ASCII or binary STL
     */
    bool isAsciiSTL(const std::string& filename) const;

    /**
     * @brief Read ASCII STL format
     */
    void readAscii(
        const std::string& filename,
        core::Mesh& mesh,
        const STLReadOptions& options,
        ProgressCallback progressCallback
    );

    /**
     * @brief Read binary STL format
     */
    void readBinary(
        const std::string& filename,
        core::Mesh& mesh,
        const STLReadOptions& options,
        ProgressCallback progressCallback
    );

    /**
     * @brief Merge duplicate vertices
     */
    void mergeVertices(
        core::Mesh& mesh,
        double tolerance
    );

    /**
     * @brief Update progress and check for cancellation
     */
    bool updateProgress(
        double progress,
        const std::string& message,
        ProgressCallback callback
    );

    std::atomic<double> m_progress;
    std::atomic<bool> m_cancelRequested;
    std::atomic<bool> m_isReading;
};

/**
 * @brief Register STL reader with factory
 */
void registerSTLReader();

} // namespace io
} // namespace koomesh
