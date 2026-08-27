#include <QTest>
#include <QTemporaryDir>
#include "MainWindow.h"
#include "ShapeCanvas.h"
#include "TestDataGenerator.h"

using namespace UI;
using namespace TestUtils;

class TestSmoke : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qputenv("QT_TEST_OFFSCREEN", "1");
        if (qApp) qApp->setProperty("QT_TEST_MODE", true);
        QVERIFY(m_tempDir.isValid());
    }

    void testMainWindowSmoke() {
        MainWindow win;
        win.show();
        QVERIFY(win.isVisible());
        QVERIFY(!win.canvas()->hasData());

        // Create a test polygon shapefile
        QString polyPath = m_tempDir.path() + "/smoke_polygon.shp";
        QVERIFY(TestDataGenerator::createPolygonWithHoleShp(polyPath));

        // Open shapefile
        win.openShapefile(polyPath);

        // Wait for async load to finish
        QTRY_VERIFY_WITH_TIMEOUT(!win.isLoading(), 5000);

        QVERIFY(win.canvas()->hasData());
        QCOMPARE(win.canvas()->dataset()->totalFeatureCount, 1);
        QCOMPARE(win.canvas()->dataset()->totalVertexCount, 10);

        // Test canvas zoom ratio
        QVERIFY(win.canvas()->currentZoomRatio() > 0.0);

        // Test fit to window
        win.canvas()->fitToWindow();
        QVERIFY(std::abs(win.canvas()->currentZoomRatio() - 1.0) < 1e-4);

        // Test invalid file does not destroy existing data
        QString invalidPath = m_tempDir.path() + "/non_existent.shp";
        win.openShapefile(invalidPath);
        QTRY_VERIFY_WITH_TIMEOUT(!win.isLoading(), 5000);

        // Dataset should still be retained
        QVERIFY(win.canvas()->hasData());
        QCOMPARE(win.canvas()->dataset()->totalFeatureCount, 1);
    }

private:
    QTemporaryDir m_tempDir;
};

QTEST_MAIN(TestSmoke)
#include "TestSmoke.moc"
