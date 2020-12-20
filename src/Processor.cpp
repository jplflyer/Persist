#include <iostream>
#include <fstream>

#include <showlib/FileUtilities.h>
#include <showlib/StringUtils.h>

#include "CodeGenerator_CPP.h"
#include "CodeGenerator_SQL.h"
#include "CodeGenerator_DB.h"
#include "Processor.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using namespace ShowLib;

/**
 * Working with this file. If it exists, read it.
 */
Processor &
Processor::setFileName(const std::string &value) {
    fileName = value;

    string contents = FileUtilities::readFile(fileName);
    if (contents.length() > 0) {
        JSON json = JSON::parse(contents);
        model.fromJSON(json);
        if (!model.fixReferences()) {
            exit(2);
        }
    }

    return *this;
}


/**
 * We want to create / update this table.
 */
DataModel::Table::Pointer
Processor::specifyTable(const std::string &tableName)
{
    DataModel::Table::Pointer table = model.findTable(tableName);

    if (table == nullptr) {
        table = model.createTable(tableName);
    }

    return table;
}

/**
 * We want to create / update this table.
 *
 * colData is of form name[,type]
 * type can be one of the simple types or:
 * 		varchar[(length)]
 * 		numeric[(precision[,scale])]
 */
DataModel::Column::Pointer
Processor::specifyColumn(DataModel::Table & table, const std::string &colData) {
    DataModel::Column::Pointer column;
    DataModel::Column::DataType dt;
    int length = 0;
    int precision = 0;
    int scale = 0;
    bool haveDatatype = false;

    std::vector<string> parts = split(colData, ":");
    string colName = parts.at(0);

    if (colName.length() == 0) {
        cerr << "Specify Column requires a column name: " << colData << endl;
        exit(EXIT_FAILURE);
    }

    if (parts.size() == 2) {
        string dtStr = parts.at(1);
        std::vector<string> dtParts = splitWithParens(dtStr, ",");
        haveDatatype = true;

        dt = toDataType(dtParts.at(0));
        if (dataTypeHasLength(dt)) {
            if (dtParts.size() == 2) {
                length = std::stoi(dtParts.at(1));
            }
        }
        else if (dataTypeHasPrecision(dt) && dtParts.size() > 1) {
            precision = std::stoi(dtParts.at(1));
            if (dtParts.size() > 2) {
                scale = std::stoi(dtParts.at(2));
            }
        }
    }

    column = table.findColumn(colName);

    if (column == nullptr) {
        if (!haveDatatype) {
            cerr << "Column not known and no data type provided: " << colData << endl;
            exit(EXIT_FAILURE);
        }
        column = table.createColumn(colName, dt);
    }
    column->setDataType(dt)
            .setLength(length)
            .setPrecision(precision, scale);

    return column;
}

/**
 * Fix the references.
 */
void
Processor::fixReferences() {
    model.fixReferences();
}

/**
 *
 */
void
Processor::writeModel() {
    std::ofstream ofs{fileName};
    JSON json = model.getJSON();
    ofs << json.dump(2) << endl;
}

/**
 *
 */
void
Processor::generate() {
    CodeGenerator_SQL sqlGen;
    sqlGen.outputFileName = sqlFileName;
    sqlGen.generate(model);

    CodeGenerator_CPP cppGen;
    cppGen.outputFileName = cppDirName;
    cppGen.cppStubDirName = cppStubDirName.length() > 0 ? cppStubDirName : cppDirName;
    cppGen.generate(model);

    CodeGenerator_DB dbGen;
    dbGen.outputFileName = cppDirName;
    dbGen.generate(model);
}
