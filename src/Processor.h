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

    std::string		srcDirName;

public:
    Processor & setFileName(const std::string &value);
    Processor & setSrcDirName(const std::string &value) { srcDirName = value; return *this; }

    DataModel::Table::Pointer specifyTable(const std::string &value);
    DataModel::Column::Pointer specifyColumn(DataModel::Table & table, const std::string &value);

    void writeModel();
    void generate();
};

