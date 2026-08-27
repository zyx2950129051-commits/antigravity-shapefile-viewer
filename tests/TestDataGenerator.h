#pragma once

#include <QString>

namespace TestUtils {

class TestDataGenerator {
public:
    static bool createPointShp(const QString& filePath);
    static bool createMultiPointShp(const QString& filePath);
    static bool createPolylineShp(const QString& filePath);
    static bool createMultiPartPolylineShp(const QString& filePath);
    static bool createPolygonShp(const QString& filePath);
    static bool createPolygonWithHoleShp(const QString& filePath);
    static bool createNullShapeShp(const QString& filePath);
    static bool createMultiPatchShp(const QString& filePath);
    static QString createChinesePathShp(const QString& baseDir);
    static bool createPolygonWithDbfShp(const QString& filePath);
};

} // namespace TestUtils
