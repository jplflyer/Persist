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
    CodeGenerator_DB(DataModel &_model, DataModel::Generator::Pointer genInfo);

    void generate() override;

    std::string cppStubDirName;
    std::string cppIncludePath;

private:
    // These methods generate the base classes into the stubDir.
    void generateH(DataModel::Table &);
    void generateCPP(DataModel::Table &);

    void generateCPP_ReadAll(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_ParseAll(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_ParseOne(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_DoInsert(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_DoUpdate(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_DeleteWithId(DataModel::Table &, std::ostream &, const std::string &myClassName);

    void generateCPP_ParameterList(DataModel::Table &, std::ostream &, bool forUpdate, int startIndex = 1);
    void generateCPP_FieldArguments(DataModel::Table &, std::ostream &);

    void generateH_FromForeignKeys(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_FromForeignKeys(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateH_FromMapFiles(DataModel::Table &, std::ostream &, const std::string &myClassName);
    void generateCPP_FromMapFiles(DataModel::Table &, std::ostream &, const std::string &myClassName);

    void generateCPP_ThisMap(DataModel::Table &,
                             std::ostream &,
                             DataModel::Table &,
                             const std::string &myClassName);

    // This generates subclasses only if they don't already exist.
    void generateConcreteH(DataModel::Table &);
    void generateConcreteCPP(DataModel::Table &);
};

