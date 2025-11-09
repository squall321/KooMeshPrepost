#pragma once

#include "io/IFileReader.h"
#include <string>
#include <atomic>

namespace koomesh {
namespace io {

/**
 * @brief Nastran BDF (Bulk Data File) format reader
 *
 * TODO: This is a stub implementation for future Nastran format support.
 *
 * Nastran BDF format details:
 * - Free field format (comma-separated) or fixed field format
 * - Supports GRID (nodes), CTETRA, CHEXA, CTRIA3, CQUAD4 (elements)
 * - PSOLID, PSHELL (properties)
 * - MAT1 (materials)
 *
 * Example Nastran BDF:
 * @code
 * BEGIN BULK
 * GRID,1,,0.0,0.0,0.0
 * GRID,2,,1.0,0.0,0.0
 * CTETRA,100,1,1,2,3,4
 * ENDDATA
 * @endcode
 *
 * Implementation notes:
 * - Handle both free-field (comma) and fixed-field (8-char) formats
 * - Parse continuation lines (starting with +)
 * - Support include files
 * - Handle GRID, CTETRA, CHEXA, CTRIA3, CQUAD4, CPENTA, CPYRAM
 * - Optionally read property and material cards
 *
 * Field format:
 * - Small field: 8 characters per field
 * - Large field: 16 characters per field
 * - Free field: Comma or space separated
 *
 * @see https://web.mit.edu/calculix_v2.7/CalculiX/ccx_2.7/doc/ccx/node252.html
 */
class NastranFileReader : public IFileReader {
public:
    NastranFileReader();
    ~NastranFileReader() override;

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
 * @brief Register Nastran reader with factory
 */
void registerNastranReader();

} // namespace io
} // namespace koomesh
