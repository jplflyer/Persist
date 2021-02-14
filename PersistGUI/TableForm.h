#pragma once

#include <QWidget>

#include <DataModel.h>

namespace Ui {
class TableForm;
}

class TableForm : public QWidget
{
    Q_OBJECT

public:
    explicit TableForm(DataModel &, DataModel::Table::Pointer, QWidget *parent = nullptr);
    ~TableForm();

    DataModel::Table::Pointer getTable() const { return table; }

signals:
    void tableChanged(DataModel::Table::Pointer);

private slots:
    // Changing table name.
    void on_tableNameTF_textChanged(const QString &arg1);
    void on_dbTableNameTF_textChanged(const QString &arg1);

    // Add a column.
    void on_addBtn_clicked();

    // Selecting from the table.
    void on_columnsTable_cellClicked(int row, int column);
    void on_columnsTable_cellDoubleClicked(int row, int column);

    // Changing column info.
    void on_nameTF_textChanged(const QString &arg1);
    void on_dbNameTF_textChanged(const QString &arg1);
    void on_datatypeCB_currentIndexChanged(int index);
    void on_primaryKeyCB_stateChanged(int arg1);
    void on_nullableCB_stateChanged(int arg1);
    void on_indexCB_stateChanged(int arg1);
    void on_foreignKeyCB_stateChanged(int arg1);
    void on_lengthTF_textChanged(const QString &arg1);
    void on_precisionTF_textChanged(const QString &arg1);
    void on_scaleTF_textChanged(const QString &arg1);
    void on_referenceTableCB_currentIndexChanged(int index);
    void on_referenceColumnCB_currentIndexChanged(int index);

    void on_finderCB_stateChanged(int arg1);

private:
    Ui::TableForm *ui;

    DataModel & model;
    DataModel::Table::Pointer table;
    DataModel::Column::Pointer selectedColumn = nullptr;
    int selectedColumnIndex = -1;

    void displayColumn(int colIndex, DataModel::Column &col);

    void showLength();
    void showPrecision();
    void showPossibleReferenceTables(DataModel::Table::Pointer);
    void showPossibleReferenceColumns(DataModel::Table &, DataModel::Column::Pointer selectPtr = nullptr);
};

