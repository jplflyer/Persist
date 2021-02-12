#pragma once

#include <vector>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QList>

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

    void on_nameTF_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QMenu * recentFilesMenu = nullptr;
    QList<QAction *> recentFileActions;

    DataModel model;
    std::vector<TableForm *> tableForms;
    std::string modelFileName;

    void fixRecents();
    void fixButtons();
    void load(const std::string);
    void loadRecent(size_t index);

    void showTables();
};
