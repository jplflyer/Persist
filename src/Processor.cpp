#include <iostream>
#include <fstream>

#include <showlib/CommonUsing.h>
#include <showlib/FileUtilities.h>
#include <showlib/StringUtils.h>

#include "CodeGenerator_CPP.h"
#include "CodeGenerator_SQL.h"
#include "CodeGenerator_DB.h"
#include "CodeGenerator_Java.h"
#include "CodeGenerator_Flyway.h"
#include "Processor.h"

using namespace ShowLib;

using Generator = DataModel::Generator;

/**
 * Working with this file. If it exists, read it.
 */
Processor & Processor::setFileName(const std::string &value) {
    fileName = value;
    model.setFilename(fileName);

    string contents = FileUtilities::readFile(fileName);
    if (contents.length() > 0) {
        JSON json = JSON::parse(contents);
        model.fromJSON(json);
        model.fixReferences();
    }

    return *this;
}

/**
 * During generate(), we can override which of the generators we'll actually do. This won't let you
 * create a new generator to run, but you could do just a single one instead of all of them.
 */
void Processor::addGenerator(const std::string &name) {
    generatorNames.add(name);
}

/**
 * We can initialize a new model.
 */
void Processor::writeModel() {
    std::ofstream ofs{fileName};
    JSON json = model.toJSON();
    ofs << json.dump(2) << endl;
}

void Processor::listGenerators() {
    for (const Generator::Pointer & generator: model.getGenerators()) {
        string name = generator->getName();

        if (generatorNames.size() > 0 && !generatorNames.contains(name)) {
            continue;
        }

        cout << name << endl;
    }
}

/**
 * Perform code generation.
 */
void Processor::generate() {
    for (const Generator::Pointer & generator: model.getGenerators()) {
        string name = generator->getName();

        if (generatorNames.size() > 0 && !generatorNames.contains(name)) {
            continue;
        }

        if (name == Generator::NAME_SQL) {
            CodeGenerator_SQL sqlGen(model, generator);
            sqlGen.generate();
        }
        else if (name == Generator::NAME_CPP) {
            CodeGenerator_CPP cppGen(model, generator);
            cppGen.generate();
        }
        else if (name == Generator::NAME_CPP_DBACCESS) {
            CodeGenerator_DB dbGen(model, generator);
            dbGen.generate();
        }
        else if (name == Generator::NAME_JAVA) {
            CodeGenerator_Java javaGen(model, generator);
            javaGen.generate();
        }
        else if (name == Generator::NAME_FLYWAY) {
            CodeGenerator_Flyway flywayGen(model, generator);
            flywayGen.generate();
        }
    }
}
