#include <showlib/CommonUsing.h>
#include <showlib/StringUtils.h>

#include "DatabaseForm.h"
#include "ui_DatabaseForm.h"

using Database = DataModel::Database;

/**
 * Constructor.
 */
DatabaseForm::DatabaseForm(DataModel & dm, DataModel::Database::Pointer db, QWidget *parent)
:	QWidget(parent) ,
    ui(new Ui::DatabaseForm),
    model(dm),
    database(db)
{
    ui->setupUi(this);

    // Add the known drivers, currently only PostgrSql
    ui->driverCB->addItem( DataModel::Database::DRIVER_POSTGRESQL );

    // Connect the slots.
    connect(ui->driverCB, &QComboBox::currentIndexChanged, this, &DatabaseForm::driverSelected);
    connect(ui->envNameTF, &QLineEdit::textChanged, this, &DatabaseForm::nameChanged);
    connect(ui->hostTF, &QLineEdit::textChanged, this, &DatabaseForm::hostChanged);
    connect(ui->portTF, &QLineEdit::textChanged, this, &DatabaseForm::portChanged);
    connect(ui->dbNameTF, &QLineEdit::textChanged, this, &DatabaseForm::dbNameChanged);
    connect(ui->usernameTF, &QLineEdit::textChanged, this, &DatabaseForm::usernameChanged);
    connect(ui->passwordTF, &QLineEdit::textChanged, this, &DatabaseForm::passwordChanged);

    reload();
}

/**
 * Destructor.
 */
DatabaseForm::~DatabaseForm()
{
    delete ui;
}

/**
 * Set incoming values.
 */
void DatabaseForm::reload() {
    string driver = database->getDriver();
    if (driver == Database::DRIVER_POSTGRESQL) {
        ui->driverCB->setCurrentIndex(0);
    }

    ui->envNameTF->setText( QString::fromStdString( database->getEnvName() ));
    ui->hostTF->setText( QString::fromStdString( database->getHost() ));
    ui->portTF->setText( QString::fromStdString( std::to_string(database->getPort()) ));
    ui->dbNameTF->setText( QString::fromStdString( database->getDbName() ));
    ui->usernameTF->setText( QString::fromStdString( database->getUsername() ));
    ui->passwordTF->setText( QString::fromStdString( database->getPassword() ));
}

/**
 * They picked a driver (PostgreSql).
 */
void DatabaseForm::driverSelected(int index) {
    switch (index) {
        case 0: database->setDriver(Database::DRIVER_POSTGRESQL);
    }
    emit databaseChanged(database);
}

/**
 * Environment name changed.
 */
void DatabaseForm::nameChanged(const QString &str) {
    database->setEnvName(str.toStdString());
    emit databaseChanged(database);
}

/**
 * Host name changed.
 */
void DatabaseForm::hostChanged(const QString &str) {
    database->setHost( str.toStdString() );
    emit databaseChanged(database);
}

/**
 * Port changed.
 */
void DatabaseForm::portChanged(const QString &str) {
    string portStr = ShowLib::trim(str.toStdString());
    if (portStr.empty() || !ShowLib::allDigits(portStr)) {
        database->setPort(0);
    }
    else {
        database->setPort( ShowLib::stol(portStr) );
    }

    emit databaseChanged(database);
}

/**
 * Database name changed.
 */
void DatabaseForm::dbNameChanged(const QString &str) {
    database->setDbName( str.toStdString() );
    emit databaseChanged(database);
}

/**
 * Username changed.
 */
void DatabaseForm::usernameChanged(const QString &str) {
    database->setUsername( str.toStdString() );
    emit databaseChanged(database);
}

/**
 * Password changed.
 */
void DatabaseForm::passwordChanged(const QString &str) {
    database->setPassword( str.toStdString() );
    emit databaseChanged(database);
}
