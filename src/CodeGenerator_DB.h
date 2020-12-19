#pragma once

#include <iostream>

#include "CodeGenerator.h"

/**
 * This generates DB access code. You'll need to subclass it for specific
 * SQL libraries.
 */
class CodeGenerator_DB: public CodeGenerator
{
public:
    CodeGenerator_DB();

    void generate(DataModel &) override;

private:
    void generateH(DataModel::Table &);
    void generateCPP(DataModel::Table &);

    void generateCPP_ReadAll(DataModel::Table &, std::ostream &);
    void generateCPP_ParseAll(DataModel::Table &, std::ostream &);
    void generateCPP_ParseOne(DataModel::Table &, std::ostream &);
    void generateCPP_DoInsert(DataModel::Table &, std::ostream &);
    void generateCPP_DoUpdate(DataModel::Table &, std::ostream &);
};

