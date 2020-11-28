#include <iostream>
#include <cerrno>

#include <showlib/OptionHandler.h>

#include "Processor.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

using namespace ShowLib;

static void specifyColumn(const std::string &);
static void specifyReferences(const std::string &);

static Processor processor;
static DataModel::Table::Pointer table;
static DataModel::Column::Pointer column;

int main(int argc, char **argv)
{
    OptionHandler::ArgumentVector args;

    bool doCreate = false;
    bool doGenerate = false;

    args.addNoArg("create",   [&](const char *){ doCreate = true; }, "Create a DataModel file");
    args.addNoArg("generate", [&](const char *){ doGenerate = true; }, "Generate output");
    args.addArg  ("model",    [&](const char *arg){ processor.setFileName(arg); }, "fname", "Specify the input/output data model file");
    args.addArg  ("srcdir",   [&](const char *arg){ processor.setSrcDirName(arg); }, "dirname", "Directory for .cpp files");
    args.addArg  ("table",    [&](const char *arg){ table = processor.specifyTable(arg); }, "tablename", "Create/Update this table");
    args.addArg  ("column",   [&](const char *arg){ specifyColumn(arg); }, "columnname[,type]", "Create/Update this column");
    args.addNoArg("pk",       [&](const char *){ if (column != nullptr) column->setIsPrimaryKey(true); }, "Mark column as a primery key.");
    args.addNoArg("notnull",  [&](const char *){ if (column != nullptr) column->setNullable(false); }, "Mark column as NOT NULL.");
    args.addArg  ("ref",      [&](const char *arg){ specifyReferences(arg); }, "table[.col]", "Mark this foreign key reference.");

    if (!OptionHandler::handleOptions(argc, argv, args)) {
        return 1;
    }

    if (doCreate) {
        processor.writeModel();
    }

    if (doGenerate) {
        processor.generate();
    }

    return 0;
}

/**
 * We're going to work with a particular column.
 */
void
specifyColumn(const std::string &arg) {
    if (table == nullptr) {
        cerr << "Must specify --table before --column." << endl;
        exit(2);
    }

    column = processor.specifyColumn(*table, arg);
}

/**
 * Mark the current column as a foreign key reference. For now, we just
 * track it as a string, but DataModel can update all references.
 */
void
specifyReferences(const std::string &arg) {
    if (column == nullptr) {
        cerr << "Must specify --column before --ref." << endl;
        exit(2);
    }

    column->setReferenceStr(arg);
    processor.fixReferences();
}
