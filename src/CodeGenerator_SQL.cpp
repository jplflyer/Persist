#include <iostream>
#include <fstream>

#include <showlib/CommonUsing.h>
#include "CodeGenerator_SQL.h"

/**
 * Constructor.
 */
CodeGenerator_SQL::CodeGenerator_SQL(DataModel &m, Generator::Pointer genInfo)
    : CodeGenerator("CodeGenerator_SQL", m, genInfo)
{
}

/**
 * This constructor is used when we're subclassed.
 */
CodeGenerator_SQL::CodeGenerator_SQL(const std::string & _name, DataModel &_model, Generator::Pointer genInfo)
    : CodeGenerator(_name, _model, genInfo)
{
}

/**
 * Generate our output file.
 */
void
CodeGenerator_SQL::generate() {
    generateTo(generatorInfo->getOutputBasePath());
}

/**
 * Generate our output file. We broke this out of generate() so we can subclass
 * (for Flyway) and specify an alternate location.
 */
void
CodeGenerator_SQL::generateTo(const string &filename) {
    std::ofstream ofs{ filename };

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

    //======================================================================
    // Foreign keys and indexes.
    //======================================================================
    ofs << endl;
    for (const Table::Pointer & table: model.getTables()) {
        generateForeignKeys(ofs, *table);
    }

    ofs << endl;
    for (const Table::Pointer & table: model.getTables()) {
        generateIndexes(ofs, *table);
    }

    ofs << endl;
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
CodeGenerator_SQL::generateForTable(std::ofstream &ofs, const Table &table) {
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

        if (needComma) {
            ofs << "," << endl;
        }
        generateDefinitionFor(ofs, *column);

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

}

/**
 * We're in either a "CREATE TABLE" or "ALTER TABLE foo ADD COLUMN" and we want the rest.
 */
std::ofstream & CodeGenerator_SQL::generateDefinitionFor(std::ofstream &ofs, const Column &column) {
    DataType dt = column.getDataType();
    ofs << "        " << column.getDbName() << " " << toString(dt);

    if (dataTypeHasLength(dt) && column.getLength() > 0) {
        ofs << "(" << column.getLength() << ")";
    }

    if (dataTypeHasPrecision(dt) && column.getPrecisionP() > 0) {
        ofs << "(" << column.getPrecisionP();
        if (column.getPrecisionS() > 0) {
            ofs << ", " << column.getPrecisionS();
        }
        ofs << ")";
    }

    if (column.getIsPrimaryKey()) {
        ofs << " PRIMARY KEY";
    }
    if (!column.getNullable()) {
        ofs << " NOT NULL";
    }


    if (column.getIsPrimaryKey() && !dataTypeIsSerial(column.getDataType()) ) {
        Table::Pointer table = column.getOurTable().lock();
        string sequenceName = table->getDbName() + "_" + column.getDbName() + "_seq";

        ofs << " DEFAULT nextval('" << sequenceName << "')";
    }

    return ofs;
}


/**
 * ALTER TABLE child_table
 * ADD CONSTRAINT constraint_name
 * FOREIGN KEY (fk_columns)
 * REFERENCES parent_table (parent_key_columns)
 * [ ON DELETE CASCADE ];
 */
void CodeGenerator_SQL::generateForeignKeys(std::ofstream &ofs, const DataModel::Table &table) {
    for (const Column::Pointer &column: table.getColumns()) {
        Column::Pointer references = column->getReferences();
        if (references != nullptr) {
            ofs << "ALTER TABLE " << table.getDbName() << " ADD CONSTRAINT "
                << table.getDbName() << "_" << column->getDbName()
                << " FOREIGN KEY (" << column->getDbName() << ")"
                << " REFERENCES " << references->fullName(true) << " ON DELETE CASCADE;"
                << endl;
        }
    }
}

/**
 * Generate CREATE INDEX for this table.
 */
void CodeGenerator_SQL::generateIndexes(std::ofstream &ofs, const DataModel::Table &table) {
    //======================================================================
    // Add requested indexes.
    //======================================================================
    for (const Column::Pointer &column: table.getColumns()) {
        if (!column->getIsPrimaryKey() && column->getWantIndex()) {
            ofs << "   CREATE INDEX ON " << table.getDbName() << " (" << column->getDbName() << ");" << endl;
        }
    }
}

