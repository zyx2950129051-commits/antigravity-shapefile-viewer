#include <QTest>
#include <QPointF>
#include "ShapeData.h"

using namespace Core;

class TestCanvasTransform : public QObject {
    Q_OBJECT

private slots:
    void testForwardAndInverseTransform() {
        ShapeBoundingBox bbox;
        bbox.expand(ShapePoint{0.0, 0.0});
        bbox.expand(ShapePoint{100.0, 50.0});

        double canvasWidth = 800.0;
        double canvasHeight = 600.0;
        double padding = 24.0;

        double viewW = canvasWidth - 2 * padding;
        double viewH = canvasHeight - 2 * padding;

        double scaleX = viewW / bbox.width();
        double scaleY = viewH / bbox.height();
        double scale = std::min(scaleX, scaleY);

        ShapePoint centerGeo = bbox.center();
        QPointF panOffset(0.0, 0.0);
        QPointF screenCenter(canvasWidth * 0.5 + panOffset.x(), canvasHeight * 0.5 + panOffset.y());

        // Forward transform
        auto geoToScreen = [&](const ShapePoint& pt) -> QPointF {
            double u = screenCenter.x() + (pt.x - centerGeo.x) * scale;
            double v = screenCenter.y() - (pt.y - centerGeo.y) * scale;
            return QPointF(u, v);
        };

        // Inverse transform
        auto screenToGeo = [&](const QPointF& scr) -> ShapePoint {
            double x = centerGeo.x + (scr.x() - screenCenter.x()) / scale;
            double y = centerGeo.y - (scr.y() - screenCenter.y()) / scale;
            return ShapePoint{x, y};
        };

        // Center should map to exact screen center
        QPointF screenCenterTransformed = geoToScreen(centerGeo);
        QCOMPARE(screenCenterTransformed.x(), screenCenter.x());
        QCOMPARE(screenCenterTransformed.y(), screenCenter.y());

        // Verify Y-axis flip: higher geo Y should produce smaller screen V (closer to top)
        ShapePoint topPt{50.0, 50.0};
        ShapePoint bottomPt{50.0, 0.0};
        QPointF topScreen = geoToScreen(topPt);
        QPointF bottomScreen = geoToScreen(bottomPt);
        QVERIFY(topScreen.y() < bottomScreen.y());

        // Roundtrip test on multiple points
        std::vector<ShapePoint> testPoints = {
            {0.0, 0.0}, {100.0, 50.0}, {25.5, 33.3}, {80.0, 10.0}
        };

        for (const auto& pt : testPoints) {
            QPointF s = geoToScreen(pt);
            ShapePoint back = screenToGeo(s);
            QVERIFY(std::abs(back.x - pt.x) < 1e-5);
            QVERIFY(std::abs(back.y - pt.y) < 1e-5);
        }
    }

    void testMouseAnchorZoomMath() {
        ShapeBoundingBox bbox;
        bbox.expand(ShapePoint{0.0, 0.0});
        bbox.expand(ShapePoint{100.0, 100.0});

        double canvasWidth = 800.0;
        double canvasHeight = 600.0;
        double scale = 5.0;

        ShapePoint centerGeo = bbox.center();
        QPointF panOffset(0.0, 0.0);
        QPointF screenCenter(canvasWidth * 0.5 + panOffset.x(), canvasHeight * 0.5 + panOffset.y());

        auto screenToGeo = [&](const QPointF& scr, double s, const QPointF& center) -> ShapePoint {
            double x = centerGeo.x + (scr.x() - center.x()) / s;
            double y = centerGeo.y - (scr.y() - center.y()) / s;
            return ShapePoint{x, y};
        };

        auto geoToScreen = [&](const ShapePoint& pt, double s, const QPointF& center) -> QPointF {
            double u = center.x() + (pt.x - centerGeo.x) * s;
            double v = center.y() - (pt.y - centerGeo.y) * s;
            return QPointF(u, v);
        };

        // Arbitrary mouse cursor position on screen
        QPointF mouseScreen(250.0, 180.0);
        ShapePoint mouseGeoBefore = screenToGeo(mouseScreen, scale, screenCenter);

        // Zoom in by factor 1.2
        double factor = 1.2;
        double newScale = scale * factor;

        // Calculate new screenCenter to keep mouseGeo invariant under newScale
        QPointF newScreenCenter(
            mouseScreen.x() - (mouseGeoBefore.x - centerGeo.x) * newScale,
            mouseScreen.y() + (mouseGeoBefore.y - centerGeo.y) * newScale
        );

        // Under new transform, mouse position should still map to identical geographic coordinate
        ShapePoint mouseGeoAfter = screenToGeo(mouseScreen, newScale, newScreenCenter);
        QVERIFY(std::abs(mouseGeoAfter.x - mouseGeoBefore.x) < 1e-6);
        QVERIFY(std::abs(mouseGeoAfter.y - mouseGeoBefore.y) < 1e-6);

        // And forward mapping of mouseGeoBefore should still yield exact mouseScreen
        QPointF remappedMouse = geoToScreen(mouseGeoBefore, newScale, newScreenCenter);
        QVERIFY(std::abs(remappedMouse.x() - mouseScreen.x()) < 1e-6);
        QVERIFY(std::abs(remappedMouse.y() - mouseScreen.y()) < 1e-6);
    }

    void testZoomClamping() {
        double baseScale = 2.0;
        double minScale = baseScale * 0.1;
        double maxScale = baseScale * 1000.0;

        auto clampZoom = [&](double s) {
            return std::clamp(s, minScale, maxScale);
        };

        QCOMPARE(clampZoom(0.01), minScale);
        QCOMPARE(clampZoom(5000.0), maxScale);
        QCOMPARE(clampZoom(20.0), 20.0);
    }
};

QTEST_MAIN(TestCanvasTransform)
#include "TestCanvasTransform.moc"
