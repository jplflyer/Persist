#pragma once

#include <vector>

#include <QMainWindow>

#include "DataModel.h"
#include "TableForm.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionClose_triggered();
    void on_tableWidget_cellDoubleClicked(int row, int column);
    void on_newTablePB_clicked();
    void tableChanged(DataModel::Table::Pointer);

private:
    Ui::MainWindow *ui;

    DataModel model;
    std::vector<TableForm *> tableForms;
    std::string modelFileName;

    void fixButtons();
    void load(const std::string &);

    void showTables();
};
