#include "MainWindow.h"
#include "ShapeCanvas.h"
#include "ShapefileReader.h"
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

namespace UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    connect(&m_loadWatcher, &QFutureWatcher<Core::LoadResult>::finished,
            this, &MainWindow::onAsyncLoadFinished);
}

MainWindow::~MainWindow() {
    if (m_loadWatcher.isRunning()) {
        m_loadWatcher.waitForFinished();
    }
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Shapefile 查看器 (ShapefileViewer) - 0.1.0"));
    resize(1024, 720);
    setMinimumSize(800, 500);

    // Modern clean stylesheet
    setStyleSheet(R"(
        QMainWindow {
            background-color: #F0F2F5;
        }
        QToolBar {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E0E0E0;
            padding: 8px 12px;
            spacing: 12px;
        }
        QPushButton {
            background-color: #1976D2;
            color: #FFFFFF;
            font-weight: bold;
            font-size: 13px;
            border-radius: 4px;
            padding: 6px 16px;
            border: none;
        }
        QPushButton:hover {
            background-color: #1565C0;
        }
        QPushButton:pressed {
            background-color: #0D47A1;
        }
        QPushButton:disabled {
            background-color: #BDBDBD;
            color: #757575;
        }
        QPushButton#btnFit {
            background-color: #455A64;
        }
        QPushButton#btnFit:hover {
            background-color: #37474F;
        }
        QStatusBar {
            background-color: #FFFFFF;
            border-top: 1px solid #E0E0E0;
            color: #424242;
            font-size: 12px;
        }
    )");

    // Toolbar
    QToolBar* toolBar = addToolBar(QStringLiteral("主要工具栏"));
    toolBar->setMovable(false);
    toolBar->setFloatable(false);

    m_btnOpen = new QPushButton(QStringLiteral("📂 选择 SHP 文件"), this);
    m_btnFit = new QPushButton(QStringLiteral("🔍 适应窗口"), this);
    m_btnFit->setObjectName("btnFit");

    m_loadingProgress = new QProgressBar(this);
    m_loadingProgress->setRange(0, 0); // Indeterminate busy spinner
    m_loadingProgress->setFixedHeight(16);
    m_loadingProgress->setFixedWidth(120);
    m_loadingProgress->setVisible(false);

    // Information labels in toolbar
    m_lblFileName = new QLabel(QStringLiteral("未选择文件"), this);
    m_lblFileName->setStyleSheet("font-weight: bold; color: #37474F; font-size: 13px;");

    m_lblType = new QLabel(QStringLiteral("类型: -"), this);
    m_lblType->setStyleSheet("color: #616161; font-size: 12px;");

    m_lblFeatureCount = new QLabel(QStringLiteral("要素数: 0"), this);
    m_lblFeatureCount->setStyleSheet("color: #616161; font-size: 12px;");

    m_lblVertexCount = new QLabel(QStringLiteral("顶点数: 0"), this);
    m_lblVertexCount->setStyleSheet("color: #616161; font-size: 12px;");

    toolBar->addWidget(m_btnOpen);
    toolBar->addWidget(m_btnFit);
    toolBar->addSeparator();
    toolBar->addWidget(m_lblFileName);
    toolBar->addSeparator();
    toolBar->addWidget(m_lblType);
    toolBar->addWidget(m_lblFeatureCount);
    toolBar->addWidget(m_lblVertexCount);
    toolBar->addWidget(m_loadingProgress);

    // Central Canvas
    m_canvas = new ShapeCanvas(this);
    setCentralWidget(m_canvas);

    // Status Bar
    QStatusBar* status = statusBar();
    m_statusMsg = new QLabel(QStringLiteral("就绪"), this);
    m_statusCoords = new QLabel(QStringLiteral("坐标: -"), this);
    m_statusCoords->setMinimumWidth(220);
    m_statusZoom = new QLabel(QStringLiteral("缩放: 100%"), this);
    m_statusZoom->setMinimumWidth(90);

    status->addWidget(m_statusMsg, 1);
    status->addPermanentWidget(m_statusCoords);
    status->addPermanentWidget(m_statusZoom);

    // Connect signals
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::onSelectFileClicked);
    connect(m_btnFit, &QPushButton::clicked, this, &MainWindow::onFitToWindowClicked);

    connect(m_canvas, &ShapeCanvas::mouseGeoPositionChanged,
            this, &MainWindow::onMouseGeoPositionChanged);
    connect(m_canvas, &ShapeCanvas::zoomLevelChanged,
            this, &MainWindow::onZoomLevelChanged);
}

void MainWindow::onSelectFileClicked() {
    if (m_isLoading) return;

    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择 ESRI Shapefile 文件"),
        QString(),
        QStringLiteral("Shapefile (*.shp);;所有文件 (*.*)")
    );

    if (filePath.isEmpty()) return;

    openShapefile(filePath);
}

void MainWindow::openShapefile(const QString& filePath) {
    if (m_isLoading) return;

    m_isLoading = true;
    m_btnOpen->setEnabled(false);
    m_btnFit->setEnabled(false);
    m_loadingProgress->setVisible(true);
    m_statusMsg->setText(QStringLiteral("正在解析 Shapefile: %1 ...").arg(QFileInfo(filePath).fileName()));

    // Run ShapefileReader in background thread via QtConcurrent
    QFuture<Core::LoadResult> future = QtConcurrent::run([filePath]() {
        return Core::ShapefileReader::load(filePath);
    });

    m_loadWatcher.setFuture(future);
}

void MainWindow::onAsyncLoadFinished() {
    m_isLoading = false;
    m_btnOpen->setEnabled(true);
    m_btnFit->setEnabled(true);
    m_loadingProgress->setVisible(false);

    Core::LoadResult result = m_loadWatcher.result();

    if (result.success && result.dataset) {
        m_canvas->setDataset(result.dataset);
        updateFileInfoPanel(result.dataset.get());
        m_statusMsg->setText(QStringLiteral("成功加载：%1（要素数：%2，顶点数：%3）")
            .arg(QFileInfo(result.dataset->filePath).fileName())
            .arg(result.dataset->totalFeatureCount)
            .arg(result.dataset->totalVertexCount));
    } else {
        // Retain current canvas content on failure
        m_statusMsg->setText(QStringLiteral("加载失败：%1").arg(result.errorMessage));
        if (!qEnvironmentVariableIsSet("QT_TEST_OFFSCREEN")) {
            QMessageBox::warning(
                this,
                QStringLiteral("Shapefile 打开失败"),
                QStringLiteral("无法读取指定文件：\n\n%1").arg(result.errorMessage)
            );
        }
    }
}

void MainWindow::updateFileInfoPanel(const Core::ShapeDataset* dataset) {
    if (!dataset) {
        m_lblFileName->setText(QStringLiteral("未选择文件"));
        m_lblType->setText(QStringLiteral("类型: -"));
        m_lblFeatureCount->setText(QStringLiteral("要素数: 0"));
        m_lblVertexCount->setText(QStringLiteral("顶点数: 0"));
        return;
    }

    m_lblFileName->setText(QFileInfo(dataset->filePath).fileName());
    m_lblType->setText(QStringLiteral("类型: %1").arg(dataset->typeDisplayName()));
    m_lblFeatureCount->setText(QStringLiteral("要素数: %1").arg(dataset->totalFeatureCount));
    m_lblVertexCount->setText(QStringLiteral("顶点数: %1").arg(dataset->totalVertexCount));
}

void MainWindow::onFitToWindowClicked() {
    if (m_canvas && m_canvas->hasData()) {
        m_canvas->fitToWindow();
        m_statusMsg->setText(QStringLiteral("视图已重置并自适应窗口。"));
    }
}

void MainWindow::onMouseGeoPositionChanged(double x, double y) {
    m_statusCoords->setText(QStringLiteral("坐标: X=%1, Y=%2")
        .arg(x, 0, 'f', 4)
        .arg(y, 0, 'f', 4));
}

void MainWindow::onZoomLevelChanged(double zoomRatio) {
    int percentage = static_cast<int>(std::round(zoomRatio * 100.0));
    m_statusZoom->setText(QStringLiteral("缩放: %1%").arg(percentage));
}

} // namespace UI
