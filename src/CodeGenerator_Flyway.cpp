#include <chrono>
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
    int genVersion = model.getGeneratedVersion();

    //----------------------------------------------------------------------
    // If this is the first generation...
    //----------------------------------------------------------------------
    if (genVersion == 0) {
        generateTo( migrationFileName("CreateDatabase"));
        return true;
    }

    for (const Table::Pointer & table: model.getTables()) {
        // Is this a new table?
        if (table->getVersion() == 0) {
        }

        else {
            if (table->getVersion() > genVersion) {
            }
        }
    }

    return false;
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
 * Write out the model.
 */
void CodeGenerator_Flyway::saveModel() {
}
