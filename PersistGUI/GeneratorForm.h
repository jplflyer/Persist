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

signals:
    void generatorChanged(DataModel::Generator::Pointer);

private slots:
    void typeSelected(int index);
    void descriptionChanged(const QString &);
    void basePathChanged(const QString &);
    void classPathChanged(const QString &);
    void authTableSelected(int);
    void extendsChanged(const QString &);
    void implementsChanged(const QString &);
    void withSpringTagsChanged(int);

private:
    void showForSQL();
    void showForCPP();
    void showForCPP_DBAccess();
    void showForJava();
    void showForFlyway();

    Ui::GeneratorForm *ui;
    DataModel & model;
    DataModel::Generator::Pointer generator;
    std::string userTable;
};
