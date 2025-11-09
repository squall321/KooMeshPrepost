#pragma once

#include "io/IFileWriter.h"
#include "io/IFileReader.h"  // For ProgressCallback definition
#include <string>
#include <atomic>

namespace koomesh {
namespace io {

/**
 * @brief STL (STereoLithography) file format writer
 *
 * Supports both ASCII and binary STL formats.
 * Exports triangular surface meshes only.
 *
 * Note: STL format limitations:
 * - Only supports triangular elements
 * - No material/part information
 * - No color information (in standard STL)
 * - Vertices are duplicated (not indexed)
 *
 * Example usage:
 * @code
 * STLFileWriter writer;
 * STLFileWriter::STLWriteOptions options;
 * options.format = STLFormat::BINARY;
 * auto result = writer.write("output.stl", mesh, options);
 * @endcode
 */
class STLFileWriter : public IFileWriter {
public:
    /**
     * @brief STL format type
     */
    enum class STLFormat {
        ASCII,   ///< Human-readable text format
        BINARY   ///< Compact binary format (recommended)
    };

    /**
     * @brief STL-specific write options
     */
    struct STLWriteOptions : public WriteOptions {
        STLFormat format = STLFormat::BINARY;  ///< Output format
        std::string solidName = "mesh";        ///< Solid name (ASCII only)
        bool exportOnlyTriangles = true;       ///< Only export triangular elements
        bool triangulateQuads = true;          ///< Convert quads to triangles
    };

    STLFileWriter();
    ~STLFileWriter() override;

    // IFileWriter interface
    bool canWrite(const std::string& filename) const override;
    std::vector<std::string> supportedExtensions() const override;
    std::string formatName() const override;

    WriteResult write(
        const std::string& filename,
        const core::Mesh& mesh,
        const WriteOptions& options = WriteOptions(),
        ProgressCallback progressCallback = nullptr
    ) override;

    /**
     * @brief Write STL file with STL-specific options
     */
    WriteResult write(
        const std::string& filename,
        const core::Mesh& mesh,
        const STLWriteOptions& options,
        ProgressCallback progressCallback = nullptr
    );

    double progress() const override;
    void cancel() override;
    bool isWriting() const override;

private:
    /**
     * @brief Write ASCII STL format
     */
    void writeAscii(
        const std::string& filename,
        const core::Mesh& mesh,
        const STLWriteOptions& options,
        ProgressCallback progressCallback
    );

    /**
     * @brief Write binary STL format
     */
    void writeBinary(
        const std::string& filename,
        const core::Mesh& mesh,
        const STLWriteOptions& options,
        ProgressCallback progressCallback
    );

    /**
     * @brief Calculate triangle normal
     */
    Eigen::Vector3d calculateNormal(
        const Eigen::Vector3d& v1,
        const Eigen::Vector3d& v2,
        const Eigen::Vector3d& v3
    ) const;

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
    std::atomic<bool> m_isWriting;
};

/**
 * @brief Register STL writer with factory
 */
void registerSTLWriter();

} // namespace io
} // namespace koomesh
