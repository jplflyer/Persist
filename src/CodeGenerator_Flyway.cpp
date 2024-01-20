#include <chrono>
#include <cstdio>
#include <fstream>

#include <showlib/CommonUsing.h>
#include <showlib/FileUtilities.h>
#include <showlib/StringUtils.h>

#include <date/date.h>

#include "DataModel.h"
#include "CodeGenerator_Flyway.h"

/**
 * Constructor. We just pass our arguments through to the superclass.
 */
CodeGenerator_Flyway::CodeGenerator_Flyway(DataModel &m, DataModel::Generator::Pointer genInfo)
    : CodeGenerator_SQL("CodeGenerator_Flyway", m, genInfo)
{

}

/**
 * Generate. We should support some of what we generate as flags, but I'm not going to worry about that yet.
 */
void CodeGenerator_Flyway::generate() {
    generate_ConfigFiles();
    if (generate_Migrations()) {
        saveModel();
    }
}

/**
 * Generate the config files. outputBasePath will be the top level directory, probably relative
 * from the current directory. We'll make sure schema-model exists, and we'll write a new flyway.toml.
 */
void CodeGenerator_Flyway::generate_ConfigFiles() {
    string outName = generatorInfo->getOutputBasePath() + "/flyway.toml";
    string tempName = outName + ".tmp";

    string dbType;
    for (const Database::Pointer & db: model.getDatabases()) {
        if (dbType.empty()) {
            dbType = db->getDriver();
        }
    }

    std::ofstream ofs{ tempName };

    ofs << "databaseType = \"" << dbType << "\"\n"
        << "name = \"AuthorDB\"\n"
        << "\n"
        << "[flyway]\n"
        << "mixed = true\n"
        << "outOfOrder = true\n"
        << "locations = [ \"filesystem:migrations\" ]\n"
        << "validateMigrationNaming = true\n"
        ;


    for (const Database::Pointer & db: model.getDatabases()) {
        ofs
            << "[environments." << db->getEnvName() << "]\n"

            << "url = \"jdbc:" << ShowLib::toLower(db->getDriver())
            << "://" << db->getHost()
            << ":" << ( db->getPort() > 0 ? std::to_string(db->getPort()) : "" )
            << "/" << db->getDbName() << "\"\n"

            << "user = \"" << db->getUsername() << "\"\n"
            ;

        if (!db->getPassword().empty()) {
            ofs << "password = \"" << db->getPassword() << "\"\n";
        }

        ofs << "schemas = [ \"public\" ]\n" ;
    }

    ofs.close();
    ShowLib::FileUtilities::moveIfDifferences(outName, tempName);

    //----------------------------------------------------------------------
    // Ensure the migrations and schema-model directories exist.
    //----------------------------------------------------------------------
    ShowLib::FileUtilities::ensurePath( generatorInfo->getOutputBasePath() + "/migrations" );
    ShowLib::FileUtilities::ensurePath( generatorInfo->getOutputBasePath() + "/schema-model" );
}

/**
 * Generate migrations in outputBasePath()/migrations.
 *
 * @return True if there were changes and the model should be updated, False if not.
 */
bool CodeGenerator_Flyway::generate_Migrations() {
    genVersion = model.getGeneratedVersion();

    //----------------------------------------------------------------------
    // If this is the first generation...
    //----------------------------------------------------------------------
    if (genVersion == 0) {
        generateTo( migrationFileName("CreateDatabase"));
        for (const Table::Pointer & table: model.getTables()) {
            setGeneratedNames(*table);
        }
        return true;
    }

    string migrationName = model.getLatestMigrationName().length() > 0 ? model.getLatestMigrationName() : "Migration";
    string fname = migrationFileName(migrationName);
    std::ofstream ofs{ fname };
    bool didWork = false;
    ofs << "BEGIN;\n";

    for (const Table::Pointer & table: model.getTables()) {
        if (generate_TableMigrations(ofs, table)) {
            didWork = true;
        }
    }

    ofs << "COMMIT;\n";
    ofs.close();
    if (!didWork) {
        std::remove(fname.c_str());
    }

    return didWork;
}

/**
 * Handle migrations for this table.
 *
 * We might have changes of:
 *
 *	-Table Renames
 *	-Column:
 *		-Renames
 *		-Data type changes
 *		-New
 *		-Removed
 *
 * @return if we did anything.
 */
bool CodeGenerator_Flyway::generate_TableMigrations(std::ofstream &ofs, const Table::Pointer &table) {
    bool didWork = false;

    // Is this a new table?
    if (table->getVersion() == 0) {
        ofs << "\n";
        generateForTable(ofs, *table);
        setGeneratedNames(*table);
        didWork = true;
    }

    else {
        if (table->getVersion() > genVersion) {
            ofs << "\n";

            generate_TableNameChanges(ofs, table);
            generate_ColumnChanges(ofs, table);

            setGeneratedNames(*table);
            didWork = true;
        }
    }

    return didWork;
}

/**
 * @return if we did anything.
 */
bool CodeGenerator_Flyway::generate_TableNameChanges(std::ofstream &ofs, const Table::Pointer &table) {
    bool didWork = false;

    if (!table->getDbNameGenerated().empty() && table->getDbName() != table->getDbNameGenerated()) {
        ofs << "ALTER TABLE " << table->getDbNameGenerated() << " RENAME TO " << table->getDbName() << ";\n";

        didWork = true;
    }
    return didWork;
}

/**
 *		-Renames
 *		-Data type changes
 *		-New
 *		-Removed
 */
bool CodeGenerator_Flyway::generate_ColumnChanges(std::ofstream &ofs, const Table::Pointer &table) {
    bool didWork = false;

    for (const Column::Pointer & col: table->getColumns()) {
        if (col->getVersion() > genVersion) {
            // Is it a new column?
            if (col->getDbNameGenerated().empty()) {
                ofs << "ALTER TABLE " << table->getDbNameGenerated()
                    << " ADD COLUMN ";
                generateDefinitionFor(ofs, *col);
                ofs << ";\n";
                didWork = true;
            }

            else {
                // Otherwise it might be a column name change.
                if (col->getDbName() != col->getDbNameGenerated()) {
                    ofs << "ALTER TABLE " << table->getDbNameGenerated()
                        << " RENAME COLUMN " << col->getDbNameGenerated() << " TO " << col->getDbName()
                        ;
                    didWork = true;
                }

                // It could also be a different datatype.
                if (col->hasDataTypeChanged()) {
                    ofs << "ALTER TABLE " << table->getDbNameGenerated()
                        << " ALTER COLUMN ";
                    generateDefinitionFor(ofs, *col);
                    ofs << ";\n";
                    didWork = true;
                }
            }
        }
    }

    for (const Column::Pointer &col: table->getDeletedColumns()) {
        ofs << "ALTER TABLE " << table->getDbNameGenerated() << " REMOVE COLUMN " << col->getDbNameGenerated() << ";\n";
        didWork = true;
    }
    table->clearDeletedColumns();

    return didWork;
}



/**
 * Return a migration filename. We assumed versioned migrations of the format:
 *
 * V001__YYYYMMDDhhmmss_NNN_Comment.sql.
 */
string CodeGenerator_Flyway::migrationFileName(const string &comment) {
    string dirName = generatorInfo->getOutputBasePath() + "/migrations/";
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    string nowStr = date::format("%Y%m%d%H%M%S", now);

    return dirName + "/V001__" + nowStr + "_" + ShowLib::toString(++sequence, 4) + "_" + comment + ".sql";
}

/**
 * We've done the generation for this table. Update accordingly.
 */
void CodeGenerator_Flyway::setGeneratedNames(Table &table) {
    int genVersion = model.getGeneratedVersion() + 1;

    table.setVersion(genVersion)
        .setDbNameGenerated(table.getDbName())
        ;

    for (const Column::Pointer & col: table.getColumns()) {
        col->setGeneratedValues();
        if (col->getVersion() == 0) {
            col->setVersion(genVersion);
        }
    }
}

/**
 * Write out the model.
 */
void CodeGenerator_Flyway::saveModel() {
    string filename = model.getFilename();
    if (filename.length() > 0) {
        std::ofstream ofs {filename};
        JSON json = model.toJSON();
        ofs << json.dump(2);
        model.markClean();
    }
}
