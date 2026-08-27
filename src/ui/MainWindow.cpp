#include "MainWindow.h"
#include "ShapeCanvas.h"
#include "AttributeTableDialog.h"
#include "ShapefileReader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QKeyEvent>
#include <QtConcurrent/QtConcurrent>

namespace UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    connect(&m_loadWatcher, &QFutureWatcher<Core::LoadResult>::finished,
            this, &MainWindow::onAsyncLoadFinished);
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Shapefile 查看器 (ShapefileViewer) 0.1.0"));
    resize(1080, 720);

    setupToolbar();

    m_canvas = new ShapeCanvas(this);
    setCentralWidget(m_canvas);

    setupStatusBar();

    // Connect Canvas signals
    connect(m_canvas, &ShapeCanvas::mouseGeoPositionChanged, this, [this](double x, double y) {
        m_lblCoordinates->setText(QStringLiteral("坐标: X=%1, Y=%2")
            .arg(x, 0, 'f', 4)
            .arg(y, 0, 'f', 4));
    });

    connect(m_canvas, &ShapeCanvas::zoomLevelChanged, this, [this](double ratio) {
        m_lblZoom->setText(QStringLiteral("缩放: %1%").arg(static_cast<int>(ratio * 100.0)));
    });

    connect(m_canvas, &ShapeCanvas::featureSelected, this, &MainWindow::onFeatureSelected);
}

void MainWindow::setupToolbar() {
    auto toolBar = addToolBar(QStringLiteral("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setStyleSheet("QToolBar { background: #FFFFFF; border-bottom: 1px solid #E0E0E0; spacing: 8px; padding: 6px; }");

    m_btnOpen = new QPushButton(QStringLiteral("📂 选择 SHP 文件"), this);
    m_btnOpen->setStyleSheet("background-color: #1976D2; color: white; padding: 6px 14px; font-weight: bold; border-radius: 4px;");
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    toolBar->addWidget(m_btnOpen);

    m_btnFit = new QPushButton(QStringLiteral("🔍 适应窗口"), this);
    m_btnFit->setEnabled(false);
    m_btnFit->setStyleSheet("padding: 6px 12px; border-radius: 4px;");
    connect(m_btnFit, &QPushButton::clicked, this, &MainWindow::onFitWindowClicked);
    toolBar->addWidget(m_btnFit);

    m_btnAttributes = new QPushButton(QStringLiteral("📊 属性表"), this);
    m_btnAttributes->setEnabled(false);
    m_btnAttributes->setStyleSheet("background-color: #388E3C; color: white; padding: 6px 12px; font-weight: bold; border-radius: 4px;");
    connect(m_btnAttributes, &QPushButton::clicked, this, &MainWindow::onOpenAttributesClicked);
    toolBar->addWidget(m_btnAttributes);

    m_btnDeleteSelected = new QPushButton(QStringLiteral("🗑️ 删除选中要素"), this);
    m_btnDeleteSelected->setEnabled(false);
    m_btnDeleteSelected->setStyleSheet("background-color: #D32F2F; color: white; padding: 6px 12px; font-weight: bold; border-radius: 4px;");
    connect(m_btnDeleteSelected, &QPushButton::clicked, this, &MainWindow::onDeleteSelectedClicked);
    toolBar->addWidget(m_btnDeleteSelected);

    toolBar->addSeparator();

    m_lblLayerInfo = new QLabel(QStringLiteral("未加载图层"), this);
    m_lblLayerInfo->setStyleSheet("color: #424242; font-weight: 500; margin-left: 8px;");
    toolBar->addWidget(m_lblLayerInfo);
}

void MainWindow::setupStatusBar() {
    auto bar = statusBar();
    bar->setStyleSheet("QStatusBar { background: #FAFAFA; border-top: 1px solid #E0E0E0; }");

    m_statusMsg = new QLabel(QStringLiteral("就绪"), this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(160);
    m_progressBar->setMaximumHeight(14);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(false);

    m_lblCoordinates = new QLabel(QStringLiteral("坐标: -"), this);
    m_lblCoordinates->setMinimumWidth(220);

    m_lblZoom = new QLabel(QStringLiteral("缩放: 100%"), this);
    m_lblZoom->setMinimumWidth(90);

    bar->addWidget(m_statusMsg, 1);
    bar->addPermanentWidget(m_progressBar);
    bar->addPermanentWidget(m_lblCoordinates);
    bar->addPermanentWidget(m_lblZoom);
}

void MainWindow::onOpenFileClicked() {
    QString filter = QStringLiteral("ESRI Shapefile (*.shp);;所有文件 (*.*)");
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择 ESRI Shapefile 文件"),
        QString(),
        filter
    );

    if (!filePath.isEmpty()) {
        openShapefile(filePath);
    }
}

void MainWindow::openShapefile(const QString& filePath) {
    if (m_loadWatcher.isRunning()) {
        return;
    }

    if (m_attrDialog) {
        m_attrDialog->close();
        m_attrDialog->deleteLater();
        m_attrDialog = nullptr;
    }

    QFileInfo info(filePath);
    m_statusMsg->setText(QStringLiteral("正在解析 Shapefile: %1 ...").arg(info.fileName()));
    m_progressBar->setVisible(true);
    m_btnOpen->setEnabled(false);

    // Asynchronous loading
    QFuture<Core::LoadResult> future = QtConcurrent::run([filePath]() {
        return Core::ShapefileReader::load(filePath);
    });

    m_loadWatcher.setFuture(future);
}

void MainWindow::onAsyncLoadFinished() {
    m_progressBar->setVisible(false);
    m_btnOpen->setEnabled(true);

    Core::LoadResult result = m_loadWatcher.result();

    if (result.success && result.dataset) {
        m_currentDataset = result.dataset;
        m_canvas->setDataset(m_currentDataset);

        m_btnFit->setEnabled(true);
        m_btnAttributes->setEnabled(true);
        m_btnDeleteSelected->setEnabled(false);

        updateLayerInfoDisplay();
        m_statusMsg->setText(QStringLiteral("加载完成：%1").arg(QFileInfo(m_currentDataset->filePath).fileName()));
    } else {
        // Retain current canvas content on failure
        m_statusMsg->setText(QStringLiteral("加载失败：%1").arg(result.errorMessage));
        bool isTestMode = qApp->property("QT_TEST_MODE").toBool() || qEnvironmentVariableIsSet("QT_TEST_OFFSCREEN");
        if (!isTestMode) {
            QMessageBox::warning(
                this,
                QStringLiteral("Shapefile 打开失败"),
                QStringLiteral("无法读取指定文件：\n\n%1").arg(result.errorMessage)
            );
        }
    }
}

void MainWindow::updateLayerInfoDisplay() {
    if (!m_currentDataset) {
        m_lblLayerInfo->setText(QStringLiteral("未加载图层"));
        m_btnFit->setEnabled(false);
        m_btnAttributes->setEnabled(false);
        m_btnDeleteSelected->setEnabled(false);
        return;
    }

    QFileInfo info(m_currentDataset->filePath);
    QString attrInfo = m_currentDataset->hasAttributes ? QStringLiteral(" | 属性字段: %1个").arg(m_currentDataset->fields.size()) : QStringLiteral(" | 无属性表");
    m_lblLayerInfo->setText(QStringLiteral("图层: %1 | 类型: %2 | 要素: %3 | 顶点: %4%5")
        .arg(info.fileName())
        .arg(m_currentDataset->typeDisplayName())
        .arg(m_currentDataset->totalFeatureCount)
        .arg(m_currentDataset->totalVertexCount)
        .arg(attrInfo));
}

void MainWindow::onFitWindowClicked() {
    m_canvas->fitToWindow();
}

void MainWindow::onOpenAttributesClicked() {
    if (!m_currentDataset) return;

    if (!m_attrDialog) {
        m_attrDialog = new AttributeTableDialog(m_currentDataset, this);
        connect(m_attrDialog, &AttributeTableDialog::featureSelectedInTable, this, [this](int featIdx) {
            m_canvas->centerOnFeature(featIdx);
            onFeatureSelected(featIdx);
        });
        connect(m_attrDialog, &AttributeTableDialog::requestDeleteFeature, this, &MainWindow::deleteFeature);
    }

    if (m_canvas->selectedFeatureIndex() >= 0) {
        m_attrDialog->selectFeature(m_canvas->selectedFeatureIndex());
    }

    m_attrDialog->show();
    m_attrDialog->raise();
    m_attrDialog->activateWindow();
}

void MainWindow::onFeatureSelected(int featureIndex) {
    if (featureIndex >= 0 && m_currentDataset && featureIndex < static_cast<int>(m_currentDataset->features.size())) {
        m_btnDeleteSelected->setEnabled(true);
        const auto& feat = m_currentDataset->features[featureIndex];
        m_statusMsg->setText(QStringLiteral("已选中要素 FID: %1 (%2, 顶点数: %3)")
            .arg(feat.id >= 0 ? feat.id : featureIndex + 1)
            .arg(Core::shapeTypeToString(feat.type))
            .arg(feat.totalVertices()));

        if (m_attrDialog && m_attrDialog->isVisible()) {
            m_attrDialog->selectFeature(featureIndex);
        }
    } else {
        m_btnDeleteSelected->setEnabled(false);
        if (m_currentDataset) {
            m_statusMsg->setText(QStringLiteral("就绪"));
        }
        if (m_attrDialog && m_attrDialog->isVisible()) {
            m_attrDialog->selectFeature(-1);
        }
    }
}

void MainWindow::onDeleteSelectedClicked() {
    int selIdx = m_canvas->selectedFeatureIndex();
    if (selIdx >= 0) {
        deleteFeature(selIdx);
    }
}

void MainWindow::deleteFeature(int featureIndex) {
    if (!m_currentDataset || featureIndex < 0 || featureIndex >= static_cast<int>(m_currentDataset->features.size())) {
        return;
    }

    // Remove feature from dataset
    m_currentDataset->features.erase(m_currentDataset->features.begin() + featureIndex);
    m_currentDataset->totalFeatureCount = static_cast<int>(m_currentDataset->features.size());

    // Recompute total vertices and bounding box
    int totalVerts = 0;
    Core::ShapeBoundingBox newBox;
    for (const auto& f : m_currentDataset->features) {
        totalVerts += f.totalVertices();
        newBox.expand(f.bbox);
    }
    m_currentDataset->totalVertexCount = totalVerts;
    m_currentDataset->bbox = newBox;

    // Clear selection
    m_canvas->setSelectedFeatureIndex(-1);
    m_btnDeleteSelected->setEnabled(false);
    updateLayerInfoDisplay();
    m_statusMsg->setText(QStringLiteral("已成功删除要素 (剩余要素数: %1)").arg(m_currentDataset->totalFeatureCount));

    m_canvas->update();

    if (m_attrDialog) {
        m_attrDialog->refreshData();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        int selIdx = m_canvas->selectedFeatureIndex();
        if (selIdx >= 0) {
            deleteFeature(selIdx);
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

} // namespace UI
