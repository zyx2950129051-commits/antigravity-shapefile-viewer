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
class AttributeTableDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    void openShapefile(const QString& filePath);
    bool isLoading() const { return m_loadWatcher.isRunning(); }
    ShapeCanvas* canvas() const { return m_canvas; }
    std::shared_ptr<Core::ShapeDataset> currentDataset() const { return m_currentDataset; }

    void deleteFeature(int featureIndex);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onOpenFileClicked();
    void onFitWindowClicked();
    void onOpenAttributesClicked();
    void onDeleteSelectedClicked();
    void onAsyncLoadFinished();
    void onFeatureSelected(int featureIndex);

private:
    void setupUi();
    void setupToolbar();
    void setupStatusBar();
    void updateLayerInfoDisplay();

private:
    ShapeCanvas* m_canvas{nullptr};
    AttributeTableDialog* m_attrDialog{nullptr};

    QPushButton* m_btnOpen{nullptr};
    QPushButton* m_btnFit{nullptr};
    QPushButton* m_btnAttributes{nullptr};
    QPushButton* m_btnDeleteSelected{nullptr};

    QLabel* m_lblLayerInfo{nullptr};
    QLabel* m_lblCoordinates{nullptr};
    QLabel* m_lblZoom{nullptr};
    QLabel* m_statusMsg{nullptr};
    QProgressBar* m_progressBar{nullptr};

    QFutureWatcher<Core::LoadResult> m_loadWatcher;
    std::shared_ptr<Core::ShapeDataset> m_currentDataset;
};

} // namespace UI
