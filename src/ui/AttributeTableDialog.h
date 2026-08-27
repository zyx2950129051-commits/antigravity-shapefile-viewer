#pragma once

#include <QDialog>
#include <QAbstractTableModel>
#include <memory>
#include <vector>
#include "ShapeData.h"

class QTableView;
class QLineEdit;
class QLabel;
class QPushButton;

namespace UI {

class AttributeTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit AttributeTableModel(std::shared_ptr<Core::ShapeDataset> dataset, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setFilterText(const QString& filter);
    int mapVisibleRowToFeatureIndex(int visibleRow) const;
    int mapFeatureIndexToVisibleRow(int featureIndex) const;
    void refresh();

private:
    std::shared_ptr<Core::ShapeDataset> m_dataset;
    std::vector<int> m_visibleFeatureIndices;
};

class AttributeTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit AttributeTableDialog(std::shared_ptr<Core::ShapeDataset> dataset, QWidget* parent = nullptr);
    ~AttributeTableDialog() override = default;

    void selectFeature(int featureIndex);
    void refreshData();

signals:
    void featureSelectedInTable(int featureIndex);
    void requestDeleteFeature(int featureIndex);

private slots:
    void onSearchTextChanged(const QString& text);
    void onTableSelectionChanged();
    void onLocateClicked();
    void onDeleteClicked();

private:
    void setupUi();
    void updateCountLabel();

private:
    std::shared_ptr<Core::ShapeDataset> m_dataset;
    AttributeTableModel* m_model{nullptr};
    QTableView* m_tableView{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QLabel* m_lblCount{nullptr};
    QPushButton* m_btnLocate{nullptr};
    QPushButton* m_btnDelete{nullptr};
};

} // namespace UI
