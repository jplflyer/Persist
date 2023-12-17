#pragma once

#include <QWidget>
#include "DataModel.h"

namespace Ui {
class GeneratorForm;
}

class GeneratorForm : public QWidget
{
    Q_OBJECT

public:
    explicit GeneratorForm(DataModel &, DataModel::Generator::Pointer gen, QWidget *parent = nullptr);
    ~GeneratorForm();

    DataModel::Generator::Pointer getGenerator() const { return generator; }
    void reload();

private slots:
    void typeSelected(int index);
    void basePathChanged(const QString &arg1);
    void classPathChanged(const QString &arg1);
    void authTableSelected(int index);

private:
    void showForSQL();
    void showForCPP();
    void showForCPP_DBAccess();
    void showForJava();

    Ui::GeneratorForm *ui;
    DataModel & model;
    DataModel::Generator::Pointer generator;
    std::string userTable;
};
