#include <QComboBox>

#include <showlib/StringUtils.h>

#include "TableForm.h"
#include "ui_TableForm.h"

using std::cout;
using std::endl;
using std::string;

using Column = DataModel::Column;
using Table = DataModel::Table;
using DataType = DataModel::Column::DataType;
using DataTypePair = DataModel::Column::DataTypePair;

static const int COL_NAME = 0;
static const int COL_DBNAME = 1;
static const int COL_DATATYPE = 2;
static const int COL_PRECISION = 3;
static const int COL_FLAGS = 4;
static const int COL_REFERENCES = 5;
//static const int COL_ACTIONS = 6;

/**
 * Constructor.
 */
TableForm::TableForm(DataModel &m, DataModel::Table::Pointer tPtr, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TableForm),
    model(m),
    table(tPtr)
{
    ui->setupUi(this);

    QString title = QString::fromStdString( string{"Table: "} + tPtr->getName() );
    this->setWindowTitle(title);

    ui->tableNameTF->setText(QString::fromStdString(tPtr->getName()));
    ui->dbTableNameTF->setText(QString::fromStdString(tPtr->getDbName()));

    QTableWidget * tWidget = ui->columnsTable;
    tWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    tWidget->setColumnCount(7);
    QStringList headers {"Name", "Database Column Name", "DataType", "Precision", "Flags", "Reference", "Actions"};
    tWidget->setHorizontalHeaderLabels(headers);
    tWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    int colIndex = 0;
    tWidget->setRowCount(tPtr->getColumns().size());
    for (const Column::Pointer & col: tPtr->getColumns()) {
        displayColumn(colIndex, *col);
        ++colIndex;
    }

    //----------------------------------------------------------------------
    // Set up for what datatypes we can have.
    //----------------------------------------------------------------------
    std::vector<DataTypePair> & dataTypes = allDataTypes();

    QComboBox &cb = *ui->datatypeCB;
    for (DataTypePair &pair: dataTypes) {
        cb.addItem(QString::fromStdString(pair.first));
    }
}

/**
 * Destructor.
 */
TableForm::~TableForm()
{
    delete ui;
}

void TableForm::displayColumn(int colIndex, DataModel::Column &col) {

    QTableWidget * tWidget = ui->columnsTable;

    tWidget->setRowCount(table->getColumns().size());
    tWidget->setItem(colIndex, COL_NAME, new QTableWidgetItem(QString::fromStdString(col.getName())));
    tWidget->setItem(colIndex, COL_DBNAME, new QTableWidgetItem(QString::fromStdString(col.getDbName())));
    tWidget->setItem(colIndex, COL_DATATYPE, new QTableWidgetItem(QString::fromStdString(toString(col.getDataType()))));
    tWidget->setItem(colIndex, COL_PRECISION, new QTableWidgetItem(QString::fromStdString(col.precisionStr())));
    tWidget->setItem(colIndex, COL_FLAGS, new QTableWidgetItem(QString::fromStdString(col.flagsStr())));

    Column::Pointer refPtr = col.getReferences();
    if (refPtr != nullptr) {
        tWidget->setItem(colIndex, COL_REFERENCES, new QTableWidgetItem(QString::fromStdString(refPtr->fullName())) );
    }
    else {
        tWidget->setItem(colIndex, COL_REFERENCES, new QTableWidgetItem(""));
    }
}

void TableForm::on_columnsTable_cellClicked(int row, int column)
{
    on_columnsTable_cellDoubleClicked(row, column);
}

/**
 * Table double click.
 */
void TableForm::on_columnsTable_cellDoubleClicked(int row, int ) {
    selectedColumnIndex = row;
    selectedColumn = table->getColumns().at(row);

    DataType dt = selectedColumn->getDataType();
    Column::Pointer references = selectedColumn->getReferences();
    Table::Pointer referenceTable = references != nullptr ? references->getOurTable().lock() : nullptr;
    bool wantReferences = referenceTable != nullptr;

    ui->nameTF->setText(QString::fromStdString(selectedColumn->getName()));
    ui->referencePtrTF->setText(QString::fromStdString(selectedColumn->getRefPtrName()));
    ui->dbNameTF->setText(QString::fromStdString(selectedColumn->getDbName()));

    ui->primaryKeyCB->setChecked(selectedColumn->getIsPrimaryKey());
    ui->nullableCB->setChecked(selectedColumn->getNullable());
    ui->indexCB->setChecked(selectedColumn->getWantIndex());
    ui->finderCB->setChecked(selectedColumn->getWantFinder());
    ui->foreignKeyCB->setChecked(wantReferences);
    ui->serializeCB->setChecked(selectedColumn->getSerialize());

    std::vector<DataTypePair> & dataTypes = allDataTypes();
    for (size_t index = 0; index < dataTypes.size(); ++index) {
        if (dataTypes[index].second == dt) {
            ui->datatypeCB->setCurrentIndex(index);
            break;
        }
    }

    //--------------------------------------------------
    // Handle the appearing/disappearing fields.
    //--------------------------------------------------
    showLength();
    showPrecision();

    ui->referenceL->setVisible(wantReferences);
    ui->referenceTableCB->setVisible(wantReferences);
    ui->referenceColumnCB->setVisible(wantReferences);
    ui->referencePtrL->setVisible(wantReferences);
    ui->referencePtrTF->setVisible(wantReferences);

    if (wantReferences) {
        showPossibleReferenceTables(referenceTable);
        showPossibleReferenceColumns(*referenceTable, references);
    }
}

/**
 * Add a new column.
 */
void TableForm::on_addBtn_clicked() {
    Column::Pointer col = table->createColumn("newColumn", Column::DataType::Integer);
    int count = table->getColumns().size();
    selectedColumnIndex = count - 1;

    ui->columnsTable->setRowCount(count);
    displayColumn(selectedColumnIndex, *col);
    on_columnsTable_cellDoubleClicked(selectedColumnIndex, 0);
    model.markDirty();
    emit tableChanged(table);
}

void TableForm::on_tableNameTF_textChanged(const QString &text)
{
    table->setName(text.toStdString());
    model.markDirty();
    emit tableChanged(table);

    QString title = QString::fromStdString( string{"Table: "} + table->getName() );
    this->setWindowTitle(title);
}

void TableForm::on_dbTableNameTF_textChanged(const QString &text)
{
    table->setDbName(text.toStdString());
    model.markDirty();
    emit tableChanged(table);
}

/**
 * Changed the generated class member variable name.
 */
void TableForm::on_nameTF_textChanged(const QString &)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setName(ui->nameTF->text().toStdString());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

void TableForm::on_referencePtrTF_textChanged(const QString &) {
    if (selectedColumn != nullptr) {
        selectedColumn->setRefPtrName(ui->referencePtrTF->text().toStdString());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Changed the SQL table column name.
 */
void TableForm::on_dbNameTF_textChanged(const QString &)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setDbName(ui->dbNameTF->text().toStdString());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Selected a datatype.
 */
void TableForm::on_datatypeCB_currentIndexChanged(int index)
{
    if (selectedColumn != nullptr) {
        DataType dt = allDataTypes().at(index).second;
        selectedColumn->setDataType(dt);
        model.markDirty();

        displayColumn(selectedColumnIndex, *selectedColumn);
        showLength();
        showPrecision();
    }

}

/**
 * Changed ths Is Primary Key checkbox.
 */
void TableForm::on_primaryKeyCB_stateChanged(int)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setIsPrimaryKey(ui->primaryKeyCB->isChecked());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Changed ths Is Nullable checkbox.
 */
void TableForm::on_nullableCB_stateChanged(int)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setNullable(ui->nullableCB->isChecked());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Changed ths Want Index checkbox.
 */
void TableForm::on_indexCB_stateChanged(int)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setWantIndex(ui->indexCB->isChecked());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

void TableForm::on_finderCB_stateChanged(int)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setWantFinder(ui->finderCB->isChecked());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Toggled Serialize.
 */
void TableForm::on_serializeCB_stateChanged(int) {
    if (selectedColumn != nullptr) {
        selectedColumn->setSerialize(ui->serializeCB->isChecked());
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

/**
 * Changed ths Is Foreign Key checkbox.
 */
void TableForm::on_foreignKeyCB_stateChanged(int)
{
    if (selectedColumn != nullptr) {
        bool wantReferences = ui->foreignKeyCB->isChecked();

        ui->referenceL->setVisible(wantReferences);
        ui->referenceTableCB->setVisible(wantReferences);
        ui->referenceColumnCB->setVisible(wantReferences);
        ui->referencePtrL->setVisible(wantReferences);
        ui->referencePtrTF->setVisible(wantReferences);

        if (wantReferences) {
            ui->referenceColumnCB->clear();
            showPossibleReferenceTables(nullptr);
        }
        else {
            selectedColumn->setReferences(nullptr);
            model.markDirty();
        }

        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }

}

void TableForm::on_precisionTF_textChanged(const QString &)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setPrecision(
                    ShowLib::stol(ui->precisionTF->text().toStdString()),
                    ShowLib::stol(ui->scaleTF->text().toStdString())
                    );
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}

void TableForm::on_scaleTF_textChanged(const QString &value)
{
    on_precisionTF_textChanged(value);
}

/**
 * User selected a table.
 */
void TableForm::on_referenceTableCB_currentIndexChanged(int)
{
    string tName = ui->referenceTableCB->currentText().toStdString();
    Table::Pointer tPtr = model.findTable(tName);
    if (tPtr != nullptr) {
        showPossibleReferenceColumns(*tPtr, selectedColumn->getReferences());
    }
}

/**
 * User selected a column.
 */
void TableForm::on_referenceColumnCB_currentIndexChanged(int) {
    if (selectedColumn != nullptr) {
        string tName = ui->referenceTableCB->currentText().toStdString();
        string cName = ui->referenceColumnCB->currentText().toStdString();
        Table::Pointer tPtr = model.findTable(tName);

        if (tPtr != nullptr) {
            Column::Pointer cPtr = tPtr->findColumn(cName);
            if (cPtr != nullptr) {
                selectedColumn->setReferences(cPtr);
                displayColumn(selectedColumnIndex, *selectedColumn);
                model.markDirty();
            }
        }
    }
}

/**
 * They changed the name of the reference column.
 */
void TableForm::on_lengthTF_textChanged(const QString &)
{
    if (selectedColumn != nullptr) {
        selectedColumn->setLength(ShowLib::stol(ui->lengthTF->text().toStdString()));
        displayColumn(selectedColumnIndex, *selectedColumn);
        model.markDirty();
    }
}


void TableForm::showLength() {
    if (selectedColumn != nullptr) {
        DataType dt = selectedColumn->getDataType();
        bool wantLength = dataTypeHasLength(dt);

        ui->lengthL->setVisible(wantLength);
        ui->lengthTF->setVisible(wantLength);

        if (wantLength) {
            ui->lengthTF->setText(QString::fromStdString(std::to_string(selectedColumn->getLength())));
        }
    }
}

void TableForm::showPrecision() {
    if (selectedColumn != nullptr) {
        DataType dt = selectedColumn->getDataType();
        bool wantPrecision = dataTypeHasPrecision(dt);

        ui->precisionL->setVisible(wantPrecision);
        ui->precisionTF->setVisible(wantPrecision);
        ui->scaleL->setVisible(wantPrecision);
        ui->scaleTF->setVisible(wantPrecision);

        if (wantPrecision) {
            ui->precisionTF->setText(QString::fromStdString(std::to_string(selectedColumn->getPrecisionP())));
            ui->scaleTF->setText(QString::fromStdString(std::to_string(selectedColumn->getPrecisionS())));
        }
    }
}

/**
 * Show the possible tables we can refer to.
 */
void TableForm::showPossibleReferenceTables(Table::Pointer referenceTable) {
    int currentTableIndex = 1;

    ui->referenceTableCB->clear();
    ui->referenceTableCB->addItem("");

    for (const Table::Pointer &tPtr: model.getTables()) {
        if (tPtr != table) {
            ui->referenceTableCB->addItem(QString::fromStdString(tPtr->getName()));

            if (tPtr == referenceTable) {
                ui->referenceTableCB->setCurrentIndex(currentTableIndex);
            }
            ++currentTableIndex;
        }
    }
}

/**
 * Display the possible columns they can link to.
 */
void TableForm::showPossibleReferenceColumns(Table & tPtr, Column::Pointer selectPtr) {
    int currentColumnIndex = 1;

    ui->referenceColumnCB->clear();
    ui->referenceColumnCB->addItem("");
    for (const Column::Pointer &cPtr: tPtr.getColumns()) {
        ui->referenceColumnCB->addItem(QString::fromStdString(cPtr->getName()));
        if (cPtr == selectPtr) {
            ui->referenceColumnCB->setCurrentIndex(currentColumnIndex);
        }

        ++currentColumnIndex;
    }
}


