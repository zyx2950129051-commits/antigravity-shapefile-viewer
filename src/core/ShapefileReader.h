#pragma once

#include "ShapeData.h"
#include <QString>

namespace Core {

class ShapefileReader {
public:
    ShapefileReader() = delete;
    ~ShapefileReader() = delete;

    /**
     * @brief Load and parse an ESRI Shapefile (*.shp)
     * @param filePath Path to the .shp file (UTF-8 encoded)
     * @return LoadResult containing success status, errorMessage, or dataset
     */
    static LoadResult load(const QString& filePath);

private:
    static ShapeType mapShpType(int shpType);
};

} // namespace Core
