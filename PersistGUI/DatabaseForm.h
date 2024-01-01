#pragma once

#include <QWidget>
#include "DataModel.h"

namespace Ui {
class DatabaseForm;
}

class DatabaseForm : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseForm(DataModel & dm, DataModel::Database::Pointer db, QWidget *parent = nullptr);
    ~DatabaseForm();

    DataModel::Database::Pointer getDatabase() const { return database; }
    void reload();

signals:
    void databaseChanged(DataModel::Database::Pointer);

private slots:
    void driverSelected(int index);
    void nameChanged(const QString &);
    void hostChanged(const QString &);
    void portChanged(const QString &);
    void dbNameChanged(const QString &);
    void usernameChanged(const QString &);
    void passwordChanged(const QString &);

private:
    Ui::DatabaseForm *ui;
    DataModel &model;
    DataModel::Database::Pointer database;
};
