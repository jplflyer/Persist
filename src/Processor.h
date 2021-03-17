#pragma once

#include <string>
#include "DataModel.h"

/**
 * This is the DataModeler's main processor.
 */
class Processor
{
private:
    std::string		fileName;
    DataModel		model;

    std::string		cppDirName = "./gensrc";
    std::string		cppStubDirName;
    std::string		cppIncludePath;

    std::string		sqlFileName = "./db.sql";

public:
    Processor & setFileName(const std::string &value);
    Processor & setCppDirName(const std::string &value) { cppDirName = value; return *this; }
    Processor & setCppStubDirName(const std::string &value) { cppStubDirName = value; return *this; }
    Processor & setCppIncludePath(const std::string &value) { cppIncludePath = value; return *this; }
    Processor & setSQLFileName(const std::string &value) { sqlFileName = value; return *this; }

    DataModel::Table::Pointer specifyTable(const std::string &value);
    DataModel::Column::Pointer specifyColumn(DataModel::Table & table, const std::string &value);

    void fixReferences();
    void writeModel();
    void generate();
};

