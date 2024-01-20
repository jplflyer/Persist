#pragma once

#include <string>

#include <showlib/StringVector.h>
#include "DataModel.h"

/**
 * This is the DataModeler's main processor.
 */
class Processor
{
public:
    Processor & setFileName(const std::string &value);

    void listGenerators();
    void writeModel();
    void generate();
    void addGenerator(const std::string &genName);

private:
    std::string		fileName;
    DataModel		model;

    ShowLib::StringVector generatorNames;
};

