#pragma once

#include <fstream>

#include "CodeGenerator.h"

class CodeGenerator_SQL: public CodeGenerator
{
public:
    CodeGenerator_SQL(DataModel &, DataModel::Generator::Pointer);
    CodeGenerator_SQL(const std::string & _name, DataModel &_model, DataModel::Generator::Pointer genInfo);

    void generate() override;

protected:
    void generateTo(const std::string & filename);
    void generateForTable(std::ofstream &, const DataModel::Table &);

    void generateForeignKeys(std::ofstream &, const DataModel::Table &);
    void generateIndexes(std::ofstream &, const DataModel::Table &);
};

