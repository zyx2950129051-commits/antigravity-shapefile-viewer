#include "TestDataGenerator.h"
#include <shapefil.h>
#include <QDir>
#include <QFileInfo>
#include <vector>

namespace TestUtils {

static SHPHandle openCreateSHP(const QString& filePath, int nShapeType) {
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
    QByteArray utf8Path = QDir::toNativeSeparators(fi.absoluteFilePath()).toUtf8();

#if defined(_WIN32) || defined(WIN32)
    SAHooks sHooks;
    SASetupUtf8Hooks(&sHooks);
    return SHPCreateLL(utf8Path.constData(), nShapeType, &sHooks);
#else
    return SHPCreate(utf8Path.constData(), nShapeType);
#endif
}

bool TestDataGenerator::createPointShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_POINT);
    if (!hSHP) return false;

    // Feature 1: (116.4, 39.9) - Beijing
    double x1 = 116.4, y1 = 39.9, z = 0, m = 0;
    SHPObject* obj1 = SHPCreateSimpleObject(SHPT_POINT, 1, &x1, &y1, &z);
    SHPWriteObject(hSHP, -1, obj1);
    SHPDestroyObject(obj1);

    // Feature 2: (121.5, 31.2) - Shanghai
    double x2 = 121.5, y2 = 31.2;
    SHPObject* obj2 = SHPCreateSimpleObject(SHPT_POINT, 1, &x2, &y2, &z);
    SHPWriteObject(hSHP, -1, obj2);
    SHPDestroyObject(obj2);

    // Feature 3: (113.3, 23.1) - Guangzhou
    double x3 = 113.3, y3 = 23.1;
    SHPObject* obj3 = SHPCreateSimpleObject(SHPT_POINT, 1, &x3, &y3, &z);
    SHPWriteObject(hSHP, -1, obj3);
    SHPDestroyObject(obj3);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createMultiPointShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_MULTIPOINT);
    if (!hSHP) return false;

    // A cluster of 4 points
    double x[4] = {100.0, 101.0, 102.0, 103.0};
    double y[4] = {30.0, 31.0, 32.0, 33.0};
    SHPObject* obj = SHPCreateSimpleObject(SHPT_MULTIPOINT, 4, x, y, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createPolylineShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_ARC);
    if (!hSHP) return false;

    double x[5] = {10.0, 20.0, 30.0, 40.0, 50.0};
    double y[5] = {10.0, 30.0, 20.0, 50.0, 40.0};
    SHPObject* obj = SHPCreateSimpleObject(SHPT_ARC, 5, x, y, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createMultiPartPolylineShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_ARC);
    if (!hSHP) return false;

    // 2 parts: part 1 has 3 vertices, part 2 has 3 vertices
    int panPartStart[2] = {0, 3};
    int panPartType[2] = {SHPP_RING, SHPP_RING};
    double x[6] = {0.0, 5.0, 10.0, 20.0, 25.0, 30.0};
    double y[6] = {0.0, 10.0, 0.0, 20.0, 10.0, 20.0};

    SHPObject* obj = SHPCreateObject(SHPT_ARC, -1, 2, panPartStart, panPartType, 6, x, y, nullptr, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createPolygonShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_POLYGON);
    if (!hSHP) return false;

    // Triangle / closed box (clockwise)
    double x[5] = {0.0, 100.0, 100.0, 0.0, 0.0};
    double y[5] = {0.0, 0.0, 100.0, 100.0, 0.0};
    SHPObject* obj = SHPCreateSimpleObject(SHPT_POLYGON, 5, x, y, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createPolygonWithHoleShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_POLYGON);
    if (!hSHP) return false;

    // Outer ring (clockwise): (0,0) -> (100,0) -> (100,100) -> (0,100) -> (0,0) (5 vertices)
    // Inner ring / hole (counter-clockwise): (25,25) -> (25,75) -> (75,75) -> (75,25) -> (25,25) (5 vertices)
    int panPartStart[2] = {0, 5};
    int panPartType[2] = {SHPP_RING, SHPP_INNERRING};

    double x[10] = {
        0.0, 100.0, 100.0, 0.0, 0.0,
        25.0, 25.0, 75.0, 75.0, 25.0
    };
    double y[10] = {
        0.0, 0.0, 100.0, 100.0, 0.0,
        25.0, 75.0, 75.0, 25.0, 25.0
    };

    SHPObject* obj = SHPCreateObject(SHPT_POLYGON, -1, 2, panPartStart, panPartType, 10, x, y, nullptr, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createNullShapeShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_POINT);
    if (!hSHP) return false;

    // Record 0: Valid point
    double x1 = 10.0, y1 = 10.0, z = 0;
    SHPObject* obj1 = SHPCreateSimpleObject(SHPT_POINT, 1, &x1, &y1, &z);
    SHPWriteObject(hSHP, -1, obj1);
    SHPDestroyObject(obj1);

    // Record 1: Null shape (SHPT_NULL)
    SHPObject* objNull = SHPCreateSimpleObject(SHPT_NULL, 0, nullptr, nullptr, nullptr);
    SHPWriteObject(hSHP, -1, objNull);
    SHPDestroyObject(objNull);

    // Record 2: Another valid point
    double x2 = 20.0, y2 = 20.0;
    SHPObject* obj2 = SHPCreateSimpleObject(SHPT_POINT, 1, &x2, &y2, &z);
    SHPWriteObject(hSHP, -1, obj2);
    SHPDestroyObject(obj2);

    SHPClose(hSHP);
    return true;
}

bool TestDataGenerator::createMultiPatchShp(const QString& filePath) {
    SHPHandle hSHP = openCreateSHP(filePath, SHPT_MULTIPATCH);
    if (!hSHP) return false;

    int panPartStart[1] = {0};
    int panPartType[1] = {SHPP_TRISTRIP};
    double x[3] = {0.0, 1.0, 0.0};
    double y[3] = {0.0, 0.0, 1.0};
    double z[3] = {0.0, 0.0, 0.0};

    SHPObject* obj = SHPCreateObject(SHPT_MULTIPATCH, -1, 1, panPartStart, panPartType, 3, x, y, z, nullptr);
    SHPWriteObject(hSHP, -1, obj);
    SHPDestroyObject(obj);

    SHPClose(hSHP);
    return true;
}

QString TestDataGenerator::createChinesePathShp(const QString& baseDir) {
    QString subDir = baseDir + QStringLiteral("/测试中文目录");
    QDir().mkpath(subDir);
    QString filePath = subDir + QStringLiteral("/中国地图边界.shp");
    if (createPolygonShp(filePath)) {
        return filePath;
    }
    return QString();
}

} // namespace TestUtils
