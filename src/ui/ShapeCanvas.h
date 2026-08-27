#pragma once

#include <QWidget>
#include <QPointF>
#include <memory>
#include "ShapeData.h"

namespace UI {

class ShapeCanvas : public QWidget {
    Q_OBJECT

public:
    explicit ShapeCanvas(QWidget* parent = nullptr);
    ~ShapeCanvas() override = default;

    void setDataset(std::shared_ptr<Core::ShapeDataset> dataset);
    void fitToWindow();
    void clear();

    bool hasData() const;
    std::shared_ptr<Core::ShapeDataset> dataset() const { return m_dataset; }
    double currentZoomRatio() const;

signals:
    void mouseGeoPositionChanged(double geoX, double geoY);
    void zoomLevelChanged(double zoomRatio);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QPointF geoToScreen(const Core::ShapePoint& pt) const;
    Core::ShapePoint screenToGeo(const QPointF& scr) const;
    Core::ShapeBoundingBox viewportGeoBoundingBox() const;
    void calculateBaseScale();

private:
    std::shared_ptr<Core::ShapeDataset> m_dataset;
    bool m_hasDataset{false};

    double m_baseScale{1.0};
    double m_currentScale{1.0};
    QPointF m_panOffset{0.0, 0.0};

    bool m_isDragging{false};
    QPoint m_lastMousePos;

    static constexpr double PADDING = 24.0;
    static constexpr double ZOOM_FACTOR = 1.2;
    static constexpr double MIN_SCALE_RATIO = 0.1;
    static constexpr double MAX_SCALE_RATIO = 1000.0;
};

} // namespace UI
