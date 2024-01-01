#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <QFileDialog>
#include <QTableWidget>

#include <showlib/CommonUsing.h>

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Configuration.h"

using Column = DataModel::Column;
using Table = DataModel::Table;
using Generator = DataModel::Generator;

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

    tWidget = ui->generatorsTable;
    tWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tWidget->setColumnCount(2);
    QStringList genHeaders {"Generator Type", "Description"};
    tWidget->setHorizontalHeaderLabels(genHeaders);
    tWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Do this in case I save the MainWindow.ui with a different tab selected.
    ui->tabWidget->setCurrentIndex(0);

    // Connections for menu actions
    connect(ui->actionNew,   &QAction::triggered,              this, &MainWindow::createNewModel);
    connect(ui->actionOpen,  &QAction::triggered,              this, &MainWindow::openModel);
    connect(ui->actionSave,  &QAction::triggered,              this, &MainWindow::saveModel);
    connect(ui->actionClose, &QAction::triggered,              this, &MainWindow::closeModel);

    // Global fields
    connect(ui->nameTF,      &QLineEdit::textChanged,          this, &MainWindow::modelNameChanged);

    // The tables
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::tablesDoubleClicked);
    connect(ui->newTablePB,  &QPushButton::clicked,            this, &MainWindow::createTable);

    // The Generators.
    connect(ui->generatorsTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::genDoubleClicked);
    connect(ui->newGeneratorPB,  &QPushButton::clicked,            this, &MainWindow::createGenerator);
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
    std::filesystem::path path(fileName);

    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
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

            std::filesystem::path absolute = std::filesystem::absolute(path);
            Configuration::singleton().pushRecent(absolute.string()).save();
            fixRecents();
        }

        showTables();
        showGenerators();
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
void MainWindow::createNewModel()
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
void MainWindow::openModel()
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
void MainWindow::saveModel()
{
    if (modelFileName.length() == 0) {
        QString fName = QFileDialog::getSaveFileName(this, tr("Save Data Model"), ".", tr("JSON files (*.json)"));
        if (fName.length() > 0) {
            modelFileName = fName.toStdString();
        }
    }
    if (modelFileName.length() > 0) {
        std::ofstream ofs {modelFileName};
        JSON json = model.toJSON();
        ofs << json.dump(2);
        model.markClean();
    }
}

/**
 * Close the open model.
 */
void MainWindow::closeModel()
{
}

void MainWindow::fixButtons() {
}

//======================================================================
// Slots related to global fields.
//======================================================================

/**
 * Name of the model has been changed.
 */
void MainWindow::modelNameChanged(const QString &)
{
    string name = ui->nameTF->text().toStdString();
    if (name.length() > 0 && model.getName() != name) {
        model.setName(name);
    }
}

//======================================================================
// Slots related to the Tables table.
//======================================================================

/**
 * Double-clicked a row in the Tables table. Pop up the TableForm for it.
 */
void MainWindow::tablesDoubleClicked(int row, int ) {
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

    connect(newForm, &TableForm::tableChanged, this, &MainWindow::tableUpdated);
    newForm->show();
}

/**
 * This table has been changed.
 */
void
MainWindow::tableUpdated(DataModel::Table::Pointer ) {
    model.sortTables();
    showTables();
}

/**
 * Create a new table.
 */
void MainWindow::createTable()
{
    Table::Pointer newTable = std::make_shared<Table>();
    newTable->setName("NewTable");
    newTable->setDbName("new_table");
    model.pushTable(newTable);
    showTables();

    TableForm * newForm = new TableForm(model, newTable);
    tableForms.push_back(newForm);

    connect(newForm, &TableForm::tableChanged, this, &MainWindow::tableUpdated);
    newForm->show();
}

//======================================================================
// Slots related to the Generators table.
//======================================================================

/**
 * Display the list of tables.
 */
void
MainWindow::showGenerators() {
    QTableWidget * table = ui->generatorsTable;
    int rowIndex = 0;

    const Generator::Vector genList = model.getGenerators();
    table->setRowCount(genList.size());
    for (const Generator::Pointer & gPtr: genList) {
        table->setItem(rowIndex, 0, new QTableWidgetItem(QString::fromStdString(gPtr->getName())));
        table->setItem(rowIndex, 1, new QTableWidgetItem(QString::fromStdString(gPtr->getDescription())));
        ++rowIndex;
    }

}
/**
 * Double-clicked a row in the Tables table. Pop up the TableForm for it.
 */
void MainWindow::genDoubleClicked(int row, int ) {
    const Generator::Vector & generators = model.getGenerators();

    if (static_cast<size_t>(row) > generators.size()) {
        return;
    }

    Generator::Pointer gen = generators[row];
    for (GeneratorForm *tForm: generatorForms) {
        if (tForm->getGenerator() == gen) {
            tForm->reload();
            tForm->show();
            tForm->raise();
            tForm->activateWindow();
            return;
        }
    }

    GeneratorForm * newForm = new GeneratorForm(model, gen);
    generatorForms.push_back(newForm);
    connect(newForm, &GeneratorForm::generatorChanged, this, &MainWindow::generatorChanged);

    newForm->show();
}

/**
 * Create a new table.
 */
void MainWindow::createGenerator()
{
    Generator::Pointer newGenerator = std::make_shared<Generator>();
    model.pushGenerator(newGenerator);
    showGenerators();

    GeneratorForm * newForm = new GeneratorForm(model, newGenerator);
    generatorForms.push_back(newForm);
    connect(newForm, &GeneratorForm::generatorChanged, this, &MainWindow::generatorChanged);

    newForm->show();
}

/**
 * The generator was changed.
 */
void MainWindow::generatorChanged(DataModel::Generator::Pointer) {
    showGenerators();
}
