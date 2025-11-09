#pragma once

#include "io/IFileWriter.h"
#include <string>
#include <atomic>

namespace koomesh {
namespace io {

/**
 * @brief VTK (Visualization Toolkit) file format writer
 *
 * TODO: This is a stub implementation for future VTK format support.
 *
 * Planned support for:
 * - VTK Legacy format (.vtk) - ASCII and Binary
 * - VTK XML formats (.vtu, .vtp) - Unstructured Grid and PolyData
 * - Cell data and point data export
 * - Scalar and vector fields
 *
 * Implementation notes:
 * - VTK Legacy format is simpler to implement
 * - XML formats require proper XML generation
 * - Binary formats need proper endianness handling
 * - Support compression for XML formats (optional)
 *
 * Example output (VTK Legacy):
 * @code
 * # vtk DataFile Version 3.0
 * KooMeshPrepost Export
 * ASCII
 * DATASET UNSTRUCTURED_GRID
 * POINTS n float
 * ...
 * CELLS m size
 * ...
 * CELL_TYPES m
 * ...
 * @endcode
 *
 * @see https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf
 */
class VTKFileWriter : public IFileWriter {
public:
    VTKFileWriter();
    ~VTKFileWriter() override;

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

    double progress() const override;
    void cancel() override;
    bool isWriting() const override;

private:
    std::atomic<double> m_progress;
    std::atomic<bool> m_cancelRequested;
    std::atomic<bool> m_isWriting;
};

/**
 * @brief Register VTK writer with factory
 */
void registerVTKWriter();

} // namespace io
} // namespace koomesh
