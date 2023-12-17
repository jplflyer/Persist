#pragma once

#include <fstream>

#include "CodeGenerator.h"

class CodeGenerator_SQL: public CodeGenerator
{
public:
    CodeGenerator_SQL(DataModel &, DataModel::Generator::Pointer);

    void generate() override;

private:
    void generateForTable(std::ofstream &, const DataModel::Table &);

    void generateForeignKeys(std::ofstream &, const DataModel::Table &);
    void generateIndexes(std::ofstream &, const DataModel::Table &);
};

