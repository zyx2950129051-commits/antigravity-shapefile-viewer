#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include "ShapefileReader.h"
#include "TestDataGenerator.h"

using namespace Core;
using namespace TestUtils;

class TestReader : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_tempDir.isValid());
    }

    void testPointShp() {
        QString path = m_tempDir.path() + "/points.shp";
        QVERIFY(TestDataGenerator::createPointShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::Point);
        QCOMPARE(res.dataset->totalFeatureCount, 3);
        QCOMPARE(res.dataset->totalVertexCount, 3);
        QVERIFY(res.dataset->bbox.isValid());
        QCOMPARE(res.dataset->typeDisplayName(), QStringLiteral("点 (Point)"));
    }

    void testMultiPointShp() {
        QString path = m_tempDir.path() + "/multipoints.shp";
        QVERIFY(TestDataGenerator::createMultiPointShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::MultiPoint);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
        QCOMPARE(res.dataset->totalVertexCount, 4);
    }

    void testPolylineShp() {
        QString path = m_tempDir.path() + "/lines.shp";
        QVERIFY(TestDataGenerator::createPolylineShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::Polyline);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
        QCOMPARE(res.dataset->totalVertexCount, 5);
        QCOMPARE(static_cast<int>(res.dataset->features[0].parts.size()), 1);
    }

    void testMultiPartPolylineShp() {
        QString path = m_tempDir.path() + "/multipart_lines.shp";
        QVERIFY(TestDataGenerator::createMultiPartPolylineShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::Polyline);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
        QCOMPARE(res.dataset->totalVertexCount, 6);
        QCOMPARE(static_cast<int>(res.dataset->features[0].parts.size()), 2);
    }

    void testPolygonShp() {
        QString path = m_tempDir.path() + "/polygons.shp";
        QVERIFY(TestDataGenerator::createPolygonShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::Polygon);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
        QCOMPARE(res.dataset->totalVertexCount, 5);
        QCOMPARE(static_cast<int>(res.dataset->features[0].parts.size()), 1);
    }

    void testPolygonWithHoleShp() {
        QString path = m_tempDir.path() + "/polygon_hole.shp";
        QVERIFY(TestDataGenerator::createPolygonWithHoleShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->primaryType, ShapeType::Polygon);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
        QCOMPARE(res.dataset->totalVertexCount, 10);
        QCOMPARE(static_cast<int>(res.dataset->features[0].parts.size()), 2);
    }

    void testNullShapeFiltering() {
        QString path = m_tempDir.path() + "/null_shapes.shp";
        QVERIFY(TestDataGenerator::createNullShapeShp(path));

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        // Original has 3 entities, but record 1 is SHPT_NULL, so only 2 valid features
        QCOMPARE(res.dataset->totalFeatureCount, 2);
        QCOMPARE(res.dataset->totalVertexCount, 2);
    }

    void testChinesePath() {
        QString path = TestDataGenerator::createChinesePathShp(m_tempDir.path());
        QVERIFY(!path.isEmpty());

        LoadResult res = ShapefileReader::load(path);
        QVERIFY2(res.success, qPrintable(res.errorMessage));
        QVERIFY(res.dataset != nullptr);
        QCOMPARE(res.dataset->totalFeatureCount, 1);
    }

    void testInvalidFilesAndErrors() {
        // Empty path
        LoadResult r1 = ShapefileReader::load("");
        QVERIFY(!r1.success);
        QVERIFY(!r1.errorMessage.isEmpty());

        // Non-existent file
        LoadResult r2 = ShapefileReader::load(m_tempDir.path() + "/non_existent.shp");
        QVERIFY(!r2.success);
        QVERIFY(r2.errorMessage.contains(QStringLiteral("不存在")));

        // Non-shp extension
        QString txtPath = m_tempDir.path() + "/sample.txt";
        QFile txtFile(txtPath);
        QVERIFY(txtFile.open(QIODevice::WriteOnly));
        txtFile.write("dummy");
        txtFile.close();
        LoadResult r3 = ShapefileReader::load(txtPath);
        QVERIFY(!r3.success);
        QVERIFY(r3.errorMessage.contains(QStringLiteral(".shp")));

        // Corrupted 0-byte shp file
        QString emptyShp = m_tempDir.path() + "/empty.shp";
        QFile f(emptyShp);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        LoadResult r4 = ShapefileReader::load(emptyShp);
        QVERIFY(!r4.success);
        QVERIFY(r4.errorMessage.contains(QStringLiteral("无法打开或解析")));

        // MultiPatch unsupported format
        QString mpPath = m_tempDir.path() + "/multipatch.shp";
        QVERIFY(TestDataGenerator::createMultiPatchShp(mpPath));
        LoadResult r5 = ShapefileReader::load(mpPath);
        QVERIFY(!r5.success);
        QVERIFY(r5.errorMessage.contains(QStringLiteral("MultiPatch")));
    }

    void testBoundingBoxCalculations() {
        ShapeBoundingBox box1;
        QVERIFY(!box1.isValid());

        box1.expand(ShapePoint{10.0, 20.0});
        QVERIFY(box1.isValid());
        QCOMPARE(box1.minX, 10.0);
        QCOMPARE(box1.maxX, 10.0);

        box1.expand(ShapePoint{30.0, 40.0});
        QCOMPARE(box1.width(), 20.0);
        QCOMPARE(box1.height(), 20.0);
        QCOMPARE(box1.center().x, 20.0);
        QCOMPARE(box1.center().y, 30.0);

        ShapeBoundingBox box2;
        box2.expand(ShapePoint{25.0, 35.0});
        box2.expand(ShapePoint{50.0, 60.0});
        QVERIFY(box1.intersects(box2));

        ShapeBoundingBox box3;
        box3.expand(ShapePoint{100.0, 100.0});
        box3.expand(ShapePoint{110.0, 110.0});
        QVERIFY(!box1.intersects(box3));
    }

private:
    QTemporaryDir m_tempDir;
};

QTEST_MAIN(TestReader)
#include "TestReader.moc"
