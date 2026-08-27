#include "ShapefileReader.h"
#include <shapefil.h>
#include <QFileInfo>
#include <QDir>
#include <QByteArray>

namespace Core {

ShapeType ShapefileReader::mapShpType(int shpType) {
    switch (shpType) {
        case SHPT_POINT:
        case SHPT_POINTZ:
        case SHPT_POINTM:
            return ShapeType::Point;

        case SHPT_MULTIPOINT:
        case SHPT_MULTIPOINTZ:
        case SHPT_MULTIPOINTM:
            return ShapeType::MultiPoint;

        case SHPT_ARC:
        case SHPT_ARCZ:
        case SHPT_ARCM:
            return ShapeType::Polyline;

        case SHPT_POLYGON:
        case SHPT_POLYGONZ:
        case SHPT_POLYGONM:
            return ShapeType::Polygon;

        default:
            return ShapeType::Unknown;
    }
}

LoadResult ShapefileReader::load(const QString& filePath) {
    LoadResult result;

    if (filePath.trimmed().isEmpty()) {
        result.errorMessage = QStringLiteral("文件路径不能为空。");
        return result;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        result.errorMessage = QStringLiteral("文件不存在：%1").arg(filePath);
        return result;
    }

    if (!fileInfo.isFile()) {
        result.errorMessage = QStringLiteral("指定路径不是有效文件：%1").arg(filePath);
        return result;
    }

    if (fileInfo.suffix().compare(QStringLiteral("shp"), Qt::CaseInsensitive) != 0) {
        result.errorMessage = QStringLiteral("不是标准的 Shapefile 格式（必须以 .shp 为后缀）。");
        return result;
    }

    // Prepare UTF-8 path string
    QByteArray utf8Path = QDir::toNativeSeparators(fileInfo.absoluteFilePath()).toUtf8();

    SHPHandle hSHP = nullptr;

#if defined(_WIN32) || defined(WIN32)
    SAHooks sHooks;
    SASetupUtf8Hooks(&sHooks);
    hSHP = SHPOpenLL(utf8Path.constData(), "rb", &sHooks);
#else
    hSHP = SHPOpen(utf8Path.constData(), "rb");
#endif

    if (!hSHP) {
        result.errorMessage = QStringLiteral("无法打开或解析 Shapefile 文件，文件可能损坏或权限不足。");
        return result;
    }

    int nEntities = 0;
    int nShapeType = 0;
    double adfMinBound[4] = {0, 0, 0, 0};
    double adfMaxBound[4] = {0, 0, 0, 0};

    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    if (nShapeType == SHPT_MULTIPATCH) {
        SHPClose(hSHP);
        result.errorMessage = QStringLiteral("暂不支持 MultiPatch 复杂三维面要素格式。");
        return result;
    }

    ShapeType primaryType = mapShpType(nShapeType);
    if (primaryType == ShapeType::Unknown && nEntities > 0) {
        SHPClose(hSHP);
        result.errorMessage = QStringLiteral("未知的 Shapefile 几何类型代码（代码：%1）。").arg(nShapeType);
        return result;
    }

    if (nEntities <= 0) {
        SHPClose(hSHP);
        result.errorMessage = QStringLiteral("Shapefile 中没有要素记录（要素数量为 0）。");
        return result;
    }

    // Check and open associated .dbf file
    QString baseNoExt = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName();
    QString dbfPath = baseNoExt + ".dbf";
    if (!QFile::exists(dbfPath)) {
        dbfPath = baseNoExt + ".DBF";
    }

    // Check .cpg for encoding
    QString cpgEncoding;
    QString cpgPath = baseNoExt + ".cpg";
    if (!QFile::exists(cpgPath)) {
        cpgPath = baseNoExt + ".CPG";
    }
    if (QFile::exists(cpgPath)) {
        QFile cpgFile(cpgPath);
        if (cpgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            cpgEncoding = QString::fromUtf8(cpgFile.readAll()).trimmed().toUpper();
            cpgFile.close();
        }
    }

    DBFHandle hDBF = nullptr;
    if (QFile::exists(dbfPath)) {
        QByteArray utf8Dbf = QDir::toNativeSeparators(dbfPath).toUtf8();
#if defined(_WIN32) || defined(WIN32)
        hDBF = DBFOpenLL(utf8Dbf.constData(), "rb", &sHooks);
#else
        hDBF = DBFOpen(utf8Dbf.constData(), "rb");
#endif
    }

    auto decodeString = [&](const char* rawStr) -> QString {
        if (!rawStr || *rawStr == '\0') return QString();
        QByteArray bytes(rawStr);

        if (cpgEncoding.contains(QStringLiteral("UTF-8")) || cpgEncoding.contains(QStringLiteral("UTF8"))) {
            return QString::fromUtf8(bytes).trimmed();
        }

        // Try UTF-8 first
        QString utf8Str = QString::fromUtf8(bytes);
        if (!utf8Str.contains(QChar::ReplacementCharacter)) {
            return utf8Str.trimmed();
        }

        // Fallback to local 8-bit string
        return QString::fromLocal8Bit(bytes).trimmed();
    };

    auto dataset = std::make_shared<ShapeDataset>();
    dataset->filePath = fileInfo.absoluteFilePath();
    dataset->primaryType = primaryType;

    // Read DBF fields metadata
    int nFields = 0;
    int nRecords = 0;
    if (hDBF) {
        nFields = DBFGetFieldCount(hDBF);
        nRecords = DBFGetRecordCount(hDBF);
        dataset->fields.reserve(static_cast<size_t>(nFields));

        for (int f = 0; f < nFields; ++f) {
            char szFieldName[20] = {0};
            int nWidth = 0;
            int nDecimals = 0;
            DBFFieldType fType = DBFGetFieldInfo(hDBF, f, szFieldName, &nWidth, &nDecimals);

            AttributeField field;
            field.name = decodeString(szFieldName);
            if (field.name.isEmpty()) {
                field.name = QStringLiteral("字段_%1").arg(f + 1);
            }
            field.width = nWidth;
            field.decimals = nDecimals;

            switch (fType) {
                case FTString:
                    field.typeName = QStringLiteral("文本 (String)");
                    break;
                case FTInteger:
                    field.typeName = QStringLiteral("整数 (Integer)");
                    break;
                case FTDouble:
                    field.typeName = QStringLiteral("浮点 (Double)");
                    break;
                case FTLogical:
                    field.typeName = QStringLiteral("布尔 (Logical)");
                    break;
                case FTDate:
                    field.typeName = QStringLiteral("日期 (Date)");
                    break;
                default:
                    field.typeName = QStringLiteral("其他");
                    break;
            }
            dataset->fields.push_back(std::move(field));
        }
        dataset->hasAttributes = (nFields > 0);
    }

    dataset->features.reserve(static_cast<size_t>(nEntities));
    int totalVertices = 0;

    for (int i = 0; i < nEntities; ++i) {
        SHPObject* psObject = SHPReadObject(hSHP, i);
        if (!psObject) {
            continue;
        }

        // Skip NULL shapes
        if (psObject->nSHPType == SHPT_NULL || psObject->nVertices <= 0) {
            SHPDestroyObject(psObject);
            continue;
        }

        ShapeType featType = mapShpType(psObject->nSHPType);
        if (featType == ShapeType::Unknown) {
            SHPDestroyObject(psObject);
            continue;
        }

        ShapeFeature feat;
        feat.id = psObject->nShapeId >= 0 ? psObject->nShapeId : i;
        feat.type = featType;

        // Read attributes for this feature
        if (hDBF && i < nRecords) {
            feat.attributes.reserve(static_cast<size_t>(nFields));
            for (int f = 0; f < nFields; ++f) {
                if (DBFIsAttributeNULL(hDBF, i, f)) {
                    feat.attributes.push_back(QStringLiteral("-"));
                } else {
                    const char* valStr = DBFReadStringAttribute(hDBF, i, f);
                    feat.attributes.push_back(decodeString(valStr));
                }
            }
        }

        if (featType == ShapeType::Point) {
            ShapePart part;
            ShapePoint pt{psObject->padfX[0], psObject->padfY[0]};
            part.points.push_back(pt);
            feat.bbox.expand(pt);
            feat.parts.push_back(std::move(part));
        } else if (featType == ShapeType::MultiPoint) {
            ShapePart part;
            part.points.reserve(static_cast<size_t>(psObject->nVertices));
            for (int v = 0; v < psObject->nVertices; ++v) {
                ShapePoint pt{psObject->padfX[v], psObject->padfY[v]};
                part.points.push_back(pt);
                feat.bbox.expand(pt);
            }
            feat.parts.push_back(std::move(part));
        } else {
            // Polyline or Polygon
            int numParts = psObject->nParts;
            if (numParts <= 0) {
                numParts = 1;
            }

            feat.parts.reserve(static_cast<size_t>(numParts));

            for (int p = 0; p < numParts; ++p) {
                int startIndex = (psObject->panPartStart && p < psObject->nParts) ? psObject->panPartStart[p] : 0;
                int endIndex = (psObject->panPartStart && (p + 1 < psObject->nParts)) ? psObject->panPartStart[p + 1] : psObject->nVertices;

                if (startIndex < 0) startIndex = 0;
                if (endIndex > psObject->nVertices) endIndex = psObject->nVertices;
                if (startIndex >= endIndex) continue;

                ShapePart part;
                part.points.reserve(static_cast<size_t>(endIndex - startIndex));
                for (int v = startIndex; v < endIndex; ++v) {
                    ShapePoint pt{psObject->padfX[v], psObject->padfY[v]};
                    part.points.push_back(pt);
                    feat.bbox.expand(pt);
                }
                if (!part.points.empty()) {
                    feat.parts.push_back(std::move(part));
                }
            }
        }

        totalVertices += feat.totalVertices();
        dataset->bbox.expand(feat.bbox);
        dataset->features.push_back(std::move(feat));

        SHPDestroyObject(psObject);
    }

    SHPClose(hSHP);
    if (hDBF) {
        DBFClose(hDBF);
    }

    if (dataset->features.empty()) {
        result.errorMessage = QStringLiteral("文件中未发现任何有效的二维几何要素。");
        return result;
    }

    dataset->totalFeatureCount = static_cast<int>(dataset->features.size());
    dataset->totalVertexCount = totalVertices;

    result.success = true;
    result.dataset = dataset;
    return result;
}

} // namespace Core
