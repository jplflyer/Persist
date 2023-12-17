#pragma once

#include "CodeGenerator.h"

class CodeGenerator_Java: public CodeGenerator
{
public:
    CodeGenerator_Java(DataModel &, DataModel::Generator::Pointer genInfo);
    void generate() override;

private:
    void generatePOJO(DataModel::Table::Pointer table);
    void generateRepository(DataModel::Table::Pointer table);

    std::string javaType(DataModel::Column::DataType dt);

    std::string userTableName;

    std::string slashedClassPath;
};
