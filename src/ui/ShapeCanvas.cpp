#include "ShapeCanvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

namespace UI {

ShapeCanvas::ShapeCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void ShapeCanvas::setDataset(std::shared_ptr<Core::ShapeDataset> dataset) {
    m_dataset = std::move(dataset);
    m_hasDataset = (m_dataset != nullptr && m_dataset->totalFeatureCount > 0);
    m_selectedFeatureIndex = -1;
    fitToWindow();
}

void ShapeCanvas::clear() {
    m_dataset.reset();
    m_hasDataset = false;
    m_selectedFeatureIndex = -1;
    m_panOffset = QPointF(0.0, 0.0);
    m_baseScale = 1.0;
    m_currentScale = 1.0;
    emit zoomLevelChanged(1.0);
    emit featureSelected(-1);
    update();
}

bool ShapeCanvas::hasData() const {
    return m_hasDataset && m_dataset != nullptr;
}

double ShapeCanvas::currentZoomRatio() const {
    if (m_baseScale <= 1e-9) return 1.0;
    return m_currentScale / m_baseScale;
}

void ShapeCanvas::setSelectedFeatureIndex(int index) {
    if (index >= 0 && hasData() && index < static_cast<int>(m_dataset->features.size())) {
        m_selectedFeatureIndex = index;
    } else {
        m_selectedFeatureIndex = -1;
    }
    update();
}

void ShapeCanvas::centerOnFeature(int index) {
    if (!hasData() || index < 0 || index >= static_cast<int>(m_dataset->features.size())) {
        return;
    }

    const auto& feat = m_dataset->features[index];
    if (!feat.bbox.isValid()) return;

    Core::ShapePoint featCenter = feat.bbox.center();
    Core::ShapePoint centerGeo = m_dataset->bbox.center();

    // Adjust panOffset so featCenter is placed at canvas center (width/2, height/2)
    m_panOffset.setX(-(featCenter.x - centerGeo.x) * m_currentScale);
    m_panOffset.setY((featCenter.y - centerGeo.y) * m_currentScale);

    m_selectedFeatureIndex = index;
    update();
}

void ShapeCanvas::calculateBaseScale() {
    if (!hasData() || !m_dataset->bbox.isValid()) {
        m_baseScale = 1.0;
        m_currentScale = 1.0;
        return;
    }

    double viewW = std::max(10.0, width() - 2.0 * PADDING);
    double viewH = std::max(10.0, height() - 2.0 * PADDING);

    double dataW = std::max(m_dataset->bbox.width(), 1e-6);
    double dataH = std::max(m_dataset->bbox.height(), 1e-6);

    double sx = viewW / dataW;
    double sy = viewH / dataH;

    m_baseScale = std::min(sx, sy);
    if (m_baseScale <= 1e-9) m_baseScale = 1.0;
}

void ShapeCanvas::fitToWindow() {
    calculateBaseScale();
    m_currentScale = m_baseScale;
    m_panOffset = QPointF(0.0, 0.0);
    emit zoomLevelChanged(currentZoomRatio());
    update();
}

QPointF ShapeCanvas::geoToScreen(const Core::ShapePoint& pt) const {
    if (!hasData()) return QPointF(0.0, 0.0);

    Core::ShapePoint centerGeo = m_dataset->bbox.center();
    double scrCenterX = width() * 0.5 + m_panOffset.x();
    double scrCenterY = height() * 0.5 + m_panOffset.y();

    double u = scrCenterX + (pt.x - centerGeo.x) * m_currentScale;
    double v = scrCenterY - (pt.y - centerGeo.y) * m_currentScale; // Invert Y-axis
    return QPointF(u, v);
}

Core::ShapePoint ShapeCanvas::screenToGeo(const QPointF& scr) const {
    if (!hasData()) return Core::ShapePoint{0.0, 0.0};

    Core::ShapePoint centerGeo = m_dataset->bbox.center();
    double scrCenterX = width() * 0.5 + m_panOffset.x();
    double scrCenterY = height() * 0.5 + m_panOffset.y();

    double x = centerGeo.x + (scr.x() - scrCenterX) / m_currentScale;
    double y = centerGeo.y - (scr.y() - scrCenterY) / m_currentScale;
    return Core::ShapePoint{x, y};
}

Core::ShapeBoundingBox ShapeCanvas::viewportGeoBoundingBox() const {
    Core::ShapeBoundingBox box;
    if (!hasData()) return box;

    box.expand(screenToGeo(QPointF(0.0, 0.0)));
    box.expand(screenToGeo(QPointF(width(), 0.0)));
    box.expand(screenToGeo(QPointF(width(), height())));
    box.expand(screenToGeo(QPointF(0.0, height())));
    return box;
}

// Distance from point to line segment in screen pixels
static double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b) {
    double l2 = (b.x() - a.x()) * (b.x() - a.x()) + (b.y() - a.y()) * (b.y() - a.y());
    if (l2 == 0.0) return std::hypot(p.x() - a.x(), p.y() - a.y());

    double t = ((p.x() - a.x()) * (b.x() - a.x()) + (p.y() - a.y()) * (b.y() - a.y())) / l2;
    t = std::clamp(t, 0.0, 1.0);

    QPointF projection(a.x() + t * (b.x() - a.x()), a.y() + t * (b.y() - a.y()));
    return std::hypot(p.x() - projection.x(), p.y() - projection.y());
}

int ShapeCanvas::pickFeatureAt(const QPointF& screenPos) const {
    if (!hasData()) return -1;

    constexpr double tolerancePx = 6.0;
    Core::ShapePoint mouseGeo = screenToGeo(screenPos);

    // Expand bounding box by tolerance in geo coordinates
    double geoTolerance = tolerancePx / m_currentScale;
    Core::ShapeBoundingBox mouseBox;
    mouseBox.minX = mouseGeo.x - geoTolerance;
    mouseBox.maxX = mouseGeo.x + geoTolerance;
    mouseBox.minY = mouseGeo.y - geoTolerance;
    mouseBox.maxY = mouseGeo.y + geoTolerance;

    int total = static_cast<int>(m_dataset->features.size());

    // Iterate backwards to pick top-most rendered feature
    for (int i = total - 1; i >= 0; --i) {
        const auto& feat = m_dataset->features[i];
        if (!feat.bbox.intersects(mouseBox)) continue;

        switch (feat.type) {
            case Core::ShapeType::Point:
            case Core::ShapeType::MultiPoint: {
                for (const auto& part : feat.parts) {
                    for (const auto& pt : part.points) {
                        QPointF scrPt = geoToScreen(pt);
                        if (std::hypot(screenPos.x() - scrPt.x(), screenPos.y() - scrPt.y()) <= (tolerancePx + 4.0)) {
                            return i;
                        }
                    }
                }
                break;
            }
            case Core::ShapeType::Polyline: {
                for (const auto& part : feat.parts) {
                    for (size_t v = 0; v + 1 < part.points.size(); ++v) {
                        QPointF s1 = geoToScreen(part.points[v]);
                        QPointF s2 = geoToScreen(part.points[v + 1]);
                        if (distanceToSegment(screenPos, s1, s2) <= tolerancePx) {
                            return i;
                        }
                    }
                }
                break;
            }
            case Core::ShapeType::Polygon: {
                QPainterPath path;
                path.setFillRule(Qt::OddEvenFill);
                for (const auto& part : feat.parts) {
                    if (part.points.size() < 3) continue;
                    path.moveTo(geoToScreen(part.points[0]));
                    for (size_t v = 1; v < part.points.size(); ++v) {
                        path.lineTo(geoToScreen(part.points[v]));
                    }
                    path.closeSubpath();
                }
                if (path.contains(screenPos)) {
                    return i;
                }
                break;
            }
            default:
                break;
        }
    }
    return -1;
}

void ShapeCanvas::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Background color
    painter.fillRect(rect(), QColor(245, 247, 250));

    if (!hasData()) {
        // Draw Empty State Placeholder
        painter.setPen(QColor(140, 150, 165));
        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);

        QString emptyTitle = QStringLiteral("暂未加载 Shapefile 数据");
        QString emptySub = QStringLiteral("请点击顶部「选择 SHP 文件」按钮打开地图要素文件 (*.shp)");

        QRect titleRect = rect().adjusted(20, -40, -20, -40);
        painter.drawText(titleRect, Qt::AlignCenter, emptyTitle);

        font.setPointSize(11);
        font.setBold(false);
        painter.setFont(font);
        painter.setPen(QColor(160, 170, 185));
        QRect subRect = rect().adjusted(20, 30, -20, 30);
        painter.drawText(subRect, Qt::AlignCenter, emptySub);
        return;
    }

    // Viewport Culling bounding box
    Core::ShapeBoundingBox viewBBox = viewportGeoBoundingBox();

    // Setup pens and brushes
    QPen pointPen(QColor(255, 255, 255), 1.0);
    QBrush pointBrush(QColor(30, 136, 229));

    QPen linePen(QColor(25, 118, 210), 1.5);
    linePen.setCosmetic(true);

    QPen polyBorderPen(QColor(21, 101, 192, 220), 1.2);
    polyBorderPen.setCosmetic(true);
    QBrush polyFillBrush(QColor(33, 150, 243, 85));

    for (size_t idx = 0; idx < m_dataset->features.size(); ++idx) {
        const auto& feat = m_dataset->features[idx];
        if (!feat.bbox.intersects(viewBBox)) {
            continue; // Skip out-of-screen features
        }

        switch (feat.type) {
            case Core::ShapeType::Point:
            case Core::ShapeType::MultiPoint: {
                painter.setPen(pointPen);
                painter.setBrush(pointBrush);
                constexpr double pointRadius = 4.0;
                for (const auto& part : feat.parts) {
                    for (const auto& pt : part.points) {
                        QPointF scrPt = geoToScreen(pt);
                        painter.drawEllipse(scrPt, pointRadius, pointRadius);
                    }
                }
                break;
            }
            case Core::ShapeType::Polyline: {
                painter.setPen(linePen);
                painter.setBrush(Qt::NoBrush);
                for (const auto& part : feat.parts) {
                    if (part.points.size() < 2) continue;
                    QPolygonF poly;
                    poly.reserve(static_cast<qsizetype>(part.points.size()));
                    for (const auto& pt : part.points) {
                        poly.append(geoToScreen(pt));
                    }
                    painter.drawPolyline(poly);
                }
                break;
            }
            case Core::ShapeType::Polygon: {
                painter.setPen(polyBorderPen);
                painter.setBrush(polyFillBrush);

                QPainterPath path;
                path.setFillRule(Qt::OddEvenFill); // Support holes

                for (const auto& part : feat.parts) {
                    if (part.points.size() < 3) continue;

                    path.moveTo(geoToScreen(part.points[0]));
                    for (size_t i = 1; i < part.points.size(); ++i) {
                        path.lineTo(geoToScreen(part.points[i]));
                    }
                    path.closeSubpath();
                }

                painter.drawPath(path);
                break;
            }
            default:
                break;
        }
    }

    // Highlight Selected Feature
    if (m_selectedFeatureIndex >= 0 && m_selectedFeatureIndex < static_cast<int>(m_dataset->features.size())) {
        const auto& selFeat = m_dataset->features[m_selectedFeatureIndex];

        QPen highlightPen(QColor(255, 112, 67), 3.0); // Vibrant orange
        highlightPen.setCosmetic(true);
        QBrush highlightBrush(QColor(255, 171, 64, 130)); // Translucent amber fill

        switch (selFeat.type) {
            case Core::ShapeType::Point:
            case Core::ShapeType::MultiPoint: {
                painter.setPen(highlightPen);
                painter.setBrush(highlightBrush);
                constexpr double selRadius = 7.0;
                for (const auto& part : selFeat.parts) {
                    for (const auto& pt : part.points) {
                        QPointF scrPt = geoToScreen(pt);
                        painter.drawEllipse(scrPt, selRadius, selRadius);
                    }
                }
                break;
            }
            case Core::ShapeType::Polyline: {
                painter.setPen(highlightPen);
                painter.setBrush(Qt::NoBrush);
                for (const auto& part : selFeat.parts) {
                    if (part.points.size() < 2) continue;
                    QPolygonF poly;
                    poly.reserve(static_cast<qsizetype>(part.points.size()));
                    for (const auto& pt : part.points) {
                        poly.append(geoToScreen(pt));
                    }
                    painter.drawPolyline(poly);
                }
                break;
            }
            case Core::ShapeType::Polygon: {
                painter.setPen(highlightPen);
                painter.setBrush(highlightBrush);

                QPainterPath path;
                path.setFillRule(Qt::OddEvenFill);
                for (const auto& part : selFeat.parts) {
                    if (part.points.size() < 3) continue;
                    path.moveTo(geoToScreen(part.points[0]));
                    for (size_t i = 1; i < part.points.size(); ++i) {
                        path.lineTo(geoToScreen(part.points[i]));
                    }
                    path.closeSubpath();
                }
                painter.drawPath(path);
                break;
            }
            default:
                break;
        }
    }
}

void ShapeCanvas::wheelEvent(QWheelEvent* event) {
    if (!hasData()) {
        event->ignore();
        return;
    }

    QPointF mousePos = event->position();
    Core::ShapePoint mouseGeoBefore = screenToGeo(mousePos);

    double delta = event->angleDelta().y();
    if (std::abs(delta) < 1e-4) {
        event->accept();
        return;
    }

    double zoomStep = (delta > 0) ? ZOOM_FACTOR : (1.0 / ZOOM_FACTOR);
    double targetScale = m_currentScale * zoomStep;

    double minScale = m_baseScale * MIN_SCALE_RATIO;
    double maxScale = m_baseScale * MAX_SCALE_RATIO;
    double newScale = std::clamp(targetScale, minScale, maxScale);

    if (std::abs(newScale - m_currentScale) < 1e-9) {
        event->accept();
        return;
    }

    // Adjust panOffset so mouseGeoBefore remains exactly at mousePos
    Core::ShapePoint centerGeo = m_dataset->bbox.center();
    double newScrCenterX = mousePos.x() - (mouseGeoBefore.x - centerGeo.x) * newScale;
    double newScrCenterY = mousePos.y() + (mouseGeoBefore.y - centerGeo.y) * newScale;

    m_panOffset.setX(newScrCenterX - width() * 0.5);
    m_panOffset.setY(newScrCenterY - height() * 0.5);
    m_currentScale = newScale;

    emit zoomLevelChanged(currentZoomRatio());
    emit mouseGeoPositionChanged(mouseGeoBefore.x, mouseGeoBefore.y);
    update();
    event->accept();
}

void ShapeCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_pressPos = event->pos();
        m_lastMousePos = event->pos();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShapeCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        if (!m_isDragging && (event->pos() - m_pressPos).manhattanLength() > 4) {
            m_isDragging = true;
            setCursor(Qt::ClosedHandCursor);
        }

        if (m_isDragging) {
            QPoint delta = event->pos() - m_lastMousePos;
            m_lastMousePos = event->pos();
            m_panOffset += QPointF(delta.x(), delta.y());
            update();
        }
    } else {
        if (hasData()) {
            setCursor(Qt::CrossCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    if (hasData()) {
        Core::ShapePoint geo = screenToGeo(event->position());
        emit mouseGeoPositionChanged(geo.x, geo.y);
    }

    event->accept();
}

void ShapeCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            m_isDragging = false;
            setCursor(hasData() ? Qt::CrossCursor : Qt::ArrowCursor);
        } else {
            // Click pick action
            int picked = pickFeatureAt(event->position());
            setSelectedFeatureIndex(picked);
            emit featureSelected(picked);
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShapeCanvas::leaveEvent(QEvent* /*event*/) {
    if (!m_isDragging) {
        setCursor(Qt::ArrowCursor);
    }
}

void ShapeCanvas::resizeEvent(QResizeEvent* /*event*/) {
    if (hasData()) {
        calculateBaseScale();
        emit zoomLevelChanged(currentZoomRatio());
        update();
    }
}

} // namespace UI
