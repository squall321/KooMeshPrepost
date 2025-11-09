#pragma once

#include "io/IFileReader.h"
#include <string>
#include <atomic>

namespace koomesh {
namespace io {

/**
 * @brief VTK (Visualization Toolkit) file format reader
 *
 * TODO: This is a stub implementation for future VTK format support.
 *
 * Planned support for:
 * - VTK Legacy format (.vtk)
 * - VTK XML formats (.vtu, .vtp, .vts, .vti)
 * - Unstructured grids
 * - Polydata
 * - Structured/Rectilinear grids
 *
 * Implementation notes:
 * - Consider using VTK library directly (vtkDataSetReader, vtkXMLReader)
 * - Or implement custom parser for VTK legacy format
 * - Handle both ASCII and binary formats
 * - Support cell data and point data
 *
 * Example VTK Legacy format:
 * @code
 * # vtk DataFile Version 3.0
 * Mesh Data
 * ASCII
 * DATASET UNSTRUCTURED_GRID
 * POINTS 8 float
 * 0.0 0.0 0.0
 * 1.0 0.0 0.0
 * ...
 * CELLS 1 9
 * 8 0 1 2 3 4 5 6 7
 * CELL_TYPES 1
 * 12
 * @endcode
 *
 * @see https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf
 */
class VTKFileReader : public IFileReader {
public:
    VTKFileReader();
    ~VTKFileReader() override;

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

    double progress() const override;
    void cancel() override;
    bool isReading() const override;

private:
    std::atomic<double> m_progress;
    std::atomic<bool> m_cancelRequested;
    std::atomic<bool> m_isReading;
};

/**
 * @brief Register VTK reader with factory
 */
void registerVTKReader();

} // namespace io
} // namespace koomesh
