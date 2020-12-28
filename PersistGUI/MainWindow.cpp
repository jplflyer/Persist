#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <QFileDialog>

#include "MainWindow.h"
#include "ui_MainWindow.h"

using std::string;

/**
 * Constructor.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fixButtons();
}

/**
 * Destructor.
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * Load this model.
 */
void MainWindow::load(const std::string &fileName) {
    if (std::filesystem::exists(fileName) && std::filesystem::is_regular_file(fileName)) {
        std::ifstream configFile(fileName);
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        string contents = buffer.str();

        if (contents.at(0) == '{') {
            JSON json = JSON::parse(buffer.str());
            model.fromJSON(json);
        }
    }
}

/**
 * Create a new model.
 */
void MainWindow::on_actionNew_triggered()
{
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
}

/**
 * Close the open model.
 */
void MainWindow::on_actionClose_triggered()
{
}

void MainWindow::fixButtons() {

}
