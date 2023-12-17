#include <showlib/CommonUsing.h>
#include "GeneratorForm.h"
#include "ui_GeneratorForm.h"

using Table = DataModel::Table;
using Generator = DataModel::Generator;

/**
 * Constructor.
 */
GeneratorForm::GeneratorForm(DataModel &m, DataModel::Generator::Pointer gen, QWidget *parent)
    :	QWidget(parent),
        ui(new Ui::GeneratorForm),
        model(m),
        generator(gen)

{
    ui->setupUi(this);

    for ( auto const & [key, value]: generator->getOptions()) {
        if (key == "userTable") {
            userTable = value;
        }
    }

    if (generator->getName().empty()) {
        generator->setName( DataModel::Generator::NAME_SQL);
    }

    // Add the possible types of generators
    ui->typeCB->addItem( DataModel::Generator::NAME_SQL );
    ui->typeCB->addItem( DataModel::Generator::NAME_CPP );
    ui->typeCB->addItem( DataModel::Generator::NAME_CPP_DBACCESS );
    ui->typeCB->addItem( DataModel::Generator::NAME_JAVA );

    // Connect the slots.
    connect(ui->typeCB,               &QComboBox::currentIndexChanged,  this, &GeneratorForm::typeSelected);
    connect(ui->basePathTF,           &QLineEdit::textChanged,          this, &GeneratorForm::basePathChanged);
    connect(ui->classPathTF,          &QLineEdit::textChanged,          this, &GeneratorForm::classPathChanged);
    connect(ui->authorizationTableCB, &QComboBox::currentIndexChanged,  this, &GeneratorForm::authTableSelected);

    reload();

    //----------------------------------------------------------------------
    // Handle the type.
    //----------------------------------------------------------------------
    if (generator->getName() == Generator::NAME_SQL) {
        ui->typeCB->setCurrentIndex(0);
        typeSelected(0);
    }
    else if (generator->getName() == Generator::NAME_SQL) {
        ui->typeCB->setCurrentIndex(1);
        typeSelected(1);
    }
    else if (generator->getName() == Generator::NAME_SQL) {
        ui->typeCB->setCurrentIndex(2);
        typeSelected(2);
    }
    else if (generator->getName() == Generator::NAME_SQL) {
        ui->typeCB->setCurrentIndex(3);
        typeSelected(3);
    }

    //----------------------------------------------------------------------
    // The other fields.
    //----------------------------------------------------------------------
    ui->basePathTF->setText( QString::fromStdString( generator->getOutputBasePath() ) );
    ui->classPathTF->setText( QString::fromStdString( generator->getOutputClassPath() ) );
}

/**
 * Destructor.
 */
GeneratorForm::~GeneratorForm()
{
    delete ui;
}

/**
 * Rebuild the list of possible tables.
 */
void GeneratorForm::reload() {
    ui->authorizationTableCB->clear();

    int index = 0;
    for (const Table::Pointer & table: model.getTables()) {
        string tName = table->getName();
        ui->authorizationTableCB->addItem( QString::fromStdString(tName) );
        if (userTable == tName) {
            ui->authorizationTableCB->setCurrentIndex(index);
        }
        else if (userTable.empty() && (tName == "Member" || tName == "User")) {
            userTable = tName;
            generator->setOption("userTable", tName);
            ui->authorizationTableCB->setCurrentIndex(index);
        }
        ++index;
    }
}

void GeneratorForm::showForSQL() {
    ui->basePathTF->setEnabled(true);
    ui->basePathHelp->setText("The name of the output file. This can be relative to the current location when DataModeler --gen is run.");

    ui->classPathL->hide();
    ui->classPathTF->hide();
    ui->classPathHelp->hide();

    ui->authorizationTableL->hide();
    ui->authorizationTableCB->hide();
    ui->authorizationTableHelp->hide();
}

void GeneratorForm::showForCPP() {
    ui->basePathTF->setEnabled(true);
    ui->basePathHelp->setText("The top of the C++ source tree. This can be relative to the current location when DataModeler --gen is run.");

    ui->classPathL->show();
    ui->classPathTF->show();
    ui->classPathHelp->show();
    ui->classPathHelp->setText("A subdirectory under the base path.");

    ui->authorizationTableL->hide();
    ui->authorizationTableCB->hide();
    ui->authorizationTableHelp->hide();
}

void GeneratorForm::showForCPP_DBAccess() {
    ui->basePathTF->setEnabled(true);
    ui->basePathHelp->setText("The top of the C++ source tree. This can be relative to the current location when DataModeler --gen is run.");

    ui->classPathL->show();
    ui->classPathTF->show();
    ui->classPathHelp->show();
    ui->classPathHelp->setText("A subdirectory under the base path. Should match the value from the C++ generator.");

    ui->authorizationTableL->hide();
    ui->authorizationTableCB->hide();
    ui->authorizationTableHelp->hide();
}

void GeneratorForm::showForJava() {
    ui->basePathTF->setEnabled(true);
    ui->basePathHelp->setText("The top of the Java source tree. This can be relative to the current location when DataModeler --gen is run.");

    ui->classPathL->show();
    ui->classPathTF->show();
    ui->classPathHelp->show();
    ui->classPathHelp->setText("A Java package name such as com.example.myproject.");

    ui->authorizationTableL->show();
    ui->authorizationTableCB->show();
    ui->authorizationTableHelp->show();
    ui->authorizationTableHelp->setText("Select the table used for authorization. This is usually something like Member or User.");
}

/**
 * They selected a type of generator. Based on which one they selected, we disable some fields and change the help text.
 */
void GeneratorForm::typeSelected(int index) {
    switch (index) {
        case 0: generator->setName(Generator::NAME_SQL); showForSQL(); break;
        case 1: generator->setName(Generator::NAME_CPP); showForCPP(); break;
        case 2: generator->setName(Generator::NAME_CPP_DBACCESS); showForCPP_DBAccess(); break;
        case 3: generator->setName(Generator::NAME_JAVA); showForJava(); break;
    }
}

/**
 * They changed the base path.
 */
void GeneratorForm::basePathChanged(const QString &newPath) {
    cout << "basePathChanged to: " << newPath.toStdString() << endl;
}

/**
 * They changed the class path.
 */
void GeneratorForm::classPathChanged(const QString &newPath) {
    cout << "classPathChanged to: " << newPath.toStdString() << endl;
}

/**
 * They selected a table from the list of tables to use as the User/Member table for authentication.
 * This table will receive additional generated code to support Spring Web.
 */
void GeneratorForm::authTableSelected(int index) {
    cout << "authTableSelected: " << index << endl;
}

