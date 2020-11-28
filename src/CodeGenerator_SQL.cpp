#include <iostream>
#include <fstream>

#include "CodeGenerator_SQL.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using Table = DataModel::Table;
using Column = DataModel::Column;
using DataType = DataModel::Column::DataType;

/**
 * Constructor.
 */
CodeGenerator_SQL::CodeGenerator_SQL()
    : CodeGenerator("CodeGenerator_SQL")
{
}

/**
 * Generate our output file.
 */
void
CodeGenerator_SQL::generate(DataModel &model) {
    if (outputFileName.length() == 0) {
        cerr << "CodeGenerator_SQL::generate() with no output file specified." << endl;
        exit(2);
    }

    std::ofstream ofs{outputFileName};

    ofs << "BEGIN;" << endl;

    //======================================================================
    // This script rebuilds the DB. Destroy the existing tables.
    //======================================================================
    for (const Table::Pointer & table: model.getTables()) {
        ofs << "   DROP TABLE IF EXISTS " << table->getDbName() << " CASCADE;" << endl;
    }
    ofs << endl;

    //======================================================================
    // And create new ones.
    //======================================================================
    for (const Table::Pointer & table: model.getTables()) {
        generateForTable(ofs, *table);
    }

    ofs << "COMMIT;" << endl;
}

/**
 * Generate.
 *
 * CREATE TABLE foo(
 *    id Integer PRIMARY KEY NOT NULL,
 *    bar_id Integer REFERENCES bar(id)
 * );
 */
void
CodeGenerator_SQL::generateForTable(std::ofstream &ofs, const DataModel::Table &table) {
    //======================================================================
    // If the primary key is not a Serial type, then we manually create
    // the sequence we'll use.
    //======================================================================
    const Column::Pointer pk = table.findPrimaryKey();
    bool needSequence = pk != nullptr && !dataTypeIsSerial(pk->getDataType());
    string sequenceName;

    if (needSequence) {
        sequenceName = table.getDbName() + "_" + pk->getDbName() + "_seq";
        ofs << "    CREATE SEQUENCE " << sequenceName << ";" << endl;
    }

    //======================================================================
    // Create the table.
    //======================================================================
    ofs << "    CREATE TABLE " << table.getDbName() << "(" << endl;

    bool needComma = false;
    for (const Column::Pointer &column: table.getColumns()) {
        DataType dt = column->getDataType();

        if (needComma) {
            ofs << "," << endl;
        }

        ofs << "        " << column->getDbName() << " " << toString(dt);

        if (dataTypeHasLength(dt) && column->getLength() > 0) {
            ofs << "(" << column->getLength() << ")";
        }

        if (dataTypeHasPrecision(dt) && column->getPrecisionP() > 0) {
            ofs << "(" << column->getPrecisionP();
            if (column->getPrecisionS() > 0) {
                ofs << ", " << column->getPrecisionS();
            }
            ofs << ")";
        }

        if (column->getIsPrimaryKey()) {
            ofs << " PRIMARY KEY";
        }
        if (!column->getNullable()) {
            ofs << " NOT NULL";
        }

        Column::Pointer references = column->getReferences();
        if (references != nullptr) {
            ofs << " REFERENCES " << references->fullName(true) << " ON DELETE CASCADE";
        }

        if (column->getIsPrimaryKey() && needSequence) {
            ofs << " DEFAULT nextval('" << sequenceName << "')";
        }

        needComma = true;
    }

    ofs << endl << "    );" << endl;

    //======================================================================
    // If we created an index, set ownership.
    //======================================================================
    if (needSequence) {
        ofs << "    ALTER SEQUENCE " << sequenceName << " OWNED BY "
            << table.getDbName() << "." << pk->getDbName() << ";" << endl;
    }

    //======================================================================
    // Add requested indexes.
    //======================================================================
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey() && column->getWantIndex()) {
            ofs << "   CREATE INDEX ON " << table.getDbName() << " (" << column->getDbName() << ");" << endl;
        }
    }
    ofs << endl;
}

