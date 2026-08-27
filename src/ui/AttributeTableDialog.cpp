#include "AttributeTableDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>

namespace UI {

AttributeTableModel::AttributeTableModel(std::shared_ptr<Core::ShapeDataset> dataset, QObject* parent)
    : QAbstractTableModel(parent)
    , m_dataset(std::move(dataset))
{
    refresh();
}

void AttributeTableModel::refresh() {
    beginResetModel();
    m_visibleFeatureIndices.clear();
    if (m_dataset) {
        m_visibleFeatureIndices.reserve(m_dataset->features.size());
        for (size_t i = 0; i < m_dataset->features.size(); ++i) {
            m_visibleFeatureIndices.push_back(static_cast<int>(i));
        }
    }
    endResetModel();
}

void AttributeTableModel::setFilterText(const QString& filter) {
    beginResetModel();
    m_visibleFeatureIndices.clear();
    if (!m_dataset) {
        endResetModel();
        return;
    }

    QString trimmed = filter.trimmed();
    if (trimmed.isEmpty()) {
        m_visibleFeatureIndices.reserve(m_dataset->features.size());
        for (size_t i = 0; i < m_dataset->features.size(); ++i) {
            m_visibleFeatureIndices.push_back(static_cast<int>(i));
        }
    } else {
        for (size_t i = 0; i < m_dataset->features.size(); ++i) {
            const auto& feat = m_dataset->features[i];
            bool match = false;
            for (const auto& attr : feat.attributes) {
                if (attr.contains(trimmed, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            if (match) {
                m_visibleFeatureIndices.push_back(static_cast<int>(i));
            }
        }
    }
    endResetModel();
}

int AttributeTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_dataset) return 0;
    return static_cast<int>(m_visibleFeatureIndices.size());
}

int AttributeTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_dataset) return 0;
    // Column 0 is FID, remaining columns are DBF fields
    return static_cast<int>(m_dataset->fields.size()) + 1;
}

QVariant AttributeTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_dataset) return QVariant();

    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_visibleFeatureIndices.size())) {
        return QVariant();
    }

    int featIdx = m_visibleFeatureIndices[row];
    if (featIdx < 0 || featIdx >= static_cast<int>(m_dataset->features.size())) {
        return QVariant();
    }

    const auto& feat = m_dataset->features[featIdx];

    if (role == Qt::DisplayRole) {
        int col = index.column();
        if (col == 0) {
            return feat.id >= 0 ? feat.id : (featIdx + 1);
        }
        int attrCol = col - 1;
        if (attrCol >= 0 && attrCol < static_cast<int>(feat.attributes.size())) {
            return feat.attributes[attrCol];
        }
        return QStringLiteral("-");
    }

    if (role == Qt::TextAlignmentRole) {
        return static_cast<int>((index.column() == 0) ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignVCenter));
    }

    return QVariant();
}

QVariant AttributeTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section == 0) {
            return QStringLiteral("要素ID (FID)");
        }
        int fieldIdx = section - 1;
        if (m_dataset && fieldIdx >= 0 && fieldIdx < static_cast<int>(m_dataset->fields.size())) {
            const auto& f = m_dataset->fields[fieldIdx];
            return QStringLiteral("%1\n[%2]").arg(f.name, f.typeName);
        }
        return QStringLiteral("字段 %1").arg(section);
    }

    if (orientation == Qt::Vertical) {
        return section + 1;
    }

    return QVariant();
}

int AttributeTableModel::mapVisibleRowToFeatureIndex(int visibleRow) const {
    if (visibleRow >= 0 && visibleRow < static_cast<int>(m_visibleFeatureIndices.size())) {
        return m_visibleFeatureIndices[visibleRow];
    }
    return -1;
}

int AttributeTableModel::mapFeatureIndexToVisibleRow(int featureIndex) const {
    for (size_t r = 0; r < m_visibleFeatureIndices.size(); ++r) {
        if (m_visibleFeatureIndices[r] == featureIndex) {
            return static_cast<int>(r);
        }
    }
    return -1;
}

// ---------------- AttributeTableDialog ----------------

AttributeTableDialog::AttributeTableDialog(std::shared_ptr<Core::ShapeDataset> dataset, QWidget* parent)
    : QDialog(parent)
    , m_dataset(std::move(dataset))
{
    setupUi();
}

void AttributeTableDialog::setupUi() {
    setWindowTitle(QStringLiteral("要素属性表 - %1").arg(m_dataset ? m_dataset->filePath : QString()));
    resize(960, 560);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Top Action & Search Bar
    auto topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);

    auto lblSearch = new QLabel(QStringLiteral("🔍 搜索属性:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("输入关键字实时过滤要素属性..."));
    m_searchEdit->setClearButtonEnabled(true);

    m_btnLocate = new QPushButton(QStringLiteral("🎯 定位要素"), this);
    m_btnLocate->setEnabled(false);
    m_btnLocate->setStyleSheet("background-color: #1976D2; color: white; padding: 5px 12px; font-weight: bold; border-radius: 4px;");

    m_btnDelete = new QPushButton(QStringLiteral("🗑️ 删除要素"), this);
    m_btnDelete->setEnabled(false);
    m_btnDelete->setStyleSheet("background-color: #D32F2F; color: white; padding: 5px 12px; font-weight: bold; border-radius: 4px;");

    m_lblCount = new QLabel(this);
    m_lblCount->setStyleSheet("color: #616161; font-weight: bold;");

    topLayout->addWidget(lblSearch);
    topLayout->addWidget(m_searchEdit, 1);
    topLayout->addWidget(m_btnLocate);
    topLayout->addWidget(m_btnDelete);
    topLayout->addWidget(m_lblCount);

    mainLayout->addLayout(topLayout);

    // Table View
    m_tableView = new QTableView(this);
    m_model = new AttributeTableModel(m_dataset, this);
    m_tableView->setModel(m_model);

    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setDefaultSectionSize(26);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setDefaultSectionSize(130);

    mainLayout->addWidget(m_tableView);

    updateCountLabel();

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AttributeTableDialog::onSearchTextChanged);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AttributeTableDialog::onTableSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &AttributeTableDialog::onLocateClicked);
    connect(m_btnLocate, &QPushButton::clicked, this, &AttributeTableDialog::onLocateClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &AttributeTableDialog::onDeleteClicked);
}

void AttributeTableDialog::onSearchTextChanged(const QString& text) {
    m_model->setFilterText(text);
    updateCountLabel();
}

void AttributeTableDialog::onTableSelectionChanged() {
    QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    bool hasSel = !sel.isEmpty();
    m_btnLocate->setEnabled(hasSel);
    m_btnDelete->setEnabled(hasSel);
}

void AttributeTableDialog::onLocateClicked() {
    QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    int featIdx = m_model->mapVisibleRowToFeatureIndex(sel.first().row());
    if (featIdx >= 0) {
        emit featureSelectedInTable(featIdx);
    }
}

void AttributeTableDialog::onDeleteClicked() {
    QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    int featIdx = m_model->mapVisibleRowToFeatureIndex(sel.first().row());
    if (featIdx >= 0) {
        emit requestDeleteFeature(featIdx);
    }
}

void AttributeTableDialog::selectFeature(int featureIndex) {
    if (featureIndex < 0) {
        m_tableView->clearSelection();
        return;
    }

    int visibleRow = m_model->mapFeatureIndexToVisibleRow(featureIndex);
    if (visibleRow >= 0) {
        QModelIndex index = m_model->index(visibleRow, 0);
        m_tableView->selectRow(visibleRow);
        m_tableView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }
}

void AttributeTableDialog::refreshData() {
    m_model->refresh();
    updateCountLabel();
    onTableSelectionChanged();
}

void AttributeTableDialog::updateCountLabel() {
    int total = m_dataset ? static_cast<int>(m_dataset->features.size()) : 0;
    int visible = m_model->rowCount();
    m_lblCount->setText(QStringLiteral("显示: %1 / %2 条记录").arg(visible).arg(total));
}

} // namespace UI
