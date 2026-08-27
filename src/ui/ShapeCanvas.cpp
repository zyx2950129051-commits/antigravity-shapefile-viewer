#include "ShapeCanvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
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
    fitToWindow();
}

void ShapeCanvas::clear() {
    m_dataset.reset();
    m_hasDataset = false;
    m_panOffset = QPointF(0.0, 0.0);
    m_baseScale = 1.0;
    m_currentScale = 1.0;
    emit zoomLevelChanged(1.0);
    update();
}

bool ShapeCanvas::hasData() const {
    return m_hasDataset && m_dataset != nullptr;
}

double ShapeCanvas::currentZoomRatio() const {
    if (m_baseScale <= 1e-9) return 1.0;
    return m_currentScale / m_baseScale;
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

    for (const auto& feat : m_dataset->features) {
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
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShapeCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_panOffset += QPointF(delta.x(), delta.y());
        update();
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
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        if (hasData()) {
            setCursor(Qt::CrossCursor);
        } else {
            setCursor(Qt::ArrowCursor);
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
        // Maintain current zoom scale relative to base
        emit zoomLevelChanged(currentZoomRatio());
        update();
    }
}

} // namespace UI
