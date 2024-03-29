#pragma once

#include "CodeGenerator.h"

class CodeGenerator_CPP: public CodeGenerator
{
public:
    CodeGenerator_CPP(DataModel &, DataModel::Generator::Pointer genInfo);

    void generate() override;

    bool wantJSON = true;

private:
    void generateIncludes();

    // This generates the base classes
    void generateH(DataModel::Table &);
    void generateCPP(DataModel::Table &);
    void generateUtilities();
    void generateH_ResolveReferences(std::ostream &, DataModel::Table::Pointer from, DataModel::Table::Pointer to);
    void generateC_ResolveReferences(std::ostream &, DataModel::Table::Pointer from, DataModel::Table::Pointer to);

    void generateH_ForwardReferences(std::ostream &, DataModel::Table &);
    void generateH_FK_Access(std::ostream &, DataModel::Table &);
    void generateH_FK_Storage(std::ostream &, DataModel::Table &);

    void generateC_CommonIncludes(std::ostream &, DataModel::Table &);
    void generateC_FK_Add(std::ostream &, DataModel::Table &, DataModel::Table &, DataModel::Column &);
    void generateC_FK_Remove(std::ostream &, DataModel::Table &, DataModel::Table &, DataModel::Column &);
    void generateC_FK_RemoveAll(std::ostream &, DataModel::Table &, DataModel::Table &, DataModel::Column &);

    // This generates subclasses only if they don't already exist.
    void generateConcreteH(DataModel::Table &);
    void generateConcreteCPP(DataModel::Table &);

    bool isInt(const std::string &);
    bool isDouble(const std::string &);
    bool isBool(const std::string &);
    bool isString(const std::string &);

    std::string cppStubDirName;
    std::string cppIncludePath;
};

