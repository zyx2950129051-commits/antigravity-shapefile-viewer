#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <memory>
#include "ShapeData.h"

class QLabel;
class QPushButton;
class QProgressBar;

namespace UI {
class ShapeCanvas;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openShapefile(const QString& filePath);
    ShapeCanvas* canvas() const { return m_canvas; }
    bool isLoading() const { return m_isLoading; }

private slots:
    void onSelectFileClicked();
    void onFitToWindowClicked();
    void onAsyncLoadFinished();
    void onMouseGeoPositionChanged(double x, double y);
    void onZoomLevelChanged(double zoomRatio);

private:
    void setupUi();
    void updateFileInfoPanel(const Core::ShapeDataset* dataset);

private:
    ShapeCanvas* m_canvas{nullptr};

    QPushButton* m_btnOpen{nullptr};
    QPushButton* m_btnFit{nullptr};
    QProgressBar* m_loadingProgress{nullptr};

    QLabel* m_lblFileName{nullptr};
    QLabel* m_lblType{nullptr};
    QLabel* m_lblFeatureCount{nullptr};
    QLabel* m_lblVertexCount{nullptr};

    QLabel* m_statusMsg{nullptr};
    QLabel* m_statusCoords{nullptr};
    QLabel* m_statusZoom{nullptr};

    bool m_isLoading{false};
    QFutureWatcher<Core::LoadResult> m_loadWatcher;
};

} // namespace UI
