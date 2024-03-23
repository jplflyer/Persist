#include <iomanip>
#include <iostream>

#include <showlib/CommonUsing.h>
#include <showlib/OptionHandler.h>

#include "Processor.h"

using ShowLib::OptionHandler;
using Generator = DataModel::Generator;

static Processor processor;

int main(int argc, char **argv)
{
    cout << std::boolalpha;

    OptionHandler::ArgumentVector args;

    bool doCreate = false;
    bool doGenerate = false;
    bool doListGen = false;

    args.addNoArg("create",     [&](const char *){ doCreate = true; }, "Create a DataModel file");
    args.addNoArg("generate",   [&](const char *){ doGenerate = true; }, "Generate output");
    args.addNoArg("gen",        [&](const char *){ doGenerate = true; }, "Shortcut for --generate");
    args.addNoArg("listgen",    [&](const char *){ doListGen = true; }, "List the configured generators");
    args.addArg  ("model",      [&](const char *arg){ processor.setFileName(arg); }, "fname", "Specify the input/output data model file");

    args.addNoArg("flyway",     [&](const char *){ processor.addGenerator(Generator::NAME_FLYWAY); }, "To limit which generators to run");
    args.addNoArg("sql",        [&](const char *){ processor.addGenerator(Generator::NAME_SQL); }, "To limit which generators to run");
    args.addNoArg("java",       [&](const char *){ processor.addGenerator(Generator::NAME_JAVA); }, "To limit which generators to run");

    if (!OptionHandler::handleOptions(argc, argv, args)) {
        return 1;
    }

    if (doCreate) {
        processor.writeModel();
    }

    if (doListGen) {
        processor.listGenerators();
    }

    if (doGenerate) {
        cout << "Time to generate." << endl;
        processor.generate();
    }

    return 0;
}
