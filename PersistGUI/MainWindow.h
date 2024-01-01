#pragma once

#include <vector>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QList>

#include "DataModel.h"
#include "TableForm.h"
#include "GeneratorForm.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void load(const std::string);

private slots:
    // Menu items
    void createNewModel();
    void openModel();
    void saveModel();
    void closeModel();

    // Global fields
    void modelNameChanged(const QString &);

    // The Tables tab
    void tablesDoubleClicked(int row, int column);
    void createTable();
    void tableUpdated(DataModel::Table::Pointer);

    // The Generators tab
    void genDoubleClicked(int row, int column);
    void createGenerator();
    void generatorChanged(DataModel::Generator::Pointer);

private:
    void fixRecents();
    void fixButtons();
    void loadRecent(size_t index);

    void showTables();
    void showGenerators();

    Ui::MainWindow *ui;
    QMenu * recentFilesMenu = nullptr;
    QList<QAction *> recentFileActions;

    DataModel model;
    std::vector<TableForm *> tableForms;
    std::string modelFileName;

    std::vector<GeneratorForm *> generatorForms;

};
