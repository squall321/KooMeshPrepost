/**
 * @file ColorMapper.h
 * @brief Color mapping for scalar field visualization
 */

#pragma once

#include "visualization/VisualizationOptions.h"
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkScalarsToColors.h>
#include <vtkActor.h>
#include <vtkMapper.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#endif

namespace koomesh {
namespace visualization {

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Color mapper for scalar field visualization
 *
 * Provides color mapping functionality for visualizing scalar fields:
 * - Multiple color schemes (rainbow, heat, viridis, etc.)
 * - Automatic or manual range configuration
 * - Discrete or continuous coloring
 * - Integration with VTK actors and mappers
 * - Color interpolation and table generation
 */
class ColorMapper {
public:
    /**
     * @brief Constructor
     * @param numColors Number of colors in lookup table (default 256)
     */
    explicit ColorMapper(int numColors = 256);

    /**
     * @brief Destructor
     */
    ~ColorMapper();

    // ========================================================================
    // Color Scheme Configuration
    // ========================================================================

    /**
     * @brief Set color scheme
     * @param scheme Color scheme to use
     */
    void setColorScheme(ColorScheme scheme);

    /**
     * @brief Get current color scheme
     * @return Current color scheme
     */
    ColorScheme getColorScheme() const;

    /**
     * @brief Set number of colors (for discrete coloring)
     * @param numColors Number of discrete colors (0 = continuous)
     */
    void setNumberOfColors(int numColors);

    /**
     * @brief Get number of colors
     * @return Number of colors in lookup table
     */
    int getNumberOfColors() const;

    // ========================================================================
    // Range Configuration
    // ========================================================================

    /**
     * @brief Set scalar value range
     * @param min Minimum scalar value
     * @param max Maximum scalar value
     */
    void setRange(double min, double max);

    /**
     * @brief Get scalar value range
     * @return Range as (min, max) pair
     */
    std::array<double, 2> getRange() const;

    /**
     * @brief Compute range from scalar values
     * @param scalarValues Scalar values to analyze
     */
    void computeRangeFromData(const std::vector<double>& scalarValues);

    // ========================================================================
    // VTK Integration
    // ========================================================================

    /**
     * @brief Get VTK lookup table
     * @return VTK lookup table pointer
     */
    vtkLookupTable* getLookupTable();

    /**
     * @brief Get VTK lookup table (const)
     * @return VTK lookup table pointer
     */
    const vtkLookupTable* getLookupTable() const;

    /**
     * @brief Apply color mapping to actor
     * @param actor VTK actor to apply coloring to
     * @param scalarValues Scalar values for each vertex/cell
     * @param useCellData Use cell data (true) or point data (false)
     */
    void applyToActor(
        vtkActor* actor,
        const std::vector<double>& scalarValues,
        bool useCellData = false
    );

    /**
     * @brief Apply color mapping to poly data
     * @param polyData VTK poly data
     * @param scalarValues Scalar values
     * @param arrayName Name for scalar array
     * @param useCellData Use cell data (true) or point data (false)
     */
    void applyToPolyData(
        vtkPolyData* polyData,
        const std::vector<double>& scalarValues,
        const std::string& arrayName,
        bool useCellData = false
    );

    // ========================================================================
    // Color Mapping Query
    // ========================================================================

    /**
     * @brief Map scalar value to RGB color
     * @param value Scalar value
     * @return RGB color (0-1 range)
     */
    std::array<double, 3> mapValueToColor(double value) const;

    /**
     * @brief Map scalar value to RGB color (0-255 range)
     * @param value Scalar value
     * @return RGB color (0-255 range)
     */
    std::array<unsigned char, 3> mapValueToColor255(double value) const;

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Build lookup table from current settings
     */
    void buildTable();

    /**
     * @brief Reset to default settings
     */
    void reset();

    /**
     * @brief Get table statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

private:
    /**
     * @brief Build color table based on scheme
     */
    void buildColorTable();

    /**
     * @brief Set rainbow color scheme
     */
    void setRainbowColors();

    /**
     * @brief Set grayscale color scheme
     */
    void setGrayscaleColors();

    /**
     * @brief Set heat color scheme
     */
    void setHeatColors();

    /**
     * @brief Set cool-warm color scheme
     */
    void setCoolWarmColors();

    /**
     * @brief Set jet color scheme
     */
    void setJetColors();

    /**
     * @brief Set viridis color scheme
     */
    void setViridisColors();

    /**
     * @brief Set plasma color scheme
     */
    void setPlasmaColors();

    /**
     * @brief Set inferno color scheme
     */
    void setInfernoColors();

    /**
     * @brief Set turbo color scheme
     */
    void setTurboColors();

    // VTK lookup table
    vtkSmartPointer<vtkLookupTable> m_lookupTable;

    // Configuration
    ColorScheme m_colorScheme;
    int m_numberOfColors;
    double m_rangeMin;
    double m_rangeMax;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class ColorMapper {
public:
    explicit ColorMapper(int numColors = 256)
        : m_colorScheme(ColorScheme::RAINBOW)
        , m_numberOfColors(numColors)
        , m_rangeMin(0.0)
        , m_rangeMax(1.0)
    {}

    ~ColorMapper() {}

    void setColorScheme(ColorScheme scheme) { m_colorScheme = scheme; }
    ColorScheme getColorScheme() const { return m_colorScheme; }

    void setNumberOfColors(int numColors) { m_numberOfColors = numColors; }
    int getNumberOfColors() const { return m_numberOfColors; }

    void setRange(double min, double max) {
        m_rangeMin = min;
        m_rangeMax = max;
    }

    std::array<double, 2> getRange() const {
        return {m_rangeMin, m_rangeMax};
    }

    void computeRangeFromData(const std::vector<double>& scalarValues) {
        if (scalarValues.empty()) {
            m_rangeMin = 0.0;
            m_rangeMax = 1.0;
            return;
        }

        m_rangeMin = scalarValues[0];
        m_rangeMax = scalarValues[0];

        for (double value : scalarValues) {
            if (value < m_rangeMin) m_rangeMin = value;
            if (value > m_rangeMax) m_rangeMax = value;
        }
    }

    void* getLookupTable() { return nullptr; }
    const void* getLookupTable() const { return nullptr; }

    void applyToActor(void*, const std::vector<double>&, bool = false) {}
    void applyToPolyData(void*, const std::vector<double>&, const std::string&, bool = false) {}

    std::array<double, 3> mapValueToColor(double value) const {
        // Simple linear interpolation for stub
        double t = (m_rangeMax > m_rangeMin) ?
                   (value - m_rangeMin) / (m_rangeMax - m_rangeMin) : 0.0;
        t = std::max(0.0, std::min(1.0, t)); // Clamp to [0, 1]

        // Simple rainbow gradient
        if (t < 0.25) {
            return {0.0, 4.0 * t, 1.0};
        } else if (t < 0.5) {
            return {0.0, 1.0, 2.0 - 4.0 * t};
        } else if (t < 0.75) {
            return {4.0 * t - 2.0, 1.0, 0.0};
        } else {
            return {1.0, 4.0 - 4.0 * t, 0.0};
        }
    }

    std::array<unsigned char, 3> mapValueToColor255(double value) const {
        auto color = mapValueToColor(value);
        return {
            static_cast<unsigned char>(color[0] * 255),
            static_cast<unsigned char>(color[1] * 255),
            static_cast<unsigned char>(color[2] * 255)
        };
    }

    void buildTable() {}

    void reset() {
        m_colorScheme = ColorScheme::RAINBOW;
        m_numberOfColors = 256;
        m_rangeMin = 0.0;
        m_rangeMax = 1.0;
    }

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Color Mapper Statistics (VTK not available):\n";
        oss << "  Color Scheme: " << getColorSchemeString(m_colorScheme) << "\n";
        oss << "  Number of Colors: " << m_numberOfColors << "\n";
        oss << "  Range: [" << m_rangeMin << ", " << m_rangeMax << "]\n";
        return oss.str();
    }

private:
    const char* getColorSchemeString(ColorScheme scheme) const {
        switch (scheme) {
            case ColorScheme::RAINBOW: return "Rainbow";
            case ColorScheme::GRAYSCALE: return "Grayscale";
            case ColorScheme::HEAT: return "Heat";
            case ColorScheme::COOL_WARM: return "Cool-Warm";
            case ColorScheme::JET: return "Jet";
            case ColorScheme::VIRIDIS: return "Viridis";
            case ColorScheme::PLASMA: return "Plasma";
            case ColorScheme::INFERNO: return "Inferno";
            case ColorScheme::TURBO: return "Turbo";
            default: return "Unknown";
        }
    }

    ColorScheme m_colorScheme;
    int m_numberOfColors;
    double m_rangeMin;
    double m_rangeMax;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
