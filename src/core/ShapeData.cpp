#include "ShapeData.h"

namespace Core {

QString shapeTypeToString(ShapeType type) {
    switch (type) {
        case ShapeType::Point:
            return QStringLiteral("点 (Point)");
        case ShapeType::MultiPoint:
            return QStringLiteral("多点 (MultiPoint)");
        case ShapeType::Polyline:
            return QStringLiteral("折线 (Polyline)");
        case ShapeType::Polygon:
            return QStringLiteral("多边形 (Polygon)");
        case ShapeType::Unknown:
        default:
            return QStringLiteral("未知几何类型");
    }
}

} // namespace Core
