#pragma once

#include "CodeGenerator.h"

class CodeGenerator_CPP: public CodeGenerator
{
public:
    CodeGenerator_CPP(DataModel &);

    void generate() override;

    std::string cppStubDirName;
    bool wantJSON = true;

private:
    void generateIncludes();

    // This generates the base classes
    void generateH(DataModel::Table &);
    void generateCPP(DataModel::Table &);

    // This generates subclasses only if they don't already exist.
    void generateConcreteH(DataModel::Table &);
    void generateConcreteCPP(DataModel::Table &);

    bool isInt(const std::string &);
    bool isDouble(const std::string &);
    bool isBool(const std::string &);
    bool isString(const std::string &);
};

