/**
 * @file ColorMapper.cpp
 * @brief Color mapping implementation
 */

#include "visualization/ColorMapper.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkProperty.h>
#include <algorithm>
#include <sstream>

namespace koomesh {
namespace visualization {

ColorMapper::ColorMapper(int numColors)
    : m_colorScheme(ColorScheme::RAINBOW)
    , m_numberOfColors(numColors)
    , m_rangeMin(0.0)
    , m_rangeMax(1.0)
{
    m_lookupTable = vtkSmartPointer<vtkLookupTable>::New();
    m_lookupTable->SetNumberOfTableValues(numColors);
    buildColorTable();
}

ColorMapper::~ColorMapper() {
}

// ============================================================================
// Color Scheme Configuration
// ============================================================================

void ColorMapper::setColorScheme(ColorScheme scheme) {
    m_colorScheme = scheme;
    buildColorTable();
}

ColorScheme ColorMapper::getColorScheme() const {
    return m_colorScheme;
}

void ColorMapper::setNumberOfColors(int numColors) {
    m_numberOfColors = numColors;
    m_lookupTable->SetNumberOfTableValues(numColors);
    buildColorTable();
}

int ColorMapper::getNumberOfColors() const {
    return m_numberOfColors;
}

// ============================================================================
// Range Configuration
// ============================================================================

void ColorMapper::setRange(double min, double max) {
    m_rangeMin = min;
    m_rangeMax = max;
    m_lookupTable->SetRange(min, max);
}

std::array<double, 2> ColorMapper::getRange() const {
    return {m_rangeMin, m_rangeMax};
}

void ColorMapper::computeRangeFromData(const std::vector<double>& scalarValues) {
    if (scalarValues.empty()) {
        setRange(0.0, 1.0);
        return;
    }

    auto minmax = std::minmax_element(scalarValues.begin(), scalarValues.end());
    setRange(*minmax.first, *minmax.second);
}

// ============================================================================
// VTK Integration
// ============================================================================

vtkLookupTable* ColorMapper::getLookupTable() {
    return m_lookupTable;
}

const vtkLookupTable* ColorMapper::getLookupTable() const {
    return m_lookupTable;
}

void ColorMapper::applyToActor(
    vtkActor* actor,
    const std::vector<double>& scalarValues,
    bool useCellData
) {
    if (!actor || !actor->GetMapper()) {
        return;
    }

    auto mapper = actor->GetMapper();
    auto input = mapper->GetInput();

    if (!input) {
        return;
    }

    // Create scalar array
    vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetName("ScalarField");
    scalars->SetNumberOfTuples(scalarValues.size());

    for (size_t i = 0; i < scalarValues.size(); ++i) {
        scalars->SetValue(i, scalarValues[i]);
    }

    // Add to poly data
    if (useCellData) {
        input->GetCellData()->SetScalars(scalars);
        mapper->SetScalarModeToUseCellData();
    } else {
        input->GetPointData()->SetScalars(scalars);
        mapper->SetScalarModeToUsePointData();
    }

    // Set lookup table
    mapper->SetLookupTable(m_lookupTable);
    mapper->SetScalarRange(m_rangeMin, m_rangeMax);
    mapper->ScalarVisibilityOn();
}

void ColorMapper::applyToPolyData(
    vtkPolyData* polyData,
    const std::vector<double>& scalarValues,
    const std::string& arrayName,
    bool useCellData
) {
    if (!polyData) {
        return;
    }

    // Create scalar array
    vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetName(arrayName.c_str());
    scalars->SetNumberOfTuples(scalarValues.size());

    for (size_t i = 0; i < scalarValues.size(); ++i) {
        scalars->SetValue(i, scalarValues[i]);
    }

    // Add to poly data
    if (useCellData) {
        polyData->GetCellData()->SetScalars(scalars);
    } else {
        polyData->GetPointData()->SetScalars(scalars);
    }
}

// ============================================================================
// Color Mapping Query
// ============================================================================

std::array<double, 3> ColorMapper::mapValueToColor(double value) const {
    double rgb[3];
    m_lookupTable->GetColor(value, rgb);
    return {rgb[0], rgb[1], rgb[2]};
}

std::array<unsigned char, 3> ColorMapper::mapValueToColor255(double value) const {
    unsigned char rgb[3];
    m_lookupTable->GetColor(value, rgb);
    return {rgb[0], rgb[1], rgb[2]};
}

// ============================================================================
// Utility
// ============================================================================

void ColorMapper::buildTable() {
    buildColorTable();
}

void ColorMapper::reset() {
    m_colorScheme = ColorScheme::RAINBOW;
    m_numberOfColors = 256;
    m_rangeMin = 0.0;
    m_rangeMax = 1.0;

    m_lookupTable->SetNumberOfTableValues(m_numberOfColors);
    m_lookupTable->SetRange(m_rangeMin, m_rangeMax);
    buildColorTable();
}

std::string ColorMapper::getStatistics() const {
    std::ostringstream oss;
    oss << "Color Mapper Statistics:\n";
    oss << "  Color Scheme: ";

    switch (m_colorScheme) {
        case ColorScheme::RAINBOW: oss << "Rainbow"; break;
        case ColorScheme::GRAYSCALE: oss << "Grayscale"; break;
        case ColorScheme::HEAT: oss << "Heat"; break;
        case ColorScheme::COOL_WARM: oss << "Cool-Warm"; break;
        case ColorScheme::JET: oss << "Jet"; break;
        case ColorScheme::VIRIDIS: oss << "Viridis"; break;
        case ColorScheme::PLASMA: oss << "Plasma"; break;
        case ColorScheme::INFERNO: oss << "Inferno"; break;
        case ColorScheme::TURBO: oss << "Turbo"; break;
        default: oss << "Unknown"; break;
    }

    oss << "\n";
    oss << "  Number of Colors: " << m_numberOfColors << "\n";
    oss << "  Range: [" << m_rangeMin << ", " << m_rangeMax << "]\n";

    return oss.str();
}

// ============================================================================
// Private Methods
// ============================================================================

void ColorMapper::buildColorTable() {
    switch (m_colorScheme) {
        case ColorScheme::RAINBOW:
            setRainbowColors();
            break;
        case ColorScheme::GRAYSCALE:
            setGrayscaleColors();
            break;
        case ColorScheme::HEAT:
            setHeatColors();
            break;
        case ColorScheme::COOL_WARM:
            setCoolWarmColors();
            break;
        case ColorScheme::JET:
            setJetColors();
            break;
        case ColorScheme::VIRIDIS:
            setViridisColors();
            break;
        case ColorScheme::PLASMA:
            setPlasmaColors();
            break;
        case ColorScheme::INFERNO:
            setInfernoColors();
            break;
        case ColorScheme::TURBO:
            setTurboColors();
            break;
    }

    m_lookupTable->Build();
}

void ColorMapper::setRainbowColors() {
    m_lookupTable->SetHueRange(0.667, 0.0); // Blue to red
    m_lookupTable->SetSaturationRange(1.0, 1.0);
    m_lookupTable->SetValueRange(1.0, 1.0);
}

void ColorMapper::setGrayscaleColors() {
    m_lookupTable->SetHueRange(0.0, 0.0);
    m_lookupTable->SetSaturationRange(0.0, 0.0);
    m_lookupTable->SetValueRange(0.0, 1.0);
}

void ColorMapper::setHeatColors() {
    // Black -> Red -> Yellow -> White
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1);

        double r, g, b;
        if (t < 0.33) {
            // Black to red
            r = t / 0.33;
            g = 0.0;
            b = 0.0;
        } else if (t < 0.67) {
            // Red to yellow
            r = 1.0;
            g = (t - 0.33) / 0.34;
            b = 0.0;
        } else {
            // Yellow to white
            r = 1.0;
            g = 1.0;
            b = (t - 0.67) / 0.33;
        }

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setCoolWarmColors() {
    // Cool blue to warm red
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1);

        double r = t;
        double g = t * (1.0 - t) * 2.0; // Peaks at middle
        double b = 1.0 - t;

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setJetColors() {
    // Matlab jet colormap
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1);
        double r, g, b;

        if (t < 0.125) {
            r = 0.0;
            g = 0.0;
            b = 0.5 + 4.0 * t;
        } else if (t < 0.375) {
            r = 0.0;
            g = 4.0 * (t - 0.125);
            b = 1.0;
        } else if (t < 0.625) {
            r = 4.0 * (t - 0.375);
            g = 1.0;
            b = 1.0 - 4.0 * (t - 0.375);
        } else if (t < 0.875) {
            r = 1.0;
            g = 1.0 - 4.0 * (t - 0.625);
            b = 0.0;
        } else {
            r = 1.0 - 4.0 * (t - 0.875);
            g = 0.0;
            b = 0.0;
        }

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setViridisColors() {
    // Viridis perceptually uniform colormap (simplified)
    // Based on matplotlib's viridis
    const double viridisData[][3] = {
        {0.267004, 0.004874, 0.329415},
        {0.282623, 0.140926, 0.457517},
        {0.253935, 0.265254, 0.529983},
        {0.206756, 0.371758, 0.553117},
        {0.163625, 0.471133, 0.558148},
        {0.127568, 0.566949, 0.550556},
        {0.134692, 0.658636, 0.517649},
        {0.266941, 0.748751, 0.440573},
        {0.477504, 0.821444, 0.318195},
        {0.741388, 0.873449, 0.149561},
        {0.993248, 0.906157, 0.143936}
    };

    const int numSamples = 11;
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1) * (numSamples - 1);
        int idx = static_cast<int>(t);
        double frac = t - idx;

        if (idx >= numSamples - 1) {
            idx = numSamples - 2;
            frac = 1.0;
        }

        double r = viridisData[idx][0] * (1.0 - frac) + viridisData[idx + 1][0] * frac;
        double g = viridisData[idx][1] * (1.0 - frac) + viridisData[idx + 1][1] * frac;
        double b = viridisData[idx][2] * (1.0 - frac) + viridisData[idx + 1][2] * frac;

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setPlasmaColors() {
    // Plasma perceptually uniform colormap (simplified)
    const double plasmaData[][3] = {
        {0.050383, 0.029803, 0.527975},
        {0.287675, 0.010384, 0.627642},
        {0.490384, 0.007351, 0.658371},
        {0.652261, 0.129972, 0.615419},
        {0.783654, 0.283888, 0.523240},
        {0.881443, 0.451734, 0.411479},
        {0.951546, 0.628268, 0.300267},
        {0.987622, 0.809575, 0.218070},
        {0.987919, 0.991438, 0.216902}
    };

    const int numSamples = 9;
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1) * (numSamples - 1);
        int idx = static_cast<int>(t);
        double frac = t - idx;

        if (idx >= numSamples - 1) {
            idx = numSamples - 2;
            frac = 1.0;
        }

        double r = plasmaData[idx][0] * (1.0 - frac) + plasmaData[idx + 1][0] * frac;
        double g = plasmaData[idx][1] * (1.0 - frac) + plasmaData[idx + 1][1] * frac;
        double b = plasmaData[idx][2] * (1.0 - frac) + plasmaData[idx + 1][2] * frac;

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setInfernoColors() {
    // Inferno perceptually uniform colormap (simplified)
    const double infernoData[][3] = {
        {0.001462, 0.000466, 0.013866},
        {0.189503, 0.018803, 0.305202},
        {0.408558, 0.045257, 0.423292},
        {0.611423, 0.128054, 0.425581},
        {0.801663, 0.281412, 0.356454},
        {0.930695, 0.479267, 0.259636},
        {0.978422, 0.693448, 0.176089},
        {0.951585, 0.906735, 0.234611},
        {0.988362, 0.998364, 0.644924}
    };

    const int numSamples = 9;
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1) * (numSamples - 1);
        int idx = static_cast<int>(t);
        double frac = t - idx;

        if (idx >= numSamples - 1) {
            idx = numSamples - 2;
            frac = 1.0;
        }

        double r = infernoData[idx][0] * (1.0 - frac) + infernoData[idx + 1][0] * frac;
        double g = infernoData[idx][1] * (1.0 - frac) + infernoData[idx + 1][1] * frac;
        double b = infernoData[idx][2] * (1.0 - frac) + infernoData[idx + 1][2] * frac;

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

void ColorMapper::setTurboColors() {
    // Google Turbo colormap (simplified)
    const double turboData[][3] = {
        {0.18995, 0.07176, 0.23217},
        {0.13840, 0.41942, 0.69615},
        {0.15482, 0.63876, 0.69149},
        {0.38626, 0.80699, 0.50775},
        {0.69765, 0.86979, 0.24792},
        {0.94188, 0.75147, 0.13089},
        {0.98323, 0.51604, 0.13042},
        {0.87770, 0.30583, 0.18088},
        {0.70584, 0.01556, 0.15029}
    };

    const int numSamples = 9;
    for (int i = 0; i < m_numberOfColors; ++i) {
        double t = static_cast<double>(i) / (m_numberOfColors - 1) * (numSamples - 1);
        int idx = static_cast<int>(t);
        double frac = t - idx;

        if (idx >= numSamples - 1) {
            idx = numSamples - 2;
            frac = 1.0;
        }

        double r = turboData[idx][0] * (1.0 - frac) + turboData[idx + 1][0] * frac;
        double g = turboData[idx][1] * (1.0 - frac) + turboData[idx + 1][1] * frac;
        double b = turboData[idx][2] * (1.0 - frac) + turboData[idx + 1][2] * frac;

        m_lookupTable->SetTableValue(i, r, g, b);
    }
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
