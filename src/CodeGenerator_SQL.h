#pragma once

#include <fstream>

#include "CodeGenerator.h"

class CodeGenerator_SQL: public CodeGenerator
{
public:
    using Table = DataModel::Table;
    using Column = DataModel::Column;
    using Generator = DataModel::Generator;
    using DataType = DataModel::Column::DataType;

    CodeGenerator_SQL(DataModel &, DataModel::Generator::Pointer);
    CodeGenerator_SQL(const std::string & _name, DataModel &_model, Generator::Pointer genInfo);

    void generate() override;

protected:
    void generateTo(const std::string & filename);
    void generateForTable(std::ofstream &, const Table &);

    std::ofstream & generateDefinitionFor(std::ofstream &, const Column &);

    void generateForeignKeys(std::ofstream &, const Table &);
    void generateIndexes(std::ofstream &, const Table &);
};

