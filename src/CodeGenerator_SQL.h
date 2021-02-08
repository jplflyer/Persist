#pragma once

#include <fstream>

#include "CodeGenerator.h"

class CodeGenerator_SQL: public CodeGenerator
{
private:
    void generateForTable(std::ofstream &, const DataModel::Table &);

    void generateForeignKeys(std::ofstream &, const DataModel::Table &);
    void generateIndexes(std::ofstream &, const DataModel::Table &);

public:
    CodeGenerator_SQL();

    void generate(DataModel &) override;
};

