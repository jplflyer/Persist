#pragma once

#include <showlib/StringVector.h>
#include "CodeGenerator.h"

class CodeGenerator_Java: public CodeGenerator
{
public:
    CodeGenerator_Java(DataModel &, DataModel::Generator::Pointer genInfo);
    void generate() override;

private:
    void generatePOJO(DataModel::Table::Pointer table);
    void generateRepository(DataModel::Table::Pointer table);
    void generateForeignKey(std::ofstream & ofs, DataModel::Column::Pointer column);

    std::string javaType(DataModel::Column::DataType dt);

    std::string userTableName;
    ShowLib::StringVector extendsList;
    ShowLib::StringVector implementsList;
    bool withSpringTags = true;

    std::string slashedClassPath;
};
