/**
 * @file AnnotationManager.cpp
 * @brief Annotation management implementation
 */

#include "visualization/AnnotationManager.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkTextProperty.h>
#include <vtkProperty2D.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkTubeFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkConeSource.h>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace koomesh {
namespace visualization {

AnnotationManager::AnnotationManager(vtkRenderer* renderer)
    : m_renderer(renderer)
    , m_nextId(0)
{
}

AnnotationManager::~AnnotationManager() {
    clear();
}

// ============================================================================
// Measurement Annotations
// ============================================================================

int AnnotationManager::addDistanceMeasurement(
    const Eigen::Vector3d& p1,
    const Eigen::Vector3d& p2,
    const std::string& label
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::DISTANCE_MEASUREMENT;
    annotation->points = {p1, p2};
    annotation->style = m_defaultStyle;

    // Calculate distance
    double distance = calculateDistance(p1, p2);

    // Set text (custom label or distance value)
    if (label.empty()) {
        annotation->text = formatNumber(distance) + " units";
    } else {
        annotation->text = label;
    }

    createDistanceMeasurement(annotation.get());

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addAngleMeasurement(
    const Eigen::Vector3d& p1,
    const Eigen::Vector3d& p2,
    const Eigen::Vector3d& p3,
    const std::string& label
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::ANGLE_MEASUREMENT;
    annotation->points = {p1, p2, p3};
    annotation->style = m_defaultStyle;

    // Calculate angle
    double angle = calculateAngle(p1, p2, p3);

    // Set text
    if (label.empty()) {
        annotation->text = formatNumber(angle) + "°";
    } else {
        annotation->text = label;
    }

    createAngleMeasurement(annotation.get());

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addDimensionLine(
    const Eigen::Vector3d& p1,
    const Eigen::Vector3d& p2,
    double offset
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::DIMENSION;
    annotation->points = {p1, p2};
    annotation->style = m_defaultStyle;

    double distance = calculateDistance(p1, p2);
    annotation->text = formatNumber(distance);

    // Create dimension line with arrows (simplified version)
    createDistanceMeasurement(annotation.get());

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

// ============================================================================
// Label Annotations
// ============================================================================

int AnnotationManager::addTextLabel(
    const std::string& text,
    const Eigen::Vector3d& position,
    bool attachedToCamera
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::TEXT_LABEL;
    annotation->text = text;
    annotation->points = {position};
    annotation->style = m_defaultStyle;

    createTextLabel(annotation.get(), attachedToCamera);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addScreenLabel(
    const std::string& text,
    double screenX,
    double screenY
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::TEXT_LABEL;
    annotation->text = text;
    annotation->style = m_defaultStyle;

    // Create 2D text actor
    auto textActor = vtkSmartPointer<vtkTextActor>::New();
    textActor->SetInput(text.c_str());
    textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    textActor->SetPosition(screenX, screenY);

    // Apply style
    auto textProp = textActor->GetTextProperty();
    textProp->SetFontSize(static_cast<int>(m_defaultStyle.fontSize));
    textProp->SetColor(
        m_defaultStyle.textColor.x(),
        m_defaultStyle.textColor.y(),
        m_defaultStyle.textColor.z()
    );
    textProp->SetBold(m_defaultStyle.textBold);
    textProp->SetItalic(m_defaultStyle.textItalic);

    annotation->actors.push_back(textActor);
    m_renderer->AddActor2D(textActor);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addCaption(
    const std::string& text,
    const Eigen::Vector3d& position,
    const Eigen::Vector3d& offset
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::TEXT_LABEL;
    annotation->text = text;
    annotation->points = {position, position + offset};
    annotation->style = m_defaultStyle;

    // Create caption with leader line
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption(text.c_str());
    caption->SetAttachmentPoint(position.x(), position.y(), position.z());
    caption->BorderOff();
    caption->LeaderOn();

    auto textProp = caption->GetCaptionTextProperty();
    textProp->SetFontSize(static_cast<int>(m_defaultStyle.fontSize));
    textProp->SetColor(
        m_defaultStyle.textColor.x(),
        m_defaultStyle.textColor.y(),
        m_defaultStyle.textColor.z()
    );

    annotation->actors.push_back(caption);
    m_renderer->AddActor2D(caption);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

// ============================================================================
// Axes and Reference
// ============================================================================

int AnnotationManager::addCoordinateAxes(bool interactive) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::COORDINATE_AXES;
    annotation->style = m_defaultStyle;

    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(1.0, 1.0, 1.0);
    axes->SetShaftType(vtkAxesActor::CYLINDER_SHAFT);
    axes->SetCylinderRadius(0.02);

    annotation->actors.push_back(axes);
    m_renderer->AddActor(axes);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addSimpleAxes(double length) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::COORDINATE_AXES;
    annotation->style = m_defaultStyle;

    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(length, length, length);

    annotation->actors.push_back(axes);
    m_renderer->AddActor(axes);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

int AnnotationManager::addOrientationMarker(int cornerPosition) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::COORDINATE_AXES;
    annotation->style = m_defaultStyle;

    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    annotation->actors.push_back(axes);

    // Note: OrientationMarkerWidget requires render window interactor
    // This is a simplified version
    m_renderer->AddActor(axes);

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

// ============================================================================
// Color Legend
// ============================================================================

int AnnotationManager::addScalarBar(
    const std::string& title,
    vtkLookupTable* lookupTable,
    int position
) {
    // Default positions and sizes based on position parameter
    double width = 0.1;
    double height = 0.8;
    double posX = 0.9;
    double posY = 0.1;

    switch (position) {
        case 0: // Right
            posX = 0.9;
            posY = 0.1;
            break;
        case 1: // Left
            posX = 0.05;
            posY = 0.1;
            break;
        case 2: // Top
            width = 0.8;
            height = 0.1;
            posX = 0.1;
            posY = 0.9;
            break;
        case 3: // Bottom
            width = 0.8;
            height = 0.1;
            posX = 0.1;
            posY = 0.05;
            break;
    }

    return addCustomScalarBar(title, lookupTable, width, height, posX, posY);
}

int AnnotationManager::addCustomScalarBar(
    const std::string& title,
    vtkLookupTable* lookupTable,
    double width,
    double height,
    double posX,
    double posY
) {
    auto annotation = std::make_unique<Annotation>();
    annotation->id = m_nextId++;
    annotation->type = AnnotationType::SCALAR_BAR;
    annotation->text = title;
    annotation->style = m_defaultStyle;

    createScalarBar(annotation.get(), lookupTable);

    // Set position and size
    if (!annotation->actors.empty()) {
        auto scalarBar = vtkScalarBarActor::SafeDownCast(annotation->actors[0]);
        if (scalarBar) {
            scalarBar->SetPosition(posX, posY);
            scalarBar->SetWidth(width);
            scalarBar->SetHeight(height);
        }
    }

    int id = annotation->id;
    m_annotations.push_back(std::move(annotation));
    return id;
}

// ============================================================================
// Annotation Management
// ============================================================================

bool AnnotationManager::removeAnnotation(int id) {
    for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
        if ((*it)->id == id) {
            // Remove all actors from renderer
            for (auto& actor : (*it)->actors) {
                if (auto actor3D = vtkActor::SafeDownCast(actor)) {
                    m_renderer->RemoveActor(actor3D);
                } else if (auto actor2D = vtkActor2D::SafeDownCast(actor)) {
                    m_renderer->RemoveActor2D(actor2D);
                }
            }

            m_annotations.erase(it);
            return true;
        }
    }
    return false;
}

void AnnotationManager::clear() {
    while (!m_annotations.empty()) {
        removeAnnotation(m_annotations[0]->id);
    }
}

void AnnotationManager::clearType(AnnotationType type) {
    std::vector<int> toRemove;
    for (const auto& annotation : m_annotations) {
        if (annotation->type == type) {
            toRemove.push_back(annotation->id);
        }
    }

    for (int id : toRemove) {
        removeAnnotation(id);
    }
}

int AnnotationManager::getAnnotationCount() const {
    return static_cast<int>(m_annotations.size());
}

bool AnnotationManager::hasAnnotation(int id) const {
    for (const auto& annotation : m_annotations) {
        if (annotation->id == id) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Visibility Control
// ============================================================================

void AnnotationManager::setVisible(int id, bool visible) {
    for (auto& annotation : m_annotations) {
        if (annotation->id == id) {
            annotation->visible = visible;
            for (auto& actor : annotation->actors) {
                if (auto actor3D = vtkActor::SafeDownCast(actor)) {
                    actor3D->SetVisibility(visible);
                } else if (auto actor2D = vtkActor2D::SafeDownCast(actor)) {
                    actor2D->SetVisibility(visible);
                }
            }
            return;
        }
    }
}

void AnnotationManager::setAllVisible(bool visible) {
    for (auto& annotation : m_annotations) {
        setVisible(annotation->id, visible);
    }
}

void AnnotationManager::setTypeVisible(AnnotationType type, bool visible) {
    for (auto& annotation : m_annotations) {
        if (annotation->type == type) {
            setVisible(annotation->id, visible);
        }
    }
}

// ============================================================================
// Style Configuration
// ============================================================================

void AnnotationManager::setDefaultStyle(const AnnotationStyle& style) {
    m_defaultStyle = style;
}

const AnnotationStyle& AnnotationManager::getDefaultStyle() const {
    return m_defaultStyle;
}

void AnnotationManager::setStyle(int id, const AnnotationStyle& style) {
    for (auto& annotation : m_annotations) {
        if (annotation->id == id) {
            annotation->style = style;
            applyStyle(annotation.get());
            return;
        }
    }
}

void AnnotationManager::updateText(int id, const std::string& text) {
    for (auto& annotation : m_annotations) {
        if (annotation->id == id) {
            annotation->text = text;

            // Update text in actors
            for (auto& actor : annotation->actors) {
                if (auto textActor = vtkTextActor::SafeDownCast(actor)) {
                    textActor->SetInput(text.c_str());
                } else if (auto caption = vtkCaptionActor2D::SafeDownCast(actor)) {
                    caption->SetCaption(text.c_str());
                }
            }
            return;
        }
    }
}

// ============================================================================
// Utility
// ============================================================================

void AnnotationManager::update() {
    // Update camera-facing actors
    if (!m_renderer || !m_renderer->GetActiveCamera()) {
        return;
    }

    auto camera = m_renderer->GetActiveCamera();

    for (auto& annotation : m_annotations) {
        for (auto& actor : annotation->actors) {
            if (auto follower = vtkFollower::SafeDownCast(actor)) {
                follower->SetCamera(camera);
            }
        }
    }
}

const Annotation* AnnotationManager::getAnnotation(int id) const {
    for (const auto& annotation : m_annotations) {
        if (annotation->id == id) {
            return annotation.get();
        }
    }
    return nullptr;
}

std::string AnnotationManager::getStatistics() const {
    std::ostringstream oss;
    oss << "Annotation Manager Statistics:\n";
    oss << "  Total Annotations: " << m_annotations.size() << "\n";

    int measurements = 0, labels = 0, axes = 0, scalarbars = 0;
    for (const auto& annotation : m_annotations) {
        switch (annotation->type) {
            case AnnotationType::DISTANCE_MEASUREMENT:
            case AnnotationType::ANGLE_MEASUREMENT:
            case AnnotationType::DIMENSION:
                measurements++;
                break;
            case AnnotationType::TEXT_LABEL:
            case AnnotationType::ARROW:
                labels++;
                break;
            case AnnotationType::COORDINATE_AXES:
                axes++;
                break;
            case AnnotationType::SCALAR_BAR:
                scalarbars++;
                break;
        }
    }

    oss << "    Measurements: " << measurements << "\n";
    oss << "    Labels: " << labels << "\n";
    oss << "    Axes: " << axes << "\n";
    oss << "    Scalar Bars: " << scalarbars << "\n";

    return oss.str();
}

// ============================================================================
// Private Methods
// ============================================================================

void AnnotationManager::createDistanceMeasurement(Annotation* annotation) {
    if (annotation->points.size() < 2) {
        return;
    }

    const auto& p1 = annotation->points[0];
    const auto& p2 = annotation->points[1];

    // Create line
    auto lineSource = vtkSmartPointer<vtkLineSource>::New();
    lineSource->SetPoint1(p1.x(), p1.y(), p1.z());
    lineSource->SetPoint2(p2.x(), p2.y(), p2.z());

    auto lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    lineMapper->SetInputConnection(lineSource->GetOutputPort());

    auto lineActor = vtkSmartPointer<vtkActor>::New();
    lineActor->SetMapper(lineMapper);
    lineActor->GetProperty()->SetColor(
        annotation->style.lineColor.x(),
        annotation->style.lineColor.y(),
        annotation->style.lineColor.z()
    );
    lineActor->GetProperty()->SetLineWidth(annotation->style.lineWidth);

    annotation->actors.push_back(lineActor);
    m_renderer->AddActor(lineActor);

    // Create spheres at endpoints
    for (const auto& point : {p1, p2}) {
        auto sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(point.x(), point.y(), point.z());
        sphere->SetRadius(annotation->style.pointSize * 0.01);
        sphere->SetThetaResolution(16);
        sphere->SetPhiResolution(16);

        auto sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        sphereMapper->SetInputConnection(sphere->GetOutputPort());

        auto sphereActor = vtkSmartPointer<vtkActor>::New();
        sphereActor->SetMapper(sphereMapper);
        sphereActor->GetProperty()->SetColor(
            annotation->style.pointColor.x(),
            annotation->style.pointColor.y(),
            annotation->style.pointColor.z()
        );

        annotation->actors.push_back(sphereActor);
        m_renderer->AddActor(sphereActor);
    }

    // Create text label at midpoint
    Eigen::Vector3d midpoint = (p1 + p2) / 2.0;
    auto textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(annotation->text.c_str());

    auto textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    textMapper->SetInputConnection(textSource->GetOutputPort());

    auto textFollower = vtkSmartPointer<vtkFollower>::New();
    textFollower->SetMapper(textMapper);
    textFollower->SetPosition(midpoint.x(), midpoint.y(), midpoint.z());
    textFollower->SetScale(0.05, 0.05, 0.05);
    textFollower->GetProperty()->SetColor(
        annotation->style.textColor.x(),
        annotation->style.textColor.y(),
        annotation->style.textColor.z()
    );

    if (m_renderer->GetActiveCamera()) {
        textFollower->SetCamera(m_renderer->GetActiveCamera());
    }

    annotation->actors.push_back(textFollower);
    m_renderer->AddActor(textFollower);
}

void AnnotationManager::createAngleMeasurement(Annotation* annotation) {
    if (annotation->points.size() < 3) {
        return;
    }

    const auto& p1 = annotation->points[0];
    const auto& p2 = annotation->points[1]; // vertex
    const auto& p3 = annotation->points[2];

    // Create two lines
    for (const auto& points : std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>>{{p2, p1}, {p2, p3}}) {
        auto lineSource = vtkSmartPointer<vtkLineSource>::New();
        lineSource->SetPoint1(points.first.x(), points.first.y(), points.first.z());
        lineSource->SetPoint2(points.second.x(), points.second.y(), points.second.z());

        auto lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        lineMapper->SetInputConnection(lineSource->GetOutputPort());

        auto lineActor = vtkSmartPointer<vtkActor>::New();
        lineActor->SetMapper(lineMapper);
        lineActor->GetProperty()->SetColor(
            annotation->style.lineColor.x(),
            annotation->style.lineColor.y(),
            annotation->style.lineColor.z()
        );
        lineActor->GetProperty()->SetLineWidth(annotation->style.lineWidth);

        annotation->actors.push_back(lineActor);
        m_renderer->AddActor(lineActor);
    }

    // Create text label at vertex
    auto textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(annotation->text.c_str());

    auto textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    textMapper->SetInputConnection(textSource->GetOutputPort());

    auto textFollower = vtkSmartPointer<vtkFollower>::New();
    textFollower->SetMapper(textMapper);
    textFollower->SetPosition(p2.x(), p2.y(), p2.z());
    textFollower->SetScale(0.05, 0.05, 0.05);
    textFollower->GetProperty()->SetColor(
        annotation->style.textColor.x(),
        annotation->style.textColor.y(),
        annotation->style.textColor.z()
    );

    if (m_renderer->GetActiveCamera()) {
        textFollower->SetCamera(m_renderer->GetActiveCamera());
    }

    annotation->actors.push_back(textFollower);
    m_renderer->AddActor(textFollower);
}

void AnnotationManager::createTextLabel(Annotation* annotation, bool attachToCamera) {
    if (annotation->points.empty()) {
        return;
    }

    const auto& position = annotation->points[0];

    auto textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(annotation->text.c_str());

    auto textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    textMapper->SetInputConnection(textSource->GetOutputPort());

    if (attachToCamera) {
        auto textFollower = vtkSmartPointer<vtkFollower>::New();
        textFollower->SetMapper(textMapper);
        textFollower->SetPosition(position.x(), position.y(), position.z());
        textFollower->SetScale(0.05, 0.05, 0.05);
        textFollower->GetProperty()->SetColor(
            annotation->style.textColor.x(),
            annotation->style.textColor.y(),
            annotation->style.textColor.z()
        );

        if (m_renderer->GetActiveCamera()) {
            textFollower->SetCamera(m_renderer->GetActiveCamera());
        }

        annotation->actors.push_back(textFollower);
        m_renderer->AddActor(textFollower);
    } else {
        auto textActor = vtkSmartPointer<vtkActor>::New();
        textActor->SetMapper(textMapper);
        textActor->SetPosition(position.x(), position.y(), position.z());
        textActor->SetScale(0.05, 0.05, 0.05);
        textActor->GetProperty()->SetColor(
            annotation->style.textColor.x(),
            annotation->style.textColor.y(),
            annotation->style.textColor.z()
        );

        annotation->actors.push_back(textActor);
        m_renderer->AddActor(textActor);
    }
}

void AnnotationManager::createScalarBar(Annotation* annotation, vtkLookupTable* lut) {
    auto scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle(annotation->text.c_str());
    scalarBar->SetNumberOfLabels(5);

    auto textProp = scalarBar->GetTitleTextProperty();
    textProp->SetFontSize(static_cast<int>(annotation->style.fontSize));
    textProp->SetColor(
        annotation->style.textColor.x(),
        annotation->style.textColor.y(),
        annotation->style.textColor.z()
    );

    annotation->actors.push_back(scalarBar);
    m_renderer->AddActor2D(scalarBar);
}

void AnnotationManager::applyStyle(Annotation* annotation) {
    // Apply style to existing actors
    for (auto& actor : annotation->actors) {
        if (auto actor3D = vtkActor::SafeDownCast(actor)) {
            actor3D->GetProperty()->SetColor(
                annotation->style.lineColor.x(),
                annotation->style.lineColor.y(),
                annotation->style.lineColor.z()
            );
        }
    }
}

double AnnotationManager::calculateDistance(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2) const {
    return (p2 - p1).norm();
}

double AnnotationManager::calculateAngle(
    const Eigen::Vector3d& p1,
    const Eigen::Vector3d& p2,
    const Eigen::Vector3d& p3
) const {
    Eigen::Vector3d v1 = (p1 - p2).normalized();
    Eigen::Vector3d v2 = (p3 - p2).normalized();

    double cosAngle = v1.dot(v2);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle)); // Clamp to [-1, 1]

    double angleRad = std::acos(cosAngle);
    return angleRad * 180.0 / M_PI; // Convert to degrees
}

std::string AnnotationManager::formatNumber(double value, int precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
