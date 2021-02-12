#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <QFileDialog>
#include <QTableWidget>

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Configuration.h"

using std::cout;
using std::endl;
using std::string;

using Column = DataModel::Column;
using Table = DataModel::Table;

using namespace ShowLib;

/**
 * Constructor.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fixRecents();
    fixButtons();
    QTableWidget * tWidget = ui->tableWidget;
    tWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tWidget->setColumnCount(4);
    QStringList headers {"Class Name", "Database Table Name", "Number of Columns", "Actions"};
    tWidget->setHorizontalHeaderLabels(headers);
    tWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

/**
 * Destructor.
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * Setup the Open Recents file menu action.
 */
void
MainWindow::fixRecents() {
    if (recentFilesMenu == nullptr) {
        recentFilesMenu = new QMenu(tr("Open Recent"), this);
        ui->menuFile->insertMenu(ui->actionClose, recentFilesMenu);
    }

    const StringVector & recents = Configuration::singleton().getRecents();
    size_t countRecents = recents.size();
    size_t countActions = recentFileActions.size();

    for (size_t index = 0; index < countRecents; ++index) {
        QAction * thisAction = (index < countActions) ? recentFileActions.at(index) : nullptr;
        if (thisAction == nullptr) {
            thisAction = new QAction(this);
            recentFileActions.push_back(thisAction);
            recentFilesMenu->addAction(thisAction);
            ++countActions;
            connect(thisAction, &QAction::triggered, [=](){ loadRecent(index); });
        }

        thisAction->setVisible(true);
        thisAction->setText(QString::fromStdString(*recents.at(index)));
    }

    // Hide any extras that exist.
    for (size_t index = countRecents; index < countActions; ++index) {
        recentFileActions.at(index)->setVisible(false);
    }
}

/**
 * Open Recent was called.
 */
void
MainWindow::loadRecent(size_t index) {
    const StringVector & recents = Configuration::singleton().getRecents();

    if (index < recents.size()) {
        load(*recents.at(index));
    }
}

/**
 * Load this model.
 */
void MainWindow::load(const std::string fileName) {
    cout << "Load: " << fileName << endl;
    if (std::filesystem::exists(fileName) && std::filesystem::is_regular_file(fileName)) {
        model.clear();

        modelFileName = fileName;
        std::ifstream configFile(fileName);
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        string contents = buffer.str();

        if (contents.at(0) == '{') {
            JSON json = JSON::parse(buffer.str());
            model.fromJSON(json);
            model.fixReferences();
            model.sortTables();
            model.sortAllColumns();

            ui->nameTF->setText(QString::fromStdString(model.getName()));

            Configuration::singleton().pushRecent(fileName).save();
            fixRecents();
        }

        showTables();
    }
}

/**
 * Display the list of tables.
 */
void
MainWindow::showTables() {
    QTableWidget * table = ui->tableWidget;
    int rowIndex = 0;

    table->setRowCount(model.getTables().size());
    for (const Table::Pointer & tPtr: model.getTables()) {
        QString numColumns = QString::fromStdString( std::to_string(tPtr->getColumns().size()) );

        table->setItem(rowIndex, 0, new QTableWidgetItem(QString::fromStdString(tPtr->getName())));
        table->setItem(rowIndex, 1, new QTableWidgetItem(QString::fromStdString(tPtr->getDbName())));
        table->setItem(rowIndex, 2, new QTableWidgetItem(numColumns));
        ++rowIndex;
    }

}

/**
 * Create a new model.
 */
void MainWindow::on_actionNew_triggered()
{
    if (model.getIsDirty()) {
        MainWindow *w = new MainWindow;
        w->show();
    }
    else {
        Table::Vector tables = model.getTables();
        tables.erase(tables.begin(), tables.end());
        modelFileName = "";
        showTables();
    }
}

/**
 * Open an existing model.
 */
void MainWindow::on_actionOpen_triggered()
{
    QString fName = QFileDialog::getOpenFileName(this, tr("Open Data Model"), ".", tr("JSON files (*.json)"));
    if (fName.length() > 0) {
        if (model.getIsDirty()) {
            MainWindow *w = new MainWindow;
            w->show();
            w->load(fName.toStdString());
        }
        else {
            load(fName.toStdString());
        }
    }
}

/**
 * Save the existing model.
 */
void MainWindow::on_actionSave_triggered()
{
    if (modelFileName.length() == 0) {
        QString fName = QFileDialog::getSaveFileName(this, tr("Save Data Model"), ".", tr("JSON files (*.json)"));
        if (fName.length() > 0) {
            modelFileName = fName.toStdString();
        }
    }
    if (modelFileName.length() > 0) {
        std::ofstream ofs {modelFileName};
        JSON json;
        model.toJSON(json);
        ofs << json.dump(2);
        model.markClean();
    }
}

/**
 * Close the open model.
 */
void MainWindow::on_actionClose_triggered()
{
}

void MainWindow::fixButtons() {
}

/**
 * Double-clicked a row.
 */
void MainWindow::on_tableWidget_cellDoubleClicked(int row, int ) {
    Table::Pointer table = model.getTables()[row];
    for (TableForm *tForm: tableForms) {
        if (tForm->getTable() == table) {
            tForm->show();
            tForm->raise();
            tForm->activateWindow();
            return;
        }
    }

    TableForm * newForm = new TableForm(model, table);
    tableForms.push_back(newForm);

    connect(newForm, &TableForm::tableChanged, this, &MainWindow::tableChanged);
    newForm->show();
}

/**
 * This table has been changed.
 */
void
MainWindow::tableChanged(DataModel::Table::Pointer table) {
    model.sortTables();
    showTables();
}

/**
 * Create a new table.
 */
void MainWindow::on_newTablePB_clicked()
{
    Table::Pointer newTable = std::make_shared<Table>();
    newTable->setName("NewTable");
    newTable->setDbName("new_table");
    model.pushTable(newTable);
    showTables();

    TableForm * newForm = new TableForm(model, newTable);
    tableForms.push_back(newForm);

    connect(newForm, &TableForm::tableChanged, this, &MainWindow::tableChanged);
    newForm->show();
}

void MainWindow::on_nameTF_textChanged(const QString &)
{
    string name = ui->nameTF->text().toStdString();
    if (name.length() > 0 && model.getName() != name) {
        model.setName(name);
    }
}
