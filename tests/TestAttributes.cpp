#include <QtTest>
#include <QTemporaryDir>
#include "ShapefileReader.h"
#include "TestDataGenerator.h"
#include "AttributeTableDialog.h"
#include "MainWindow.h"
#include "ShapeCanvas.h"

class TestAttributes : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qputenv("QT_TEST_OFFSCREEN", "1");
        if (qApp) qApp->setProperty("QT_TEST_MODE", true);
        QVERIFY(m_tempDir.isValid());
    }

    void testDbfAttributesLoading() {
        QString shpPath = m_tempDir.path() + "/polygon_attr.shp";
        QVERIFY(TestUtils::TestDataGenerator::createPolygonWithDbfShp(shpPath));

        Core::LoadResult res = Core::ShapefileReader::load(shpPath);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QVERIFY(res.dataset->hasAttributes);
        QCOMPARE(static_cast<int>(res.dataset->fields.size()), 3);

        QCOMPARE(res.dataset->fields[0].name, QStringLiteral("NAME"));
        QCOMPARE(res.dataset->fields[1].name, QStringLiteral("AREA"));
        QCOMPARE(res.dataset->fields[2].name, QStringLiteral("CODE"));

        QVERIFY(!res.dataset->features.empty());
        const auto& feat = res.dataset->features[0];
        QCOMPARE(static_cast<int>(feat.attributes.size()), 3);
        QCOMPARE(feat.attributes[0], QStringLiteral("地块A (测试)"));
        QCOMPARE(feat.attributes[1], QStringLiteral("1250.75"));
        QCOMPARE(feat.attributes[2], QStringLiteral("1001"));
    }

    void testAttributeTableModelAndFilter() {
        QString shpPath = m_tempDir.path() + "/polygon_attr.shp";
        Core::LoadResult res = Core::ShapefileReader::load(shpPath);
        QVERIFY(res.success);

        UI::AttributeTableModel model(res.dataset);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.columnCount(), 4); // FID + 3 fields

        // Filter test
        model.setFilterText(QStringLiteral("地块A"));
        QCOMPARE(model.rowCount(), 1);

        model.setFilterText(QStringLiteral("不存在的关键字"));
        QCOMPARE(model.rowCount(), 0);

        model.setFilterText(QString());
        QCOMPARE(model.rowCount(), 1);
    }

    void testFeatureDeletion() {
        QString shpPath = m_tempDir.path() + "/polygon_attr.shp";
        UI::MainWindow win;
        win.openShapefile(shpPath);
        QTest::qWait(200);

        auto dataset = win.currentDataset();
        QVERIFY(dataset != nullptr);
        int initialCount = dataset->totalFeatureCount;
        QVERIFY(initialCount > 0);

        // Delete feature 0
        win.deleteFeature(0);
        QCOMPARE(dataset->totalFeatureCount, initialCount - 1);
        QCOMPARE(dataset->totalVertexCount, 0);
    }

private:
    QTemporaryDir m_tempDir;
};

QTEST_MAIN(TestAttributes)
#include "TestAttributes.moc"
