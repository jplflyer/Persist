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
    bool withSpringTags = false;
    string extendsStr = "";
    string implementsStr = "";

    ui->setupUi(this);

    for ( auto const & [key, value]: generator->getOptions()) {
        if (key == "userTable") {
            userTable = value;
        }
        else if (key == "withSpringTags") {
            withSpringTags = value == "true";
        }
        else if (key == "extends") {
            extendsStr = value;
        }
        else if (key == "implements") {
            implementsStr = value;
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
    connect(ui->descTF,               &QLineEdit::textChanged,          this, &GeneratorForm::descriptionChanged);
    connect(ui->basePathTF,           &QLineEdit::textChanged,          this, &GeneratorForm::basePathChanged);
    connect(ui->classPathTF,          &QLineEdit::textChanged,          this, &GeneratorForm::classPathChanged);
    connect(ui->authorizationTableCB, &QComboBox::currentIndexChanged,  this, &GeneratorForm::authTableSelected);
    connect(ui->extendsTF,            &QLineEdit::textChanged,          this, &GeneratorForm::extendsChanged);
    connect(ui->implementsTF,         &QLineEdit::textChanged,          this, &GeneratorForm::implementsChanged);
    connect(ui->withSpringTags,       &QCheckBox::stateChanged,         this, &GeneratorForm::withSpringTagsChanged);

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
    ui->withSpringTags->setChecked(withSpringTags);
    ui->extendsTF->setText( QString::fromStdString( extendsStr) );
    ui->implementsTF->setText( QString::fromStdString( implementsStr) );

    ui->descHelp->setText("A meaningful description");
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

/**
 * This generator is for SQL. Set the help and hide the extra fields.
 */
void GeneratorForm::showForSQL() {
    ui->basePathTF->setEnabled(true);
    ui->basePathHelp->setText("The name of the output file. This can be relative to the current location when DataModeler --gen is run.");

    ui->classPathL->hide();
    ui->classPathTF->hide();
    ui->classPathHelp->hide();

    ui->authorizationTableL->hide();
    ui->authorizationTableCB->hide();
    ui->authorizationTableHelp->hide();

    ui->extendsL->hide();
    ui->extendsTF->hide();
    ui->extendsHelp->hide();
    ui->implementsL->hide();
    ui->implementsTF->hide();
    ui->implementsHelp->hide();
    ui->withSpringTags->hide();
}

/**
 * This generator is for C++. Set the help and show/hide the extra fields.
 */
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

    ui->extendsL->hide();
    ui->extendsTF->hide();
    ui->extendsHelp->hide();
    ui->implementsL->hide();
    ui->implementsTF->hide();
    ui->implementsHelp->hide();
    ui->withSpringTags->hide();
}

/**
 * This generator is for C++ DBAccess. Set the help and show/hide the extra fields.
 */
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

    ui->extendsL->hide();
    ui->extendsTF->hide();
    ui->extendsHelp->hide();
    ui->implementsL->hide();
    ui->implementsTF->hide();
    ui->implementsHelp->hide();
    ui->withSpringTags->hide();
}

/**
 * This generator is for Java. Set the help and show/hide the extra fields.
 */
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

    ui->extendsL->show();
    ui->extendsTF->show();
    ui->extendsHelp->show();
    ui->implementsL->show();
    ui->implementsTF->show();
    ui->implementsHelp->show();

    ui->withSpringTags->show();
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
    emit generatorChanged(generator);
}

void GeneratorForm::descriptionChanged(const QString &newDesc) {
    generator->setDescription(newDesc.toStdString());
    emit generatorChanged(generator);
}

/**
 * They changed the base path.
 */
void GeneratorForm::basePathChanged(const QString &newPath) {
    generator->setOutputBasePath(newPath.toStdString());
    emit generatorChanged(generator);
}

/**
 * They changed the class path.
 */
void GeneratorForm::classPathChanged(const QString &newPath) {
    generator->setOutputClassPath(newPath.toStdString());
    emit generatorChanged(generator);
}

/**
 * They selected a table from the list of tables to use as the User/Member table for authentication.
 * This table will receive additional generated code to support Spring Web.
 */
void GeneratorForm::authTableSelected(int index) {
    if (index >= 0) {
        Table::Pointer table = model.getTables().at(index);
        generator->setOption("userTable", table->getName());
        emit generatorChanged(generator);
    }
}

void GeneratorForm::withSpringTagsChanged(int) {
    bool withSpringTags = ui->withSpringTags->isChecked();
    generator->setOption("withSpringTags", withSpringTags ? "true" : "false");
    emit generatorChanged(generator);
}

/**
 * They edited the "Extends" field.
 */
void GeneratorForm::extendsChanged(const QString &str) {
    generator->setOption("extends", str.toStdString());
}


/**
 * They edited the "Implements" field.
 */
void GeneratorForm::implementsChanged(const QString &str) {
    generator->setOption("implements", str.toStdString());
}
