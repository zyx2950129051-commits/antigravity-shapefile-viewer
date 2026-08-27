#pragma once

#include <QString>
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>

namespace Core {

struct ShapePoint {
    double x{0.0};
    double y{0.0};

    bool operator==(const ShapePoint& other) const {
        return x == other.x && y == other.y;
    }
};

struct ShapeBoundingBox {
    double minX{std::numeric_limits<double>::infinity()};
    double minY{std::numeric_limits<double>::infinity()};
    double maxX{-std::numeric_limits<double>::infinity()};
    double maxY{-std::numeric_limits<double>::infinity()};

    bool isValid() const {
        return minX <= maxX && minY <= maxY &&
               minX != std::numeric_limits<double>::infinity() &&
               maxX != -std::numeric_limits<double>::infinity();
    }

    void expand(const ShapePoint& pt) {
        if (pt.x < minX) minX = pt.x;
        if (pt.x > maxX) maxX = pt.x;
        if (pt.y < minY) minY = pt.y;
        if (pt.y > maxY) maxY = pt.y;
    }

    void expand(const ShapeBoundingBox& other) {
        if (!other.isValid()) return;
        if (other.minX < minX) minX = other.minX;
        if (other.maxX > maxX) maxX = other.maxX;
        if (other.minY < minY) minY = other.minY;
        if (other.maxY > maxY) maxY = other.maxY;
    }

    double width() const {
        return isValid() ? (maxX - minX) : 0.0;
    }

    double height() const {
        return isValid() ? (maxY - minY) : 0.0;
    }

    ShapePoint center() const {
        return ShapePoint{(minX + maxX) * 0.5, (minY + maxY) * 0.5};
    }

    bool intersects(const ShapeBoundingBox& other) const {
        if (!isValid() || !other.isValid()) return false;
        return !(other.minX > maxX || other.maxX < minX ||
                 other.minY > maxY || other.maxY < minY);
    }
};

enum class ShapeType {
    Unknown = 0,
    Point,
    MultiPoint,
    Polyline,
    Polygon
};

QString shapeTypeToString(ShapeType type);

struct AttributeField {
    QString name;
    QString typeName;
    int width{0};
    int decimals{0};
};

struct ShapePart {
    std::vector<ShapePoint> points;
};

struct ShapeFeature {
    int id{-1};
    ShapeType type{ShapeType::Unknown};
    std::vector<ShapePart> parts;
    ShapeBoundingBox bbox;
    std::vector<QString> attributes;

    int totalVertices() const {
        int count = 0;
        for (const auto& part : parts) {
            count += static_cast<int>(part.points.size());
        }
        return count;
    }
};

struct ShapeDataset {
    QString filePath;
    ShapeType primaryType{ShapeType::Unknown};
    ShapeBoundingBox bbox;
    std::vector<ShapeFeature> features;
    std::vector<AttributeField> fields;
    bool hasAttributes{false};
    int totalFeatureCount{0};
    int totalVertexCount{0};

    QString typeDisplayName() const {
        return shapeTypeToString(primaryType);
    }
};

struct LoadResult {
    bool success{false};
    QString errorMessage;
    std::shared_ptr<ShapeDataset> dataset;
};

} // namespace Core
