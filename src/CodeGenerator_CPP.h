#pragma once

#include "CodeGenerator.h"

class CodeGenerator_CPP: public CodeGenerator
{
public:
    CodeGenerator_CPP();

    void generate(DataModel &) override;

    bool wantJSON = true;

private:
    void generateH(DataModel::Table &);
    void generateCPP(DataModel::Table &);

    bool isInt(const std::string &);
    bool isDouble(const std::string &);
    bool isBool(const std::string &);
    bool isString(const std::string &);
};

